/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file handlers/script_company.cpp RPC handlers for ScriptCompany methods */

#include "script_company.h"

#include "../../../script/api/script_company.hpp"

void HandleScriptCompany_GetQuarterlyCompanyValue(const openttd::GetQuarterlyCompanyValueRequest &request, openttd::GetQuarterlyCompanyValueReply &response, openttd::GenericError *error)
{
	(void)error;
	::ScriptCompany::CompanyID company = static_cast<::ScriptCompany::CompanyID>(request.company());
	::SQInteger quarter = static_cast<::SQInteger>(request.quarter());
	::Money value = ::ScriptCompany::GetQuarterlyCompanyValue(company, quarter);
	response.set_company_value(static_cast<int64_t>(value));
}
