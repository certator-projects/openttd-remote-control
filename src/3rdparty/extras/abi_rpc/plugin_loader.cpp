/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file plugin_loader.cpp Implementation of plugin dynamic library loader. */

#include "plugin_loader.h"
#include "plugin_interface.h"
#include "host_errors.h"
#include "scoped_memory_manager.h"
#include "rpc_handler.h"

#include "../../../stdafx.h"
#include "../../../debug.h"

#include <cstdlib>
#include <cstring>

// Platform-specific dynamic library loading
#ifdef _WIN32
#	include <windows.h>
typedef HMODULE LibraryHandle;
#	define LOAD_LIBRARY(path) LoadLibraryA(path)
#	define GET_SYMBOL(handle, name) GetProcAddress(handle, name)
#	define CLOSE_LIBRARY(handle) FreeLibrary(handle)
#	define LIBRARY_ERROR() "Windows LoadLibrary failed"
#else
#	include <dlfcn.h>
typedef void *LibraryHandle;
#	define LOAD_LIBRARY(path) dlopen(path, RTLD_LAZY)
#	define GET_SYMBOL(handle, name) dlsym(handle, name)
#	define CLOSE_LIBRARY(handle) dlclose(handle)
#	define LIBRARY_ERROR() dlerror()
#endif

// Plugin function pointers
typedef int32_t (*GetAPIVersionFunc)();
typedef const char *(*GetLastErrorFunc)(int32_t);
typedef void (*RegisterHostOpsFunc)(const HostOps *);
typedef int32_t (*StartRPCServerFunc)();
typedef int32_t (*HandleRPCCallsFunc)(RPCHandler);

// Plugin state
struct PluginState
{
	LibraryHandle library_handle = nullptr;
	GetAPIVersionFunc get_api_version = nullptr;
	GetLastErrorFunc plugin_get_last_error = nullptr;
	RegisterHostOpsFunc register_host_ops = nullptr;
	StartRPCServerFunc start_rpc_server = nullptr;
	HandleRPCCallsFunc handle_rpc_calls = nullptr;
	bool initialized = false;
	bool started = false;
};

static PluginState g_plugin_state;

static const HostOps k_host_ops = {
	&HostGetLastError,
	{
		&ScopedMemoryManager_Create,
		&ScopedMemoryManager_Allocate,
		&ScopedMemoryManager_Deallocate,
		&ScopedMemoryManager_ReleaseAll,
		&ScopedMemoryManager_Destroy,
	},
};

bool PluginLoader_Initialize()
{
	// Check if plugin path is specified
	const char *plugin_path = std::getenv("OTTD_USE_RPC_PLUGIN");
	if (plugin_path == nullptr || plugin_path[0] == '\0')
	{
		Debug(driver, 1, "No RPC plugin specified (OTTD_USE_RPC_PLUGIN not set)");
		return true; // Not an error, just no plugin to load
	}

	Debug(driver, 1, "Loading RPC plugin: {}", plugin_path);

	// Load the dynamic library
	g_plugin_state.library_handle = LOAD_LIBRARY(plugin_path);
	if (g_plugin_state.library_handle == nullptr)
	{
		Debug(driver, 0, "Failed to load plugin library: {}", LIBRARY_ERROR());
		return false;
	}

	// Look up plugin functions
	g_plugin_state.get_api_version =
		reinterpret_cast<GetAPIVersionFunc>(GET_SYMBOL(g_plugin_state.library_handle, "GetAPIVersion"));
	g_plugin_state.plugin_get_last_error =
		reinterpret_cast<GetLastErrorFunc>(GET_SYMBOL(g_plugin_state.library_handle, "PluginGetLastError"));
	g_plugin_state.register_host_ops =
		reinterpret_cast<RegisterHostOpsFunc>(GET_SYMBOL(g_plugin_state.library_handle, "RegisterHostOps"));
	g_plugin_state.start_rpc_server =
		reinterpret_cast<StartRPCServerFunc>(GET_SYMBOL(g_plugin_state.library_handle, "StartRPCServer"));
	g_plugin_state.handle_rpc_calls =
		reinterpret_cast<HandleRPCCallsFunc>(GET_SYMBOL(g_plugin_state.library_handle, "HandleRPCCalls"));

	// Verify all required functions are present
	if (g_plugin_state.get_api_version == nullptr ||
		g_plugin_state.plugin_get_last_error == nullptr ||
		g_plugin_state.register_host_ops == nullptr ||
		g_plugin_state.start_rpc_server == nullptr ||
		g_plugin_state.handle_rpc_calls == nullptr)
	{
		Debug(driver, 0, "Plugin missing required functions");
		CLOSE_LIBRARY(g_plugin_state.library_handle);
		g_plugin_state.library_handle = nullptr;
		return false;
	}

	// Check API version
	int32_t plugin_version = g_plugin_state.get_api_version();
	if (plugin_version != PLUGIN_API_VERSION)
	{
		Debug(driver, 0, "Plugin API version mismatch: expected {}, got {}",
			  PLUGIN_API_VERSION, plugin_version);
		CLOSE_LIBRARY(g_plugin_state.library_handle);
		g_plugin_state.library_handle = nullptr;
		return false;
	}

	g_plugin_state.register_host_ops(&k_host_ops);

	g_plugin_state.initialized = true;
	Debug(driver, 1, "RPC plugin loaded successfully (API version {})", plugin_version);

	return true;
}

bool PluginLoader_Start()
{
	if (!g_plugin_state.initialized)
	{
		return true; // No plugin loaded, not an error
	}

	Debug(driver, 1, "Starting RPC plugin server");

	// Initialize RPC handler system
	RPCHandler_Initialize();

	// Start the plugin's RPC server
	int32_t result = g_plugin_state.start_rpc_server();
	if (result != 0)
	{
		Debug(driver, 0, "Plugin StartRPCServer failed with error code: {}", result);
		return false;
	}

	g_plugin_state.started = true;
	Debug(driver, 1, "RPC plugin server started successfully");

	return true;
}

void PluginLoader_HandleRPCCalls()
{
	if (!g_plugin_state.started)
	{
		return; // No plugin loaded or not started
	}

	int32_t result = g_plugin_state.handle_rpc_calls(RPCHandler_Handle);

	if (result < 0)
	{
		Debug(driver, 0, "Plugin HandleRPCCalls returned error: {}", result);
	}
}

void PluginLoader_Shutdown()
{
	if (!g_plugin_state.initialized)
	{
		return; // No plugin loaded
	}

	Debug(driver, 1, "Shutting down RPC plugin");

	g_plugin_state.started = false;
	g_plugin_state.initialized = false;

	// Unload the library
	if (g_plugin_state.library_handle != nullptr)
	{
		CLOSE_LIBRARY(g_plugin_state.library_handle);
		g_plugin_state.library_handle = nullptr;
	}

	// Clear function pointers
	g_plugin_state.get_api_version = nullptr;
	g_plugin_state.plugin_get_last_error = nullptr;
	g_plugin_state.register_host_ops = nullptr;
	g_plugin_state.start_rpc_server = nullptr;
	g_plugin_state.handle_rpc_calls = nullptr;

	Debug(driver, 1, "RPC plugin unloaded");
}

bool PluginLoader_IsLoaded()
{
	return g_plugin_state.initialized;
}
