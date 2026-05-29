/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file handlers/rpc_console.cpp RPC handlers for Console service methods */

#include "rpc_console.h"

#include "../../../company_type.h"
#include "../../../network/network.h"
#include "../../../settings_func.h"
#include "../../../../controllers/reset_company_controller.h"
#include "../../../../controllers/restart_game_controller.h"

/**
 * Handle ResetCompany RPC request (game console command reset_company).
 * @param request Deserialized request.
 * @param response Response to fill.
 * @param error Error output parameter. If operation fails, error is set.
 */
void HandleConsole_ResetCompany(const openttd::ResetCompanyRequest &request, openttd::ResetCompanyReply &response, openttd::GenericError *error)
{
	(void)response;

	if (!_network_server)
	{
		error->set_error_code(openttd::ERROR_INTERNAL);
		error->set_error_summary("Network server is not running");
		return;
	}

	if (request.company_id() > 0xFFU)
	{
		error->set_error_code(openttd::ERROR_INVALID_ARGUMENT);
		error->set_error_summary("The given company-id is not a valid number.");
		return;
	}

	CompanyID index{static_cast<uint8_t>(request.company_id())};
	std::string controller_error;
	ResetCompanyControllerStatus status = ResetCompanyController(index, controller_error);
	if (status != ResetCompanyControllerStatus::Success)
	{
		error->set_error_code(openttd::ERROR_INVALID_ARGUMENT);
		error->set_error_summary(controller_error);
		return;
	}
}

/**
 * Handle SettingNewgame RPC request (game console command setting_newgame).
 * When request.value is not set, returns the current newgame value.
 * When request.value is set, updates the newgame setting to the given value and returns the resulting value.
 */
void HandleConsole_SettingNewgame(const openttd::SettingNewgameRequest &request, openttd::SettingNewgameReply &response, openttd::GenericError *error)
{
	if (request.name().empty())
	{
		error->set_error_code(openttd::ERROR_INVALID_ARGUMENT);
		error->set_error_summary("Setting name must not be empty.");
		return;
	}

	/* Get-only */
	if (!request.has_value())
	{
		std::string value;
		std::string err;
		if (!TryGetSettingValue(request.name(), value, err, true))
		{
			error->set_error_code(openttd::ERROR_INVALID_ARGUMENT);
			error->set_error_summary(err);
			return;
		}
		response.set_value(value);
		return;
	}

	/* Set (force_newgame=true) */
	std::string value;
	std::string err;
	if (!TrySetSettingValue(request.name(), request.value(), value, err, true))
	{
		error->set_error_code(openttd::ERROR_INVALID_ARGUMENT);
		error->set_error_summary(err);
		return;
	}

	response.set_value(value);
}

/**
 * Handle Setting RPC request (game console command setting).
 * When request.value is not set, returns the current value for the running game.
 * When request.value is set, updates the setting for the current game and returns the resulting value.
 */
void HandleConsole_Setting(const openttd::SettingRequest &request, openttd::SettingReply &response, openttd::GenericError *error)
{
	if (request.name().empty())
	{
		error->set_error_code(openttd::ERROR_INVALID_ARGUMENT);
		error->set_error_summary("Setting name must not be empty.");
		return;
	}

	/* Get-only */
	if (!request.has_value())
	{
		std::string value;
		std::string err;
		if (!TryGetSettingValue(request.name(), value, err, false))
		{
			error->set_error_code(openttd::ERROR_INVALID_ARGUMENT);
			error->set_error_summary(err);
			return;
		}
		response.set_value(value);
		return;
	}

	/* Set (force_newgame=false — current game) */
	std::string value;
	std::string err;
	if (!TrySetSettingValue(request.name(), request.value(), value, err, false))
	{
		error->set_error_code(openttd::ERROR_INVALID_ARGUMENT);
		error->set_error_summary(err);
		return;
	}

	response.set_value(value);
}

/**
 * Handle Restart RPC request (game console command restart).
 * @param request Deserialized request.
 * @param response Response to fill.
 * @param error Error output parameter. If operation fails, error is set.
 */
void HandleConsole_Restart(const openttd::RestartRequest &request, openttd::RestartReply &response, openttd::GenericError *error)
{
	(void)response;

	std::string_view mode;
	if (request.has_mode()) mode = request.mode();

	std::string controller_error;
	const RestartGameControllerStatus status = RestartGameController(mode, controller_error);
	if (status != RestartGameControllerStatus::Success)
	{
		error->set_error_code(openttd::ERROR_INVALID_ARGUMENT);
		error->set_error_summary(controller_error);
	}
}
