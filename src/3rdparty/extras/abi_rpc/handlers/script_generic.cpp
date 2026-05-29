/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file handlers/script_generic.cpp RPC handlers for ScriptGeneric methods */

#include "script_generic.h"
#include "../last_int_result.h"

/**
 * Handle ScriptGeneric_GetLastIntResult RPC request.
 * Returns the last integer result from a script operation stored via Squirrel::InsertResult(int).
 *
 * This is used to retrieve IDs from asynchronous script operations like ScriptGoal::New
 * that don't return the created ID directly but instead queue a command and return
 * the result via a callback to Squirrel::InsertResult.
 *
 * @param request Deserialized request (empty, no parameters needed).
 * @param response Response to fill with the last int result.
 */
void HandleScriptGeneric_GetLastIntResult(const openttd::GetLastIntResultRequest &request, openttd::GetLastIntResultReply &response, openttd::GenericError *error)
{
	(void)request; // Unused - request has no parameters
	(void)error;
	response.set_result(GetLastIntResult());
}
