/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file deferred_result.cpp Implementation of per-request deferred results for ABI-RPC. */

#include "deferred_result.h"
#include "deferred_result_hook.h"

#include "../../../script/api/script_object.hpp"

#include <atomic>
#include <mutex>
#include <unordered_map>

struct DeferredResultEntry
{
	DeferredResultStatus status = DeferredResultStatus::Pending;
	int32_t value = 0;
};

static std::mutex g_deferred_mutex;
static std::unordered_map<uint32_t, DeferredResultEntry> g_deferred_results;
static std::atomic<uint32_t> g_next_deferred_id{1};

uint32_t DeferredResult_Create()
{
	const uint32_t id = g_next_deferred_id.fetch_add(1, std::memory_order_relaxed);
	std::lock_guard lock(g_deferred_mutex);
	g_deferred_results[id] = DeferredResultEntry{};
	return id;
}

void DeferredResult_Resolve(uint32_t id, int32_t value)
{
	if (id == 0) return;

	std::lock_guard lock(g_deferred_mutex);
	auto it = g_deferred_results.find(id);
	if (it == g_deferred_results.end()) return;

	it->second.status = DeferredResultStatus::Resolved;
	it->second.value = value;
}

void DeferredResult_Fail(uint32_t id)
{
	if (id == 0) return;

	std::lock_guard lock(g_deferred_mutex);
	auto it = g_deferred_results.find(id);
	if (it == g_deferred_results.end()) return;

	it->second.status = DeferredResultStatus::Failed;
}

void DeferredResult_Cancel(uint32_t id)
{
	if (id == 0) return;

	std::lock_guard lock(g_deferred_mutex);
	g_deferred_results.erase(id);
}

DeferredResultStatus DeferredResult_Poll(uint32_t id, int32_t *out_value)
{
	if (id == 0) return DeferredResultStatus::NotFound;

	std::lock_guard lock(g_deferred_mutex);
	auto it = g_deferred_results.find(id);
	if (it == g_deferred_results.end()) return DeferredResultStatus::NotFound;

	if (out_value != nullptr && it->second.status == DeferredResultStatus::Resolved)
	{
		*out_value = it->second.value;
	}
	return it->second.status;
}

bool DeferredResult_IsPending(uint32_t id)
{
	return DeferredResult_Poll(id, nullptr) == DeferredResultStatus::Pending;
}

void AbiRpc_OnScriptInsertResult(int32_t result)
{
	if (!ScriptObject::HasActiveInstance()) return;

	const uint32_t id = ScriptObject::ConsumePendingAbiDeferredId();
	if (id == 0) return;

	DeferredResult_Resolve(id, result);
}

void AbiRpc_OnScriptCommandFailed()
{
	if (!ScriptObject::HasActiveInstance()) return;

	const uint32_t id = ScriptObject::ConsumePendingAbiDeferredId();
	if (id == 0) return;

	DeferredResult_Fail(id);
}
