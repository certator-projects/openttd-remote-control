/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file handlers/script_goal.h RPC handlers for ScriptGoal methods */

#ifndef ABI_RPC_PROXY_SCRIPT_GOAL_H
#define ABI_RPC_PROXY_SCRIPT_GOAL_H

#include "abi_rpc/script_goal.pb.h"
#include "abi_rpc/script_generic.pb.h"

void HandleScriptGoal_New(const openttd::NewGoalRequest &request, openttd::NewGoalReply &response, openttd::GenericError *error);

#endif /* ABI_RPC_PROXY_SCRIPT_GOAL_H */
