/*
 * gRPC Server for OpenTTD
 * Standalone executable that exposes gRPC and forwards calls to OpenTTD
 * through the UDS IPC bridge plugin.
 */

#include "call_base.h"
#include "grpc_server_services.h"

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include <spdlog/spdlog.h>

#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>

std::string g_uds_socket_path;

static std::unique_ptr<grpc::Server> g_grpc_server = nullptr;
static std::unique_ptr<grpc::ServerCompletionQueue> g_completion_queue = nullptr;

static void ShutdownGRPCServer()
{
	if (g_grpc_server != nullptr)
	{
		g_grpc_server->Shutdown();
	}

	if (g_completion_queue != nullptr)
	{
		g_completion_queue->Shutdown();

		void *tag = nullptr;
		bool ok = false;
		while (g_completion_queue->Next(&tag, &ok))
		{
			static_cast<CallBase *>(tag)->Proceed(ok);
		}
	}

	g_completion_queue.reset();
	g_grpc_server.reset();
}

static bool StartGRPCServer(const std::string &hostname, const std::string &port)
{
	std::string server_address = hostname + ":" + port;

	try
	{
		grpc::ServerBuilder builder;
		builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

		RegisterGRPCServices(builder);
		g_completion_queue = builder.AddCompletionQueue();
		g_grpc_server = builder.BuildAndStart();

		if (g_grpc_server == nullptr)
		{
			spdlog::error("[gRPC Server] Failed to build gRPC server");
			return false;
		}

		RegisterGRPCHandlers(builder, g_completion_queue);
		spdlog::info("[gRPC Server] Listening on {}", server_address);
		return true;
	}
	catch (const std::exception &e)
	{
		spdlog::error("[gRPC Server] Failed to start gRPC server: {}", e.what());
		return false;
	}
}

static void RunCompletionQueueLoop()
{
	while (true)
	{
		void *tag = nullptr;
		bool ok = false;
		auto status = g_completion_queue->AsyncNext(&tag, &ok, gpr_time_0(GPR_CLOCK_REALTIME));

		if (status == grpc::ServerCompletionQueue::SHUTDOWN)
		{
			break;
		}

		if (status == grpc::ServerCompletionQueue::TIMEOUT)
		{
			continue;
		}

		if (status == grpc::ServerCompletionQueue::GOT_EVENT)
		{
			static_cast<CallBase *>(tag)->Proceed(ok);
		}
	}
}

int main(int argc, char **argv)
{
	const char *grpc_enabled_env = std::getenv("OTTD_GRPC_ENABLED");
	if (grpc_enabled_env == nullptr ||
		(strcmp(grpc_enabled_env, "1") != 0 && strcmp(grpc_enabled_env, "true") != 0))
	{
		spdlog::info("[gRPC Server] Disabled (OTTD_GRPC_ENABLED not set to 1 or true)");
		return 0;
	}

	const char *socket_path_env = std::getenv("OTTD_UDS_SOCKET_PATH");
	g_uds_socket_path = socket_path_env != nullptr ? socket_path_env : "/run/openttd/rpc.sock";

	const char *hostname_env = std::getenv("OTTD_GRPC_HOSTNAME");
	std::string hostname = hostname_env != nullptr ? hostname_env : "0.0.0.0";

	const char *port_env = std::getenv("OTTD_GRPC_PORT");
	std::string port = port_env != nullptr ? port_env : "50051";

	spdlog::info("[gRPC Server] Using UDS bridge at {}", g_uds_socket_path);

	if (!StartGRPCServer(hostname, port))
	{
		return 1;
	}

	std::atexit(ShutdownGRPCServer);
	RunCompletionQueueLoop();
	return 0;
}
