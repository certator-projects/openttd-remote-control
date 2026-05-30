/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file deferred_result_hook.h Hooks from script DoCommand callbacks into ABI-RPC deferred results. */

#ifndef DEFERRED_RESULT_HOOK_H
#define DEFERRED_RESULT_HOOK_H

#include <cstdint>

#ifdef __cplusplus
extern "C"
{
#endif

	/**
 * Called from Squirrel::InsertResult(int) when a DoCommand callback delivers an int.
 * Resolves a pending ABI-RPC deferred result bound in ScriptStorage, if any.
 */
	void AbiRpc_OnScriptInsertResult(int32_t result);

	/**
 * Called when a script DoCommand completes with failure while a deferred result is pending.
 */
	void AbiRpc_OnScriptCommandFailed();

#ifdef __cplusplus
}
#endif

#endif /* DEFERRED_RESULT_HOOK_H */
