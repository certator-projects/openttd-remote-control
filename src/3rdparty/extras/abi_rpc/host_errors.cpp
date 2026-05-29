/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file host_errors.cpp Implementation of host-side RPC error descriptions. */

#include "host_errors.h"
#include <vector>
#include <cstdlib>
#include <cstring>
#include <cstdio>

/**
 * Last error state for memory manager operations.
 */
static thread_local int32_t g_last_memory_error = MEM_SUCCESS;

/**
 * Get error description for host error codes.
 * This function handles error codes from all host subsystems (memory, RPC, etc).
 * @param error_code The error code to describe.
 * @return Pointer to a static string describing the error.
 */
extern "C" const char *HostGetLastError(int32_t error_code)
{
	static char error_buffer[256];

	// Memory manager errors (1-9)
	if (error_code >= MEM_ERROR_NULL_PARAMETER && error_code <= MEM_ERROR_OUT_OF_MEMORY)
	{
		switch (error_code)
		{
			case MEM_ERROR_NULL_PARAMETER:
				return "Memory: Null parameter provided";
			case MEM_ERROR_ALLOCATION_FAILED:
				return "Memory: Allocation failed";
			case MEM_ERROR_OUT_OF_MEMORY:
				return "Memory: Out of memory";
		}
	}

	// RPC handler errors (10-19)
	if (error_code >= 10 && error_code <= 19)
	{
		switch (error_code)
		{
			case 10: // RPC_ERROR_NULL_PARAMETER
				return "RPC: Null parameter provided";
			case 11: // RPC_ERROR_PARSE_FAILED
				return "RPC: Failed to parse request";
			case 12: // RPC_ERROR_SERIALIZE_FAILED
				return "RPC: Failed to serialize response";
			case 13: // RPC_ERROR_ALLOCATION_FAILED
				return "RPC: Memory allocation failed";
			case 14: // RPC_ERROR_UNKNOWN_METHOD
				return "RPC: Unknown method ID";
		}
	}

	if (error_code == 0)
	{
		return "Success";
	}

	snprintf(error_buffer, sizeof(error_buffer), "Unknown error code: %d", error_code);
	return error_buffer;
}
