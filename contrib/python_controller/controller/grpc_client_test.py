import asyncio
import uuid
from collections.abc import AsyncIterable
from typing import cast

import grpc_gen.admin.openttd as admin
import grpc_gen.script_goal.openttd as script_goal
from dishka import Scope, make_async_container, provide
from grpclib.client import Channel

from controller.grpc_services import AppProvider


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


async def test_goal() -> None:
    container = make_async_container(_GrpcAppProvider("127.0.0.1", 50051))

    admin_service = await container.get(admin.AdminStub)
    company_list = await admin_service.get_company_list(admin.GetCompanyListRequest())
    print(company_list)

    goal_service = await container.get(script_goal.ScriptGoalStub)
    goal_random_id = str(uuid.uuid4())
    goal_response = await goal_service.new(
        script_goal.NewGoalRequest(
            company=0,
            goal_text=f"Random goal {goal_random_id}",
            goal_type=cast(script_goal.GoalType, script_goal.GoalType.GT_COMPANY),
            goal_global=False,
        )
    )
    print(goal_random_id, goal_response)


def main() -> None:
    print("Hello from grpc_client_test!")
    asyncio.run(test_goal())


if __name__ == "__main__":
    main()
