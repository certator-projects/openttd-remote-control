/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file scoped_memory_manager.cpp Implementation of scoped memory manager. */

#include "scoped_memory_manager.h"
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
 * Internal implementation of the scoped memory manager.
 */
struct ScopedMemoryManager
{
	std::vector<void *> allocations; ///< List of all allocations made by this manager.
};

/**
 * Create a new scoped memory manager.
 * @param out_manager Output parameter for the created memory manager.
 * @return 0 on success, non-zero error code on failure.
 */
extern "C" int32_t ScopedMemoryManager_Create(ScopedMemoryManager **out_manager)
{
	if (out_manager == nullptr)
	{
		g_last_memory_error = MEM_ERROR_NULL_PARAMETER;
		return MEM_ERROR_NULL_PARAMETER;
	}

	try
	{
		*out_manager = new ScopedMemoryManager();
		g_last_memory_error = MEM_SUCCESS;
		return MEM_SUCCESS;
	}
	catch (...)
	{
		*out_manager = nullptr;
		g_last_memory_error = MEM_ERROR_OUT_OF_MEMORY;
		return MEM_ERROR_OUT_OF_MEMORY;
	}
}

/**
 * Allocate memory using the scoped memory manager.
 * @param manager The memory manager instance.
 * @param size Number of bytes to allocate.
 * @param out_ptr Output parameter for the allocated memory pointer.
 * @return 0 on success, non-zero error code on failure.
 */
extern "C" int32_t ScopedMemoryManager_Allocate(ScopedMemoryManager *manager, size_t size, void **out_ptr)
{
	if (out_ptr == nullptr)
	{
		g_last_memory_error = MEM_ERROR_NULL_PARAMETER;
		return MEM_ERROR_NULL_PARAMETER;
	}

	if (manager == nullptr)
	{
		*out_ptr = nullptr;
		g_last_memory_error = MEM_ERROR_NULL_PARAMETER;
		return MEM_ERROR_NULL_PARAMETER;
	}

	// Special case: allocating 0 bytes is valid and returns nullptr
	if (size == 0)
	{
		*out_ptr = nullptr;
		g_last_memory_error = MEM_SUCCESS;
		return MEM_SUCCESS;
	}

	void *ptr = std::malloc(size);
	if (ptr == nullptr)
	{
		*out_ptr = nullptr;
		g_last_memory_error = MEM_ERROR_ALLOCATION_FAILED;
		return MEM_ERROR_ALLOCATION_FAILED;
	}

	try
	{
		manager->allocations.push_back(ptr);
		*out_ptr = ptr;
		g_last_memory_error = MEM_SUCCESS;
		return MEM_SUCCESS;
	}
	catch (...)
	{
		std::free(ptr);
		*out_ptr = nullptr;
		g_last_memory_error = MEM_ERROR_OUT_OF_MEMORY;
		return MEM_ERROR_OUT_OF_MEMORY;
	}
}

/**
 * Deallocate specific memory allocated by this manager.
 * @param manager The memory manager instance.
 * @param ptr Pointer to memory to deallocate.
 */
extern "C" void ScopedMemoryManager_Deallocate(ScopedMemoryManager *manager, void *ptr)
{
	if (manager == nullptr || ptr == nullptr) return;

	auto it = std::find(manager->allocations.begin(), manager->allocations.end(), ptr);
	if (it != manager->allocations.end())
	{
		manager->allocations.erase(it);
		std::free(ptr);
	}
}

/**
 * Release all memory allocated by this manager.
 * @param manager The memory manager instance.
 */
extern "C" void ScopedMemoryManager_ReleaseAll(ScopedMemoryManager *manager)
{
	if (manager == nullptr) return;

	for (void *ptr : manager->allocations)
	{
		std::free(ptr);
	}
	manager->allocations.clear();
}

/**
 * Destroy the scoped memory manager and release all its memory.
 * @param manager The memory manager instance to destroy.
 */
extern "C" void ScopedMemoryManager_Destroy(ScopedMemoryManager *manager)
{
	if (manager == nullptr) return;

	ScopedMemoryManager_ReleaseAll(manager);
	delete manager;
}
