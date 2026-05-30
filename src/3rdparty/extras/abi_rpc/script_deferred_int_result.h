/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file script_deferred_int_result.h RAII helper for ABI-RPC deferred script int results. */

#ifndef SCRIPT_DEFERRED_INT_RESULT_H
#define SCRIPT_DEFERRED_INT_RESULT_H

#include "deferred_result.h"
#include "script_execution_context.h"

#include "../../../script/api/script_object.hpp"

/**
 * Binds a deferred ABI-RPC result to the next DoCommand int callback on the active
 * script instance. Matches native one-in-flight DoCommand semantics.
 */
class ScriptDeferredIntResult
{
public:
	explicit ScriptDeferredIntResult(const ScriptExecutionContext &ctx)
	{
		if (!ctx.IsValid()) return;

		this->id_ = DeferredResult_Create();
		ScriptObject::SetPendingAbiDeferredId(this->id_);
		this->active_ = true;
	}

	~ScriptDeferredIntResult()
	{
		/* Leave ScriptStorage binding in place when still pending; the DoCommand
		 * callback consumes it when the command completes. */
	}

	ScriptDeferredIntResult(const ScriptDeferredIntResult &) = delete;
	ScriptDeferredIntResult &operator=(const ScriptDeferredIntResult &) = delete;

	uint32_t Id() const { return this->id_; }

	bool IsActive() const { return this->active_; }

	bool IsPending() const
	{
		return this->active_ && DeferredResult_IsPending(this->id_);
	}

	bool TryGetResolvedValue(int32_t &value) const
	{
		if (!this->active_) return false;
		return DeferredResult_Poll(this->id_, &value) == DeferredResultStatus::Resolved;
	}

	void Cancel()
	{
		if (!this->active_) return;

		ScriptObject::ClearPendingAbiDeferredIdIf(this->id_);
		DeferredResult_Cancel(this->id_);
		this->active_ = false;
		this->id_ = 0;
	}

private:
	uint32_t id_ = 0;
	bool active_ = false;
};

#endif /* SCRIPT_DEFERRED_INT_RESULT_H */
