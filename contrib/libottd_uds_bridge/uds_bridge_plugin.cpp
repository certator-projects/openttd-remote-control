/*
 * UDS IPC bridge plugin for OpenTTD.
 * Exposes the ABI RPC interface over a local Unix domain socket so that
 * out-of-process services (for example the gRPC server executable) can
 * call into OpenTTD without linking Apache-2.0 code into the game process.
 */

#include "plugin_interface.h"
#include "uds_bridge_server.hpp"

#include <spdlog/spdlog.h>

#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>

enum PluginError
{
	PLUGIN_SUCCESS = 0,
	PLUGIN_ERROR_NULL_PARAMETER = 100,
	PLUGIN_ERROR_NOT_INITIALIZED = 101,
	PLUGIN_ERROR_RPC_FAILED = 102,
	PLUGIN_ERROR_MEMORY_FAILED = 103,
	PLUGIN_ERROR_BRIDGE_DISABLED = 104,
};

static HostOps g_host_ops = {};
static std::unique_ptr<UdsBridgeServer> g_bridge_server = nullptr;
static bool g_bridge_enabled = false;

static bool HostOpsReady()
{
	return g_host_ops.get_last_error != nullptr &&
		   g_host_ops.memory.create != nullptr &&
		   g_host_ops.memory.allocate != nullptr &&
		   g_host_ops.memory.deallocate != nullptr &&
		   g_host_ops.memory.release_all != nullptr &&
		   g_host_ops.memory.destroy != nullptr &&
		   g_host_ops.handle_rpc != nullptr;
}

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
			return "UDS Bridge Plugin: Null parameter provided";
		case PLUGIN_ERROR_NOT_INITIALIZED:
			return "UDS Bridge Plugin: Not initialized";
		case PLUGIN_ERROR_RPC_FAILED:
			return "UDS Bridge Plugin: RPC call failed";
		case PLUGIN_ERROR_MEMORY_FAILED:
			return "UDS Bridge Plugin: Memory operation failed";
		case PLUGIN_ERROR_BRIDGE_DISABLED:
			return "UDS Bridge Plugin: Bridge disabled via OTTD_UDS_BRIDGE_ENABLED";
		default:
			snprintf(error_buffer, sizeof(error_buffer),
					 "UDS Bridge Plugin: Unknown error code: %d", error_code);
			return error_buffer;
	}
}

extern "C" void RegisterHostOps(const HostOps *ops)
{
	spdlog::info("[UDS Bridge] RegisterHostOps called");
	if (ops == nullptr)
	{
		g_host_ops = {};
		return;
	}
	g_host_ops = *ops;
}

extern "C" int32_t StartRPCServer()
{
	spdlog::info("[UDS Bridge] StartRPCServer called");

	if (!HostOpsReady())
	{
		spdlog::error("[UDS Bridge] ERROR: Host operations not registered!");
		return PLUGIN_ERROR_NOT_INITIALIZED;
	}

	const char *bridge_enabled_env = std::getenv("OTTD_UDS_BRIDGE_ENABLED");
	if (bridge_enabled_env == nullptr ||
		(strcmp(bridge_enabled_env, "1") != 0 && strcmp(bridge_enabled_env, "true") != 0))
	{
		spdlog::info("[UDS Bridge] Bridge disabled (OTTD_UDS_BRIDGE_ENABLED not set to 1 or true)");
		g_bridge_enabled = false;
		return PLUGIN_SUCCESS;
	}

	const char *socket_path_env = std::getenv("OTTD_UDS_SOCKET_PATH");
	std::string socket_path = socket_path_env != nullptr ? socket_path_env : "/run/openttd/rpc.sock";

	g_bridge_server = std::make_unique<UdsBridgeServer>();
	if (!g_bridge_server->Start(socket_path))
	{
		g_bridge_server.reset();
		return PLUGIN_ERROR_NOT_INITIALIZED;
	}

	g_bridge_enabled = true;
	return PLUGIN_SUCCESS;
}

extern "C" int32_t HandleRPCCalls()
{
	if (!HostOpsReady())
	{
		return PLUGIN_ERROR_NULL_PARAMETER;
	}

	if (!g_bridge_enabled || g_bridge_server == nullptr)
	{
		return 0;
	}

	return g_bridge_server->Poll(g_host_ops);
}
