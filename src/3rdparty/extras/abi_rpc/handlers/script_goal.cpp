/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file handlers/script_goal.cpp RPC handlers for ScriptGoal methods */

#include "script_goal.h"
#include "../script_execution_context.h"
#include "../script_deferred_int_result.h"

#include "../../../script/api/script_goal.hpp"
#include "../../../script/api/script_text.hpp"
#include "../../../script/api/script_company.hpp"
#include "../../../script/api/script_client.hpp"
#include "../../../script/script_suspend.hpp"
#include "../../../game/game.hpp"

static void SetGameScriptUnavailable(openttd::GenericError *error)
{
	if (error != nullptr)
	{
		error->set_error_code(openttd::ERROR_INTERNAL);
		error->set_error_summary("Game script instance not available");
	}
}

void HandleScriptGoal_New(const openttd::NewGoalRequest &request, openttd::ScriptGoalNewAbiReply &response, openttd::GenericError *error)
{
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

	ScriptDeferredIntResult deferred(ctx);

	GoalID goal_id = ::ScriptGoal::GOAL_INVALID;
	try
	{
		RawText *text = new RawText(goal_text);
		goal_id = ::ScriptGoal::New(company, text, goal_type, destination);
	}
	catch (Script_Suspend &e)
	{
		Game::GetInstance()->HandleScriptSuspend(e);
		if (deferred.IsPending())
		{
			response.set_goal_id(0);
			response.set_deferred_result_id(deferred.Id());
			return;
		}
		response.set_goal_id(::ScriptGoal::GOAL_INVALID.base());
		return;
	}

	int32_t resolved_value = 0;
	if (deferred.TryGetResolvedValue(resolved_value))
	{
		response.set_goal_id(static_cast<uint32_t>(resolved_value));
		return;
	}

	if (goal_id == ::ScriptGoal::GOAL_INVALID)
	{
		deferred.Cancel();
		response.set_goal_id(::ScriptGoal::GOAL_INVALID.base());
		return;
	}

	if (deferred.IsPending())
	{
		response.set_goal_id(0);
		response.set_deferred_result_id(deferred.Id());
		return;
	}

	response.set_goal_id(goal_id.base());
}

void HandleScriptGoal_Question(const openttd::QuestionRequest &request, openttd::QuestionReply &response, openttd::GenericError *error)
{
	ScriptExecutionContext ctx;
	if (!ctx.IsValid())
	{
		SetGameScriptUnavailable(error);
		response.set_success(false);
		return;
	}

	::ScriptCompany::CompanyID company = request.goal_global()
											 ? ::ScriptCompany::COMPANY_INVALID
											 : static_cast<::ScriptCompany::CompanyID>(request.company());

	bool success = ExecuteScriptCommand([&]()
										{
		RawText *text = new RawText(request.question());
		return ::ScriptGoal::Question(
			static_cast<SQInteger>(request.unique_id()),
			company,
			text,
			static_cast<::ScriptGoal::QuestionType>(request.type()),
			static_cast<SQInteger>(request.buttons())); });

	response.set_success(success);
}

void HandleScriptGoal_QuestionClient(const openttd::QuestionClientRequest &request, openttd::QuestionClientReply &response, openttd::GenericError *error)
{
	ScriptExecutionContext ctx;
	if (!ctx.IsValid())
	{
		SetGameScriptUnavailable(error);
		response.set_success(false);
		return;
	}

	bool success = ExecuteScriptCommand([&]()
										{
		RawText *text = new RawText(request.question());
		return ::ScriptGoal::QuestionClient(
			static_cast<SQInteger>(request.unique_id()),
			static_cast<::ScriptClient::ClientID>(request.client_id()),
			text,
			static_cast<::ScriptGoal::QuestionType>(request.type()),
			static_cast<SQInteger>(request.buttons())); });

	response.set_success(success);
}

void HandleScriptGoal_CloseQuestion(const openttd::CloseQuestionRequest &request, openttd::CloseQuestionReply &response, openttd::GenericError *error)
{
	ScriptExecutionContext ctx;
	if (!ctx.IsValid())
	{
		SetGameScriptUnavailable(error);
		response.set_success(false);
		return;
	}

	bool success = ExecuteScriptCommand([&]()
										{ return ::ScriptGoal::CloseQuestion(static_cast<SQInteger>(request.unique_id())); });

	response.set_success(success);
}
