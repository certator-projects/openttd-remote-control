/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file last_int_result.cpp Implementation of script result value capture. */

#include "last_int_result.h"

/**
 * Static storage for the last integer result from script operations.
 * This is used to capture values like GoalID that are returned asynchronously
 * via DoCommandReturn* callbacks and Squirrel::InsertResult.
 */
static int32_t g_last_int_result = 0;

void SetLastIntResult(int32_t result)
{
	g_last_int_result = result;
}

int32_t GetLastIntResult()
{
	return g_last_int_result;
}

void ClearLastIntResult()
{
	g_last_int_result = 0;
}
