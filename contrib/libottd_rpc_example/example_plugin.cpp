/*
 * Example plugin implementing the OpenTTD RPC interface.
 * This demonstrates how to create a plugin that:
 * - Registers with the host application
 * - Makes RPC calls to query game state
 * - Tracks game mode changes
 * - Outputs periodic status messages
 */

#include "plugin_interface.h"
#include "admin.pb.h"

#include "spdlog/spdlog.h"
#include <ctime>
#include <cstring>

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
};

/**
 * Last error state for plugin operations.
 */
static int32_t g_last_plugin_error = PLUGIN_SUCCESS;

// Plugin state
static HostOps g_host_ops = {};
static openttd::GameModeType g_last_mode = openttd::UNKNOWN;
static time_t g_last_idle_time = 0;
static bool initial_game_mode_reported = false;

static bool HostOpsReady()
{
	return g_host_ops.get_last_error != nullptr &&
		   g_host_ops.memory.create != nullptr &&
		   g_host_ops.memory.allocate != nullptr &&
		   g_host_ops.memory.deallocate != nullptr &&
		   g_host_ops.memory.release_all != nullptr &&
		   g_host_ops.memory.destroy != nullptr;
}

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
			return "Plugin: Null parameter provided";
		case PLUGIN_ERROR_NOT_INITIALIZED:
			return "Plugin: Not initialized";
		case PLUGIN_ERROR_RPC_FAILED:
			return "Plugin: RPC call failed";
		case PLUGIN_ERROR_MEMORY_FAILED:
			return "Plugin: Memory operation failed";
		default:
			snprintf(error_buffer, sizeof(error_buffer), "Plugin: Unknown error code: %d", error_code);
			return error_buffer;
	}
}

extern "C" void RegisterHostOps(const HostOps *ops)
{
	spdlog::info("[ExamplePlugin] RegisterHostOps called");
	if (ops == nullptr)
	{
		g_host_ops = {};
		return;
	}
	g_host_ops = *ops;
}

extern "C" int32_t StartRPCServer()
{
	spdlog::info("[ExamplePlugin] StartRPCServer called");

	if (!HostOpsReady())
	{
		spdlog::error("[ExamplePlugin] ERROR: Host operations not registered!");
		g_last_plugin_error = PLUGIN_ERROR_NOT_INITIALIZED;
		return PLUGIN_ERROR_NOT_INITIALIZED;
	}

	// Initialize idle timer
	g_last_idle_time = time(nullptr);

	g_last_plugin_error = PLUGIN_SUCCESS;
	return PLUGIN_SUCCESS;
}

extern "C" int32_t HandleRPCCalls(RPCHandler handler)
{
	if (!HostOpsReady() || handler == nullptr)
	{
		g_last_plugin_error = PLUGIN_ERROR_NULL_PARAMETER;
		return -1;
	}

	// Create a memory manager for this call
	ScopedMemoryManager *mem_mgr = nullptr;
	int32_t result = g_host_ops.memory.create(&mem_mgr);
	if (result != 0 || mem_mgr == nullptr)
	{
		g_last_plugin_error = PLUGIN_ERROR_MEMORY_FAILED;
		if (result != 0)
		{
			spdlog::error("[ExamplePlugin] Failed to create memory manager: {}",
						  g_host_ops.get_last_error(result));
		}
		return -1;
	}

	int32_t calls_processed = 0;

	// Make GetGameMode RPC call
	{
		openttd::GetGameModeRequest request;

		// Serialize request
		size_t request_size = request.ByteSizeLong();
		void *request_buffer = nullptr;
		result = g_host_ops.memory.allocate(mem_mgr, request_size, &request_buffer);
		if (result == 0)
		{
			request.SerializeToArray(request_buffer, request_size);

			void *response_buffer = nullptr;
			size_t response_size = 0;
			void *error_buffer = nullptr;
			size_t error_size = 0;
			result = handler(mem_mgr, RPC_GET_MODE, request_buffer, request_size,
							 &response_buffer, &response_size, &error_buffer, &error_size);

			if (result != 0)
			{
				g_last_plugin_error = PLUGIN_ERROR_RPC_FAILED;
				spdlog::error("[ExamplePlugin] RPC call failed: {}",
							  g_host_ops.get_last_error(result));
			}
			else {
				// Deserialize response
				openttd::GetGameModeReply response;
				if (response.ParseFromArray(response_buffer, response_size))
				{
					// Check if mode changed
					if (response.current_mode() != g_last_mode || !initial_game_mode_reported)
					{
						initial_game_mode_reported = true;
						std::string mode_message = "[ExamplePlugin] Game mode changed: ";

						// Output mode name
						switch (response.current_mode())
						{
							case openttd::NEW_GAME_MENU:
								mode_message += "NEW_GAME_MENU";
								break;
							case openttd::SCENARIO_EDITOR:
								mode_message += "SCENARIO_EDITOR";
								break;
							case openttd::HEIGHTMAP_EDITOR:
								mode_message += "HEIGHTMAP_EDITOR";
								break;
							case openttd::SINGLE_GAME:
								mode_message += "SINGLE_GAME";
								break;
							case openttd::MULTIPLAYER_CLIENT:
								mode_message += "MULTIPLAYER_CLIENT";
								break;
							case openttd::MULTIPLAYER_SERVER:
								mode_message += "MULTIPLAYER_SERVER";
								break;
							default:
								mode_message += "UNKNOWN";
								break;
						}

						if (response.is_dedicated())
						{
							mode_message += " (dedicated server)";
						}

						if (!response.server_name().empty())
						{
							mode_message += " - Server: " + response.server_name();
						}

						spdlog::info(mode_message);

						g_last_mode = response.current_mode();
					}

					calls_processed++;
				}
			}
		}
		else {
			g_last_plugin_error = PLUGIN_ERROR_MEMORY_FAILED;
			spdlog::error("[ExamplePlugin] Failed to allocate memory: {}",
						  g_host_ops.get_last_error(result));
		}
	}

	// Check if we should output idle message (every 5 seconds)
	time_t current_time = time(nullptr);
	if (current_time - g_last_idle_time >= 5)
	{
		spdlog::info("[ExamplePlugin] Plugin idle");
		g_last_idle_time = current_time;
	}

	// Clean up memory
	g_host_ops.memory.destroy(mem_mgr);

	g_last_plugin_error = PLUGIN_SUCCESS;
	return calls_processed;
}
