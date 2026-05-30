from __future__ import annotations

import argparse
import asyncio
import logging
import os
import sys
from collections.abc import AsyncIterable
from dataclasses import dataclass
from datetime import UTC, datetime
from pathlib import Path
from typing import cast

import grpc_gen.admin.openttd as admin
import grpc_gen.script_company.openttd as script_company
import grpc_gen.script_goal.openttd as script_goal
import grpc_gen.script_map.openttd as script_map
from dishka import AsyncContainer, Scope, make_async_container, provide
from grpc_gen.console.openttd import (
    ConsoleStub,
    ResetCompanyRequest,
    RestartRequest,
    SettingNewgameRequest,
    SettingRequest,
)
from grpclib.client import Channel

from controller.company_value_goal.storage import Storage
from controller.grpc_services import AppProvider

logger = logging.getLogger(__name__)

WIN_VALUE = 300_000
SYNC_INTERVAL_S = 1.0
SCOREBOARD_INTERVAL_S = 5 * 60.0
POST_WIN_RESTART_DELAY_S = 60.0

SPECTATOR_PLAYER_NAME = "controller"

# ScriptGoal::BUTTON_START (1 << 11)
GOAL_QUESTION_BUTTON_START = 2048


def _win_value_compact(*, value: int = WIN_VALUE) -> str:
    """Short label for goals (e.g. 10M, 1.5M); falls back to comma-separated."""
    if value >= 1_000_000 and value % 1_000_000 == 0:
        return f"{value // 1_000_000}M"
    if value >= 1_000_000:
        return f"{value / 1_000_000:g}M"
    if value >= 1_000 and value % 1_000 == 0:
        return f"{value // 1_000}K"
    return f"{value:,}"


@dataclass(frozen=True)
class ControllerConfig:
    server_name: str
    spectator_player_name: str
    scenario_path: str
    max_clients: int
    max_companies: int
    visibility_type: admin.ServerVisibilityType


DEFAULT_CONFIG = ControllerConfig(
    server_name="company-value-goal",
    spectator_player_name=SPECTATOR_PLAYER_NAME,
    scenario_path=os.environ.get("OTTD_SCENARIO_PATH", "sc_hilly1.scn"),
    max_clients=10,
    max_companies=10,
    visibility_type=cast(admin.ServerVisibilityType, admin.ServerVisibilityType.LOCAL),
)


def _grpc_host_port() -> tuple[str, int]:
    host = os.environ.get("OTTD_GRPC_HOSTNAME", "127.0.0.1")
    if host in ("[::]", "::"):
        host = "127.0.0.1"
    port = int(os.environ.get("OTTD_GRPC_PORT", "50051"))
    return host, port


class _GrpcAppProvider(AppProvider):
    def __init__(self, host: str, port: int) -> None:
        super().__init__()
        self._grpc_host = host
        self._grpc_port = port

    @provide(scope=Scope.APP)
    async def grpc_channel(self) -> AsyncIterable[Channel]:
        channel = Channel(host=self._grpc_host, port=self._grpc_port)
        yield channel
        channel.close()


def _utc_iso_compact(dt: datetime) -> str:
    # file-name safe (no ":"), still UTC ISO-ish
    s = dt.astimezone(UTC).replace(microsecond=0).isoformat().replace("+00:00", "Z")
    return s.replace(":", "")


def _sessions_dir() -> Path:
    # Per project convention: project root is two levels up from this file.
    # DB files live directly under ./storage/
    project_root = Path(__file__).resolve().parents[2]
    return project_root / "storage"


async def _wait_for_grpc(admin_service: admin.AdminStub) -> None:
    while True:
        try:
            await admin_service.get_game_mode(admin.GetGameModeRequest())
            return
        except Exception:
            logger.exception("gRPC ping failed; retrying")
            await asyncio.sleep(2)


async def _ensure_network_server(
    container: AsyncContainer, cfg: ControllerConfig
) -> None:
    admin_service = await container.get(admin.AdminStub)
    game_mode = await admin_service.get_game_mode(admin.GetGameModeRequest())
    if game_mode.current_mode == admin.GameModeType.MULTIPLAYER_SERVER:
        logger.info(
            "Connected to existing multiplayer server (server_name=%r)",
            game_mode.server_name,
        )
        return

    # Standby mode: start a fresh network session.
    logger.info(
        "Starting new network game (scenario=%r, server_name=%r)",
        cfg.scenario_path,
        cfg.server_name,
    )

    await admin_service.start_network_server(
        admin.StartNetworkServerRequest(
            server_name=cfg.server_name,
            spectator_player_name=cfg.spectator_player_name,
            max_clients=cfg.max_clients,
            max_companies=cfg.max_companies,
            visibility_type=cfg.visibility_type,
            server_password="",
            scenario_path=cfg.scenario_path,
            script_name="Company Value Goal",
            script_version=101,
        )
    )

    # Wait until mode switches.
    for _ in range(120):
        mode = await admin_service.get_game_mode(admin.GetGameModeRequest())
        if mode.current_mode == admin.GameModeType.MULTIPLAYER_SERVER:
            logger.info(
                "Network game is now running (server_name=%r)", mode.server_name
            )
            return
        await asyncio.sleep(1)

    logger.warning("Timed out waiting for server to enter MULTIPLAYER_SERVER mode")


async def _reset_idle_companies(container: AsyncContainer) -> None:
    admin_service = await container.get(admin.AdminStub)
    console = await container.get(ConsoleStub)

    company_list = await admin_service.get_company_list(admin.GetCompanyListRequest())
    for company in company_list.companies:
        if not company.client_ids:
            await console.reset_company(
                ResetCompanyRequest(company_id=company.company_id)
            )


async def _center_tile_index(container: AsyncContainer) -> int:
    map_service = await container.get(script_map.ScriptMapStub)
    size_x = (await map_service.get_map_size_x(script_map.GetMapSizeXRequest())).size_x
    size_y = (await map_service.get_map_size_y(script_map.GetMapSizeYRequest())).size_y
    return (
        await map_service.get_tile_index(
            script_map.GetTileIndexRequest(x=size_x // 2, y=size_y // 2)
        )
    ).tile_index


async def _ensure_global_goal(container: AsyncContainer, storage: Storage) -> int:
    existing = await storage.get_kv("global_goal_id")
    if existing is not None:
        return int(existing)

    goal_service = await container.get(script_goal.ScriptGoalStub)
    center = await _center_tile_index(container)
    goal_response = await goal_service.new(
        script_goal.NewGoalRequest(
            company=-1,
            goal_text=f"Reach {_win_value_compact()} company value",
            goal_type=cast(script_goal.GoalType, script_goal.GoalType.GT_TILE),
            destination=center,
            goal_global=True,
        )
    )

    await storage.set_kv("global_goal_id", str(goal_response.goal_id))
    return goal_response.goal_id


async def _load_or_create_storage(container: AsyncContainer) -> Storage:
    """
    Per README: keep separate sqlite per session; reuse latest unless a new session is detected.

    Detection heuristic:
    - if server is not in MULTIPLAYER_SERVER -> new session will be created elsewhere
    - else reuse latest db file, but if its stored `server_name` differs from live `server_name`, start new.
    """
    sessions_dir = _sessions_dir()
    sessions_dir.mkdir(parents=True, exist_ok=True)

    admin_service = await container.get(admin.AdminStub)
    game_mode = await admin_service.get_game_mode(admin.GetGameModeRequest())

    latest = sorted(sessions_dir.glob("session_*.sqlite"), reverse=True)
    if latest:
        storage = await Storage.open(latest[0])
        stored_name = await storage.get_kv("server_name")
        if stored_name in ("", game_mode.server_name):
            return storage
        await storage.close()

    ts = _utc_iso_compact(datetime.now(tz=UTC))
    db_path = sessions_dir / f"session_{ts}.sqlite"
    storage = await Storage.open(db_path)
    await storage.set_kv("created_at_utc", datetime.now(tz=UTC).isoformat())
    await storage.set_kv("server_name", game_mode.server_name)
    return storage


def _greeting_message(client_name: str) -> str:
    return (
        f"Welcome, {client_name}!\n\n"
        f"Goal: create or join a company, grow it, and reach {WIN_VALUE:,} company value.\n"
        "You can start as spectator, create a company, or join an existing one. "
        f"First company to reach {WIN_VALUE:,} wins."
    )


async def _greet_and_instruct_new_clients(
    *,
    admin_service: admin.AdminStub,
    goal_service: script_goal.ScriptGoalStub,
    storage: Storage,
    clients: list[admin.NetworkClient],
) -> None:
    now = datetime.now(tz=UTC)
    known = {c.client_id: c for c in await storage.list_clients()}

    for client in clients:
        state = await storage.upsert_client(
            client_id=client.client_id,
            name=client.client_name,
            company_id=client.company_id,
            is_spectator=client.is_spectator,
            seen_at=now,
        )

        if state.instructions_sent:
            continue

        # First sighting: wait briefly so the client finishes joining before the popup.
        if client.client_id not in known:
            await asyncio.sleep(1.5)

        question = _greeting_message(client.client_name)
        reply = await goal_service.question_client(
            script_goal.QuestionClientRequest(
                unique_id=client.client_id,
                client_id=client.client_id,
                question=question,
                type=cast(
                    script_goal.GoalQuestionType,
                    script_goal.GoalQuestionType.GQT_WARNING,
                ),
                buttons=GOAL_QUESTION_BUTTON_START,
            )
        )

        if reply.success:
            logger.info("Greeted client %s via goal question", client.client_name)
            await storage.mark_greeted(client.client_id)
            await storage.mark_instructions_sent(client.client_id)
            continue

        logger.warning(
            "Goal question greeting failed for client %s; falling back to chat",
            client.client_name,
        )
        await admin_service.send_chat_message(
            admin.SendChatMessageRequest(
                message=f"Welcome {client.client_name}! Type /help for OpenTTD help."
            )
        )
        await admin_service.send_private_chat_message(
            admin.SendPrivateChatMessageRequest(
                client_id=client.client_id,
                message=question,
            )
        )
        await storage.mark_greeted(client.client_id)
        await storage.mark_instructions_sent(client.client_id)


async def _update_company_values(
    *,
    company_service: script_company.ScriptCompanyStub,
    storage: Storage,
    companies: list[admin.Company],
) -> dict[int, int]:
    now = datetime.now(tz=UTC)
    values: dict[int, int] = {}
    for c in companies:
        # Use quarterly=0 as "current" snapshot (server-defined).
        v = (
            await company_service.get_quarterly_company_value(
                script_company.GetQuarterlyCompanyValueRequest(
                    company=int(c.company_id), quarter=0
                )
            )
        ).company_value
        values[int(c.company_id)] = int(v)
        await storage.upsert_company_value(
            company_id=int(c.company_id),
            name=c.company_name,
            value=int(v),
            value_at=now,
        )
    return values


def _format_scoreboard(values: list[tuple[str, int]]) -> str:
    parts: list[str] = []
    for idx, (name, value) in enumerate(values[:10], start=1):
        pct = min(100, int(value * 100 / WIN_VALUE))
        parts.append(f"{idx}. {name}: {value:,} ({pct}%)")
    return "\n".join(parts) if parts else "No active companies yet."


async def _announce_scoreboard(
    *,
    admin_service: admin.AdminStub,
    storage: Storage,
    company_service: script_company.ScriptCompanyStub,
) -> None:
    admin_companies = (
        await admin_service.get_company_list(admin.GetCompanyListRequest())
    ).companies
    values = await _update_company_values(
        company_service=company_service,
        storage=storage,
        companies=list(admin_companies),
    )
    sorted_values = sorted(
        (
            (c.company_name, values.get(int(c.company_id), 0))
            for c in admin_companies
            if values.get(int(c.company_id), 0) > 0
        ),
        key=lambda x: x[1],
        reverse=True,
    )
    await admin_service.send_chat_message(
        admin.SendChatMessageRequest(
            message=f"Company value race to {WIN_VALUE:,}: {_format_scoreboard(sorted_values)}"
        )
    )


async def _check_win_and_restart(
    *,
    container: AsyncContainer,
    cfg: ControllerConfig,
    storage: Storage,
    values: dict[int, int],
    companies: list[admin.Company],
) -> Storage | None:
    winners = [cid for cid, v in values.items() if v >= WIN_VALUE]
    if not winners:
        return None

    winner_id = winners[0]
    winner_company = next(
        (c for c in companies if int(c.company_id) == winner_id), None
    )
    admin_service = await container.get(admin.AdminStub)

    winner_name = (
        winner_company.company_name if winner_company else f"Company {winner_id}"
    )
    await admin_service.send_chat_message(
        admin.SendChatMessageRequest(
            message=f"{winner_name} wins! Reached {WIN_VALUE:,} company value. Restarting in {int(POST_WIN_RESTART_DELAY_S)}s."
        )
    )
    await asyncio.sleep(POST_WIN_RESTART_DELAY_S)

    console_service = await container.get(ConsoleStub)
    await console_service.restart(RestartRequest(mode="newgame"))

    # New session => new DB
    await storage.close()
    new_storage = await Storage.open(
        _sessions_dir() / f"session_{_utc_iso_compact(datetime.now(tz=UTC))}.sqlite"
    )
    await new_storage.set_kv("created_at_utc", datetime.now(tz=UTC).isoformat())
    await new_storage.set_kv("server_name", cfg.server_name)
    await _reset_idle_companies(container)
    await _ensure_global_goal(container, new_storage)
    return new_storage


SETTINGS_TO_APPLY = [
    ("vehicle.max_ships", "0"),
    ("vehicle.plane_crashes", "0"),
]


async def _apply_settings(container: AsyncContainer) -> None:
    console_service = await container.get(ConsoleStub)

    for name, value in SETTINGS_TO_APPLY:
        await console_service.setting_newgame(
            SettingNewgameRequest(name=name, value=value)
        )
        await console_service.setting(SettingRequest(name=name, value=value))


async def _poll_and_log_chat_messages(*, admin_service: admin.AdminStub) -> None:
    reply = await admin_service.poll_last_chat_messages(
        admin.PollLastChatMessagesRequest()
    )
    for msg in reply.messages:
        logger.info(
            "Chat message (client_id=%s action=%s desttype=%s): %s",
            msg.client_id,
            msg.action,
            msg.desttype,
            msg.message,
        )


async def run_controller(
    *, cfg: ControllerConfig = DEFAULT_CONFIG, one_tick: bool = False
) -> None:
    grpc_host, grpc_port = _grpc_host_port()
    logger.info("Connecting to gRPC at %s:%s", grpc_host, grpc_port)
    async with make_async_container(
        _GrpcAppProvider(grpc_host, grpc_port)
    ) as container:
        admin_service = await container.get(admin.AdminStub)
        await _wait_for_grpc(admin_service)

        await _ensure_network_server(container, cfg)

        await _apply_settings(container)

        storage = await _load_or_create_storage(container)

        await _reset_idle_companies(container)
        await _ensure_global_goal(container, storage)

        company_service = await container.get(script_company.ScriptCompanyStub)
        goal_service = await container.get(script_goal.ScriptGoalStub)

        last_scoreboard_at = 0.0
        while True:
            try:
                game_mode = await admin_service.get_game_mode(
                    admin.GetGameModeRequest()
                )
                if game_mode.current_mode != admin.GameModeType.MULTIPLAYER_SERVER:
                    await asyncio.sleep(2)
                    await _ensure_network_server(container, cfg)
                    continue

                await _poll_and_log_chat_messages(admin_service=admin_service)

                network = await admin_service.get_network_clients(
                    admin.GetNetworkClientsRequest()
                )
                network.clients = [
                    c for c in network.clients if c.client_name != SPECTATOR_PLAYER_NAME
                ]
                await _greet_and_instruct_new_clients(
                    admin_service=admin_service,
                    goal_service=goal_service,
                    storage=storage,
                    clients=list(network.clients),
                )

                companies_reply = await admin_service.get_company_list(
                    admin.GetCompanyListRequest()
                )
                values = await _update_company_values(
                    company_service=company_service,
                    storage=storage,
                    companies=list(companies_reply.companies),
                )

                now_s = asyncio.get_running_loop().time()
                if now_s - last_scoreboard_at >= SCOREBOARD_INTERVAL_S:
                    await _announce_scoreboard(
                        admin_service=admin_service,
                        storage=storage,
                        company_service=company_service,
                    )
                    last_scoreboard_at = now_s

                restarted = await _check_win_and_restart(
                    container=container,
                    cfg=cfg,
                    storage=storage,
                    values=values,
                    companies=list(companies_reply.companies),
                )
                if restarted is not None:
                    storage = restarted
                    last_scoreboard_at = 0.0

                if one_tick:
                    return

                await asyncio.sleep(SYNC_INTERVAL_S)
            except Exception:
                # Lost gRPC connectivity; wait and retry.
                logger.exception("Controller loop error; will retry after delay")
                await asyncio.sleep(2)
                await _wait_for_grpc(admin_service)


def main() -> None:
    parser = argparse.ArgumentParser(prog="company-value-goal-controller")
    parser.add_argument(
        "--one-tick",
        action="store_true",
        help="Run exactly one controller loop iteration and exit.",
    )
    args = parser.parse_args()

    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s %(levelname)s %(name)s: %(message)s",
        datefmt="%Y-%m-%d %H:%M:%S",
        handlers=[logging.StreamHandler(sys.stdout)],
        force=True,
    )
    logger.info("Starting controller")
    asyncio.run(run_controller(one_tick=bool(args.one_tick)))
    logger.info("Controller stopped")


if __name__ == "__main__":
    main()
