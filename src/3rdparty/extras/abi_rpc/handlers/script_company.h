/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file handlers/script_company.h RPC handlers for ScriptCompany methods */

#ifndef ABI_RPC_PROXY_SCRIPT_COMPANY_H
#define ABI_RPC_PROXY_SCRIPT_COMPANY_H

#include "abi_rpc/script_company.pb.h"
#include "abi_rpc/script_generic.pb.h"

void HandleScriptCompany_GetQuarterlyCompanyValue(const openttd::GetQuarterlyCompanyValueRequest &request, openttd::GetQuarterlyCompanyValueReply &response, openttd::GenericError *error);

#endif /* ABI_RPC_PROXY_SCRIPT_COMPANY_H */
