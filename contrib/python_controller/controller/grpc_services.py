from collections.abc import AsyncIterable

import grpc_gen.admin.openttd as admin
import grpc_gen.script_company.openttd as script_company
import grpc_gen.script_generic.openttd as script_generic
import grpc_gen.script_goal.openttd as script_goal
import grpc_gen.script_map.openttd as script_map
from dishka import Provider, Scope, provide
from grpc_gen.console.openttd import ConsoleStub
from grpclib.client import Channel


# ---------------------------------------------------------
# The Provider
# ---------------------------------------------------------
class AppProvider(Provider):
    # Scope.APP means this is a singleton for the lifetime of the container
    @provide(scope=Scope.APP)
    async def grpc_channel(self) -> AsyncIterable[Channel]:
        channel = Channel(host="127.0.0.1", port=50051)

        yield channel  # Yields the ready-to-use client

        # This teardown logic runs automatically when the container closes
        channel.close()

    @provide(scope=Scope.APP)
    async def admin_service(
        self, grpc_channel: Channel
    ) -> AsyncIterable[admin.AdminStub]:
        yield admin.AdminStub(grpc_channel)

    @provide(scope=Scope.APP)
    async def script_map_service(
        self, grpc_channel: Channel
    ) -> AsyncIterable[script_map.ScriptMapStub]:
        yield script_map.ScriptMapStub(grpc_channel)

    @provide(scope=Scope.APP)
    async def script_company_service(
        self, grpc_channel: Channel
    ) -> AsyncIterable[script_company.ScriptCompanyStub]:
        yield script_company.ScriptCompanyStub(grpc_channel)

    @provide(scope=Scope.APP)
    async def goal_service(
        self, grpc_channel: Channel
    ) -> AsyncIterable[script_goal.ScriptGoalStub]:
        yield script_goal.ScriptGoalStub(grpc_channel)

    @provide(scope=Scope.APP)
    async def generic_service(
        self, grpc_channel: Channel
    ) -> AsyncIterable[script_generic.ScriptGenericStub]:
        yield script_generic.ScriptGenericStub(grpc_channel)

    @provide(scope=Scope.APP)
    async def console_service(
        self, grpc_channel: Channel
    ) -> AsyncIterable[ConsoleStub]:
        yield ConsoleStub(grpc_channel)


provider = AppProvider()
