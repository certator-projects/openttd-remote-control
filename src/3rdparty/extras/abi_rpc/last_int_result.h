/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file last_int_result.h Interface for capturing script result values.
 *
 * This module provides a mechanism to capture the last integer result from
 * script operations like ScriptGoal::New that return IDs asynchronously
 * via the DoCommandReturn* callbacks.
 *
 * The value is set by Squirrel::InsertResult(int) and can be retrieved
 * via the ABI-RPC ScriptGeneric.GetLastIntResult method.
 */

#ifndef LAST_INT_RESULT_H
#define LAST_INT_RESULT_H

#include <cstdint>

/**
 * Store the last integer result from a script operation.
 * Called by Squirrel::InsertResult(int) to capture values like GoalID.
 * @param result The integer result to store.
 */
void SetLastIntResult(int32_t result);

/**
 * Get the last stored integer result.
 * @return The last integer result stored via SetLastIntResult.
 */
int32_t GetLastIntResult();

/**
 * Clear the last stored integer result (set to 0).
 */
void ClearLastIntResult();

#endif /* LAST_INT_RESULT_H */
