/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file handlers/script_generic.h RPC handler declarations for ScriptGeneric methods */

#ifndef ABI_RPC_HANDLERS_SCRIPT_GENERIC_H
#define ABI_RPC_HANDLERS_SCRIPT_GENERIC_H

#include "abi_rpc/script_generic.pb.h"

/**
 * Handle ScriptGeneric_GetLastIntResult RPC request.
 * Returns the last integer result from a script operation stored via Squirrel::InsertResult(int).
 * @param request Deserialized request (empty).
 * @param response Response to fill with the last int result.
 */
void HandleScriptGeneric_GetLastIntResult(const openttd::GetLastIntResultRequest &request, openttd::GetLastIntResultReply &response, openttd::GenericError *error);

#endif /* ABI_RPC_HANDLERS_SCRIPT_GENERIC_H */
