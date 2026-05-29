/*

* =============================================================================
*  OpenTTD ABI scoped memory manager interface
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

#ifndef SCOPED_MEMORY_MANAGER_H
#define SCOPED_MEMORY_MANAGER_H

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

	/**
 * Opaque handle to a scoped memory manager instance.
 */
	typedef struct ScopedMemoryManager ScopedMemoryManager;

	/**
 * Create a new scoped memory manager.
 * @param out_manager Output parameter for the created memory manager.
 * @return 0 on success, non-zero error code on failure.
 */
	int32_t ScopedMemoryManager_Create(ScopedMemoryManager **out_manager);

	/**
 * Allocate memory using the scoped memory manager.
 * @param manager The memory manager instance.
 * @param size Number of bytes to allocate.
 * @param out_ptr Output parameter for the allocated memory pointer.
 * @return 0 on success, non-zero error code on failure. Returns 0 with nullptr
 * when size is 0.
 */
	int32_t ScopedMemoryManager_Allocate(ScopedMemoryManager *manager, size_t size,
										 void **out_ptr);

	/**
 * Deallocate specific memory allocated by this manager.
 * @param manager The memory manager instance.
 * @param ptr Pointer to memory to deallocate.
 */
	void ScopedMemoryManager_Deallocate(ScopedMemoryManager *manager, void *ptr);

	/**
 * Release all memory allocated by this manager.
 * @param manager The memory manager instance.
 */
	void ScopedMemoryManager_ReleaseAll(ScopedMemoryManager *manager);

	/**
 * Destroy the scoped memory manager and release all its memory.
 * @param manager The memory manager instance to destroy.
 */
	void ScopedMemoryManager_Destroy(ScopedMemoryManager *manager);

#ifdef __cplusplus
}
#endif

#endif /* SCOPED_MEMORY_MANAGER_H */
