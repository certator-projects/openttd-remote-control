/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file rpc_handler.h RPC handler for plugin interface. */

#ifndef RPC_HANDLER_H
#define RPC_HANDLER_H

#include "plugin_interface.h"

/**
 * Initialize the RPC handler system.
 */
void RPCHandler_Initialize();

/**
 * Host RPC handler: routes calls by method ID; may return GenericError.
 * @param memory_manager Memory manager to use for allocations.
 * @param rpc_method_id Integer identifying the RPC method to call.
 * @param request Pointer to serialized protobuf request data.
 * @param request_size Size of the request data in bytes.
 * @param out_response Output parameter for pointer to serialized protobuf response data.
 * @param out_response_size Output parameter for the size of the response.
 * @param out_error Output for serialized GenericError (nullptr if none).
 * @param out_error_size Output for GenericError size (0 if none).
 * @return 0 on success, non-zero error code on failure.
 */
int32_t RPCHandler_Handle(
	ScopedMemoryManager *memory_manager,
	int32_t rpc_method_id,
	const void *request,
	size_t request_size,
	void **out_response,
	size_t *out_response_size,
	void **out_error,
	size_t *out_error_size);

#endif /* RPC_HANDLER_H */
