import asyncio

from dishka import make_async_container
from grpc_gen.admin import openttd

from controller.grpc_services import provider


async def async_main() -> None:
    async with make_async_container(provider) as container:
        service = await container.get(openttd.AdminStub)
        response = await service.get_game_mode(openttd.GetGameModeRequest())
        print(response)


def main() -> None:
    print("Hello from grpc_client_test!")

    asyncio.run(async_main())


if __name__ == "__main__":
    main()
