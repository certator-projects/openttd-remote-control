/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file handlers/script_goal.cpp RPC handlers for ScriptGoal methods */

#include "script_goal.h"
#include "../script_execution_context.h"

#include "../../../script/api/script_goal.hpp"
#include "../../../script/api/script_text.hpp"
#include "../../../script/api/script_company.hpp"
#include "../../../script/script_suspend.hpp"
#include "../../../game/game.hpp"

void HandleScriptGoal_New(const openttd::NewGoalRequest &request, openttd::NewGoalReply &response, openttd::GenericError *error)
{
	(void)error;
	ScriptExecutionContext ctx;
	if (!ctx.IsValid())
	{
		response.set_goal_id(::ScriptGoal::GOAL_INVALID.base());
		return;
	}

	::ScriptCompany::CompanyID company = request.goal_global()
											 ? ::ScriptCompany::COMPANY_INVALID
											 : static_cast<::ScriptCompany::CompanyID>(request.company());
	const std::string &goal_text = request.goal_text();
	::ScriptGoal::GoalType goal_type = static_cast<::ScriptGoal::GoalType>(request.goal_type());
	SQInteger destination = static_cast<SQInteger>(request.destination());

	GoalID goal_id = ::ScriptGoal::GOAL_INVALID;
	try
	{
		RawText *text = new RawText(goal_text);
		goal_id = ::ScriptGoal::New(company, text, goal_type, destination);
	}
	catch (Script_Suspend &e)
	{
		Game::GetInstance()->HandleScriptSuspend(e);
		goal_id = ::ScriptGoal::GOAL_INVALID;
	}

	response.set_goal_id(goal_id.base());
}
