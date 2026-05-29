/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file handlers/rpc_console.h RPC handlers for Console service (console command–backed methods). */

#ifndef ABI_RPC_PROXY_RPC_CONSOLE_H
#define ABI_RPC_PROXY_RPC_CONSOLE_H

#include "abi_rpc/console.pb.h"
#include "abi_rpc/script_generic.pb.h"

void HandleConsole_ResetCompany(const openttd::ResetCompanyRequest &request, openttd::ResetCompanyReply &response, openttd::GenericError *error);
void HandleConsole_SettingNewgame(const openttd::SettingNewgameRequest &request, openttd::SettingNewgameReply &response, openttd::GenericError *error);
void HandleConsole_Setting(const openttd::SettingRequest &request, openttd::SettingReply &response, openttd::GenericError *error);
void HandleConsole_Restart(const openttd::RestartRequest &request, openttd::RestartReply &response, openttd::GenericError *error);

#endif /* ABI_RPC_PROXY_RPC_CONSOLE_H */
