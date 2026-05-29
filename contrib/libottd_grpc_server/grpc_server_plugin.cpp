/*
 * gRPC Server Plugin for OpenTTD
 * This plugin provides a gRPC interface to OpenTTD by acting as a bridge
 * between external gRPC clients and OpenTTD's C-ABI RPC interface.
 */

#include "plugin_interface.h"
#include "call_base.h"

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include "admin.grpc.pb.h"
#include "console.grpc.pb.h"
#include "script_company.grpc.pb.h"
#include "script_generic.grpc.pb.h"
#include "script_goal.grpc.pb.h"
#include "script_map.grpc.pb.h"

#include "grpc_server_services.h"

#include "spdlog/spdlog.h"

#include <cstdlib>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>

/**
 * Plugin error codes.
 */
enum PluginError
{
	PLUGIN_SUCCESS = 0,
	PLUGIN_ERROR_NULL_PARAMETER = 100,
	PLUGIN_ERROR_NOT_INITIALIZED = 101,
	PLUGIN_ERROR_RPC_FAILED = 102,
	PLUGIN_ERROR_MEMORY_FAILED = 103,
	PLUGIN_ERROR_GRPC_DISABLED = 104,
};

// Plugin state (declared as extern in call_base.h)
HostOps g_host_ops = {};
RPCHandler g_rpc_handler = nullptr;
static std::unique_ptr<grpc::Server> g_grpc_server = nullptr;
static std::unique_ptr<grpc::ServerCompletionQueue> g_completion_queue = nullptr;
static bool g_grpc_enabled = false;

static void ShutdownGRPCServer()
{
	if (!g_grpc_enabled) return;

	/* gRPC teardown must drain the completion queue before destroying the server.
	 * Otherwise gRPC aborts in grpc_completion_queue destruction with
	 * `queue.num_items() == 0` checks. */
	try
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
	}
	catch (const std::exception &e)
	{
		spdlog::error("[gRPC Plugin] Exception during shutdown: {}", e.what());
	}
	catch (...)
	{
		spdlog::error("[gRPC Plugin] Unknown exception during shutdown");
	}

	g_completion_queue.reset();
	g_grpc_server.reset();
	g_grpc_enabled = false;
}

static void EnsureShutdownRegistered()
{
	static std::once_flag once;
	std::call_once(once, []()
				   { std::atexit(ShutdownGRPCServer); });
}


// Handler creation functions moved to proxy_services/ directory

// Plugin interface implementation

extern "C" int32_t GetAPIVersion()
{
	return PLUGIN_API_VERSION;
}

extern "C" const char *PluginGetLastError(int32_t error_code)
{
	static char error_buffer[256];

	switch (error_code)
	{
		case PLUGIN_SUCCESS:
			return "Success";
		case PLUGIN_ERROR_NULL_PARAMETER:
			return "gRPC Plugin: Null parameter provided";
		case PLUGIN_ERROR_NOT_INITIALIZED:
			return "gRPC Plugin: Not initialized";
		case PLUGIN_ERROR_RPC_FAILED:
			return "gRPC Plugin: RPC call failed";
		case PLUGIN_ERROR_MEMORY_FAILED:
			return "gRPC Plugin: Memory operation failed";
		case PLUGIN_ERROR_GRPC_DISABLED:
			return "gRPC Plugin: gRPC server disabled via OTTD_GRPC_ENABLED";
		default:
			snprintf(error_buffer, sizeof(error_buffer),
					 "gRPC Plugin: Unknown error code: %d", error_code);
			return error_buffer;
	}
}

bool HostOpsReady()
{
	return g_host_ops.get_last_error != nullptr &&
		   g_host_ops.memory.create != nullptr &&
		   g_host_ops.memory.allocate != nullptr &&
		   g_host_ops.memory.deallocate != nullptr &&
		   g_host_ops.memory.release_all != nullptr &&
		   g_host_ops.memory.destroy != nullptr;
}

extern "C" void RegisterHostOps(const HostOps *ops)
{
	spdlog::info("[gRPC Plugin] RegisterHostOps called");
	if (ops == nullptr)
	{
		g_host_ops = {};
		return;
	}
	g_host_ops = *ops;
}

extern "C" int32_t StartRPCServer()
{
	spdlog::info("[gRPC Plugin] StartRPCServer called");

	// Check initialization
	if (!HostOpsReady())
	{
		spdlog::error("[gRPC Plugin] ERROR: Host operations not registered!");
		return PLUGIN_ERROR_NOT_INITIALIZED;
	}

	// Check if gRPC is enabled via environment variable
	const char *grpc_enabled_env = std::getenv("OTTD_GRPC_ENABLED");
	if (grpc_enabled_env == nullptr ||
		(strcmp(grpc_enabled_env, "1") != 0 && strcmp(grpc_enabled_env, "true") != 0))
	{
		spdlog::info("[gRPC Plugin] gRPC server disabled (OTTD_GRPC_ENABLED not set to 1 or true)");
		g_grpc_enabled = false;
		return PLUGIN_SUCCESS;
	}

	g_grpc_enabled = true;

	// Get configuration from environment variables
	const char *hostname_env = std::getenv("OTTD_GRPC_HOSTNAME");
	std::string hostname = hostname_env ? hostname_env : "0.0.0.0";

	const char *port_env = std::getenv("OTTD_GRPC_PORT");
	std::string port = port_env ? port_env : "50051";

	std::string server_address = hostname + ":" + port;

	try
	{
		// Initialize gRPC server with async API
		grpc::ServerBuilder builder;
		builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

		// Register async services
		RegisterGRPCServices(builder);

		// Create completion queue
		g_completion_queue = builder.AddCompletionQueue();

		// Build and start server
		g_grpc_server = builder.BuildAndStart();

		if (g_grpc_server == nullptr)
		{
			spdlog::error("[gRPC Plugin] Failed to build gRPC server");
			return PLUGIN_ERROR_NOT_INITIALIZED;
		}

		EnsureShutdownRegistered();

		RegisterGRPCHandlers(builder, g_completion_queue);

		spdlog::info("[gRPC Plugin] gRPC server started and listening on {}", server_address);

		return PLUGIN_SUCCESS;
	}
	catch (const std::exception &e)
	{
		spdlog::error("[gRPC Plugin] Failed to start gRPC server: {}", e.what());
		return PLUGIN_ERROR_NOT_INITIALIZED;
	}
}

extern "C" int32_t HandleRPCCalls(RPCHandler handler)
{
	if (handler == nullptr)
	{
		return PLUGIN_ERROR_NULL_PARAMETER;
	}

	if (g_rpc_handler == nullptr)
	{
		g_rpc_handler = handler;
	}

	// If gRPC is not enabled, no RPC calls to process
	if (!g_grpc_enabled)
	{
		return 0;
	}

	// Verify that StartRPCServer was called and server was created
	if (g_grpc_server == nullptr || g_completion_queue == nullptr)
	{
		spdlog::error("[gRPC Plugin] HandleRPCCalls called but gRPC server not initialized. StartRPCServer should be called first.");
		return PLUGIN_ERROR_NOT_INITIALIZED;
	}

	// Drain the completion queue non-blockingly
	// This processes pending gRPC requests without blocking
	void *tag;
	bool ok;
	int32_t events_processed = 0;

	while (true)
	{
		// Use AsyncNext with zero timeout for non-blocking operation
		auto next_status = g_completion_queue->AsyncNext(&tag, &ok, gpr_time_0(GPR_CLOCK_REALTIME));

		if (next_status == grpc::ServerCompletionQueue::SHUTDOWN)
		{
			// Server is shutting down
			break;
		}

		if (next_status == grpc::ServerCompletionQueue::TIMEOUT)
		{
			// No more events available right now
			break;
		}

		if (next_status == grpc::ServerCompletionQueue::GOT_EVENT)
		{
			// Process the event
			static_cast<CallBase *>(tag)->Proceed(ok);
			events_processed++;
		}
	}

	return events_processed;
}
