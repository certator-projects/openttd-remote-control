/*

* =============================================================================
*  OpenTTD ABI interface definitions
* =============================================================================

MIT License

Copyright 2026 @certator

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files
(the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software,
and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

* =============================================================================

*/

#ifndef PLUGIN_INTERFACE_H
#define PLUGIN_INTERFACE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Current API version.
 * Increment this when making breaking changes to the plugin interface.
 */
#define PLUGIN_API_VERSION 1

	/**
 * Opaque handle to a scoped memory manager instance.
 */
	typedef struct ScopedMemoryManager ScopedMemoryManager;

	/**
 * Error string retrieval function type.
 * Returns a human-readable error description for a given error code.
 * @param error_code The error code to describe.
 * @return Pointer to a static string describing the error. Never returns nullptr.
 */
	typedef const char *(*GetLastErrorFunc)(int32_t error_code);

	/**
 * Host memory manager operation function types (vtable).
 * Plugins must not link against host ScopedMemoryManager_* symbols.
 */
	typedef int32_t (*HostMemoryCreateFn)(ScopedMemoryManager **out_manager);
	typedef int32_t (*HostMemoryAllocateFn)(ScopedMemoryManager *manager, size_t size, void **out_ptr);
	typedef void (*HostMemoryDeallocateFn)(ScopedMemoryManager *manager, void *ptr);
	typedef void (*HostMemoryReleaseAllFn)(ScopedMemoryManager *manager);
	typedef void (*HostMemoryDestroyFn)(ScopedMemoryManager *manager);

	/**
 * Host scoped memory manager operations.
 */
	typedef struct HostMemoryOps
	{
		HostMemoryCreateFn create;
		HostMemoryAllocateFn allocate;
		HostMemoryDeallocateFn deallocate;
		HostMemoryReleaseAllFn release_all;
		HostMemoryDestroyFn destroy;
	} HostMemoryOps;

	/**
 * Host services provided to the plugin at load time.
 * The plugin copies this table; all host interaction uses these pointers.
 */
	typedef struct HostOps
	{
		GetLastErrorFunc get_last_error;
		HostMemoryOps memory;
	} HostOps;

	/**
 * Host RPC handler invoked by the plugin.
 * @param memory_manager Memory manager to use for allocations.
 * @param rpc_method_id Integer identifying the RPC method to call.
 * @param request Pointer to serialized protobuf request data.
 * @param request_size Size of the request data in bytes.
 * @param out_response Output parameter for pointer to serialized protobuf response data.
 * @param out_response_size Output parameter for the size of the response.
 * @param out_error Output for serialized GenericError (set to nullptr when none).
 * @param out_error_size Output for GenericError size (set to 0 when none).
 * @return 0 on success, non-zero error code on failure.
 */
	typedef int32_t (*RPCHandler)(
		ScopedMemoryManager *memory_manager,
		int32_t rpc_method_id,
		const void *request,
		size_t request_size,
		void **out_response,
		size_t *out_response_size,
		void **out_error,
		size_t *out_error_size);

	/**
 * RPC method IDs.
 */
	enum RPCMethodID
	{
		RPC_GET_MODE = 1, ///< Get current OpenTTD mode

		// Network / Admin (3-9)
		RPC_START_NETWORK_SERVER = 3, ///< Start multiplayer server
		RPC_GET_NETWORK_CLIENTS = 5, ///< Get list of connected clients
		RPC_SEND_CHAT_MESSAGE = 6, ///< Broadcast chat message
		RPC_SEND_PRIVATE_CHAT_MESSAGE = 7, ///< Send private chat message
		RPC_GET_COMPANY_LIST = 8, ///< Get list of companies
		RPC_POLL_LAST_CHAT_MESSAGES = 9, ///< Drain recent chat notifications from OpenTTD

		// Console service — game console–backed RPCs (1000-1199)
		RPC_RESET_COMPANY = 1000, ///< Remove an idle company (reset_company)
		RPC_SETTING_NEWGAME = 1001, ///< Get/set a setting for the next game (setting_newgame)
		RPC_RESTART = 1002, ///< Restart game (restart)
		RPC_SETTING = 1003, ///< Get/set a setting for the current game (setting)

		// ScriptCompany methods (200-249)
		RPC_SCRIPTCOMPANY_GET_QUARTERLY_COMPANY_VALUE = 213,

		// ScriptGoal methods (400-419)
		RPC_SCRIPTGOAL_NEW = 402,

		// ScriptMap methods (700-719)
		RPC_SCRIPTMAP_GET_MAP_SIZE_X = 702,
		RPC_SCRIPTMAP_GET_MAP_SIZE_Y = 703,
		RPC_SCRIPTMAP_GET_TILE_INDEX = 706,

		// ScriptGeneric methods (800-819)
		RPC_SCRIPTGENERIC_GET_LAST_INT_RESULT = 800,
	};

	/**
 * Plugin interface functions.
 * These must be exported by the plugin library.
 */

	/**
 * Get the API version supported by the plugin.
 * @return The API version number.
 */
	int32_t GetAPIVersion();

	/**
 * Get error description from plugin.
 * Returns a human-readable error message for a given error code.
 * @param error_code The error code to describe.
 * @return Pointer to a static string describing the error. Never returns nullptr.
 */
	const char *PluginGetLastError(int32_t error_code);

	/**
 * Register host operations (error strings, memory manager, …).
 * Called once during plugin load. The plugin copies the table.
 * @param ops Host-provided operations (valid until copy completes).
 */
	void RegisterHostOps(const HostOps *ops);

	/**
 * Start the RPC server.
 * Called by the host to initialize the plugin's RPC handling.
 * @return 0 on success, non-zero on error.
 */
	int32_t StartRPCServer();

	/**
 * Handle pending RPC calls.
 * Called periodically by the host to process any pending RPC requests.
 * @param handler Host RPC handler (response + optional GenericError).
 * @return Number of RPC calls processed, or negative on error.
 */
	int32_t HandleRPCCalls(RPCHandler handler);

#ifdef __cplusplus
}
#endif

#endif /* PLUGIN_INTERFACE_H */
