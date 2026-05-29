from __future__ import annotations

from dataclasses import dataclass
from datetime import UTC, datetime
from pathlib import Path

from sqlalchemy import Boolean, DateTime, Integer, String, select
from sqlalchemy.ext.asyncio import (
    AsyncEngine,
    AsyncSession,
    async_sessionmaker,
    create_async_engine,
)
from sqlalchemy.orm import DeclarativeBase, Mapped, mapped_column


class Base(DeclarativeBase):
    pass


class KV(Base):
    __tablename__ = "kv"

    key: Mapped[str] = mapped_column(String, primary_key=True)
    value: Mapped[str] = mapped_column(String, nullable=False)
    updated_at: Mapped[datetime] = mapped_column(
        DateTime(timezone=True), nullable=False
    )


class ClientState(Base):
    __tablename__ = "client_state"

    client_id: Mapped[int] = mapped_column(Integer, primary_key=True)
    name: Mapped[str] = mapped_column(String, nullable=False)
    company_id: Mapped[int] = mapped_column(Integer, nullable=False)
    is_spectator: Mapped[bool] = mapped_column(Boolean, nullable=False)
    first_seen_at: Mapped[datetime] = mapped_column(
        DateTime(timezone=True), nullable=False
    )
    last_seen_at: Mapped[datetime] = mapped_column(
        DateTime(timezone=True), nullable=False
    )
    greeted: Mapped[bool] = mapped_column(Boolean, nullable=False, default=False)
    instructions_sent: Mapped[bool] = mapped_column(
        Boolean, nullable=False, default=False
    )


class CompanyState(Base):
    __tablename__ = "company_state"

    company_id: Mapped[int] = mapped_column(Integer, primary_key=True)
    name: Mapped[str] = mapped_column(String, nullable=False)
    last_value: Mapped[int] = mapped_column(Integer, nullable=False, default=0)
    last_value_at: Mapped[datetime] = mapped_column(
        DateTime(timezone=True), nullable=False
    )


@dataclass(frozen=True)
class Storage:
    db_path: Path
    engine: AsyncEngine
    sessionmaker: async_sessionmaker[AsyncSession]

    @staticmethod
    async def open(db_path: Path) -> Storage:
        db_path.parent.mkdir(parents=True, exist_ok=True)
        engine = create_async_engine(f"sqlite+aiosqlite:///{db_path}", future=True)
        sessionmaker_ = async_sessionmaker(engine, expire_on_commit=False)
        async with engine.begin() as conn:
            await conn.run_sync(Base.metadata.create_all)
        return Storage(db_path=db_path, engine=engine, sessionmaker=sessionmaker_)

    async def close(self) -> None:
        await self.engine.dispose()

    async def get_kv(self, key: str) -> str | None:
        async with self.sessionmaker() as session:
            row = await session.get(KV, key)
            return row.value if row else None

    async def set_kv(self, key: str, value: str) -> None:
        now = datetime.now(tz=UTC)
        async with self.sessionmaker() as session:
            existing = await session.get(KV, key)
            if existing is None:
                session.add(KV(key=key, value=value, updated_at=now))
            else:
                existing.value = value
                existing.updated_at = now
            await session.commit()

    async def upsert_client(
        self,
        *,
        client_id: int,
        name: str,
        company_id: int,
        is_spectator: bool,
        seen_at: datetime,
    ) -> ClientState:
        async with self.sessionmaker() as session:
            existing = await session.get(ClientState, client_id)
            if existing is None:
                existing = ClientState(
                    client_id=client_id,
                    name=name,
                    company_id=company_id,
                    is_spectator=is_spectator,
                    first_seen_at=seen_at,
                    last_seen_at=seen_at,
                    greeted=False,
                    instructions_sent=False,
                )
                session.add(existing)
            else:
                existing.name = name
                existing.company_id = company_id
                existing.is_spectator = is_spectator
                existing.last_seen_at = seen_at
            await session.commit()
            return existing

    async def mark_greeted(self, client_id: int) -> None:
        async with self.sessionmaker() as session:
            existing = await session.get(ClientState, client_id)
            if existing is None:
                return
            existing.greeted = True
            await session.commit()

    async def mark_instructions_sent(self, client_id: int) -> None:
        async with self.sessionmaker() as session:
            existing = await session.get(ClientState, client_id)
            if existing is None:
                return
            existing.instructions_sent = True
            await session.commit()

    async def list_clients(self) -> list[ClientState]:
        async with self.sessionmaker() as session:
            rows = (await session.execute(select(ClientState))).scalars().all()
            return list(rows)

    async def upsert_company_value(
        self, *, company_id: int, name: str, value: int, value_at: datetime
    ) -> None:
        async with self.sessionmaker() as session:
            existing = await session.get(CompanyState, company_id)
            if existing is None:
                existing = CompanyState(
                    company_id=company_id,
                    name=name,
                    last_value=value,
                    last_value_at=value_at,
                )
                session.add(existing)
            else:
                existing.name = name
                existing.last_value = value
                existing.last_value_at = value_at
            await session.commit()

    async def list_company_values(self) -> list[CompanyState]:
        async with self.sessionmaker() as session:
            rows = (await session.execute(select(CompanyState))).scalars().all()
            return list(rows)
