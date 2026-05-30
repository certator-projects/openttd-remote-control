/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file deferred_result.h Per-request deferred results for ABI-RPC script commands. */

#ifndef DEFERRED_RESULT_H
#define DEFERRED_RESULT_H

#include <cstdint>

enum class DeferredResultStatus : uint8_t
{
	Pending,
	Resolved,
	Failed,
	NotFound,
};

/**
 * Create a pending deferred result slot.
 * @return Non-zero deferred result id.
 */
uint32_t DeferredResult_Create();

/**
 * Resolve a deferred result with an integer value (e.g. GoalID).
 */
void DeferredResult_Resolve(uint32_t id, int32_t value);

/**
 * Mark a deferred result as failed.
 */
void DeferredResult_Fail(uint32_t id);

/**
 * Cancel a pending deferred result and remove it from the registry.
 */
void DeferredResult_Cancel(uint32_t id);

/**
 * Poll a deferred result without removing it from the registry.
 * @param id Deferred result id.
 * @param out_value Set when status is Resolved.
 * @return Current status.
 */
DeferredResultStatus DeferredResult_Poll(uint32_t id, int32_t *out_value);

/**
 * @return True when the deferred result is still pending.
 */
bool DeferredResult_IsPending(uint32_t id);

#endif /* DEFERRED_RESULT_H */
