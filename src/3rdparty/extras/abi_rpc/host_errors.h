/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file host_errors.h Host-side RPC error codes and descriptions. */


#ifndef ABI_RPC_HOST_ERRORS_H
#define ABI_RPC_HOST_ERRORS_H

#include <stdint.h>

/**
 * Error codes for memory manager operations.
 */
enum MemoryManagerError
{
	MEM_SUCCESS = 0,
	MEM_ERROR_NULL_PARAMETER = 1,
	MEM_ERROR_ALLOCATION_FAILED = 2,
	MEM_ERROR_OUT_OF_MEMORY = 3,
};


/**
 * RPC handler error codes.
 */
enum RPCHandlerError
{
	RPC_SUCCESS = 0,
	RPC_ERROR_NULL_PARAMETER = 10,
	RPC_ERROR_PARSE_FAILED = 11,
	RPC_ERROR_SERIALIZE_FAILED = 12,
	RPC_ERROR_ALLOCATION_FAILED = 13,
	RPC_ERROR_UNKNOWN_METHOD = 14,
};

#ifdef __cplusplus
extern "C"
{
#endif

	const char *HostGetLastError(int32_t error_code);

#ifdef __cplusplus
}
#endif

#endif /* ABI_RPC_HOST_ERRORS_H */
