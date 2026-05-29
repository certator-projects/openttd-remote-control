/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file controllers/reset_company_controller.h Shared reset_company controller logic. */

#ifndef RESET_COMPANY_CONTROLLER_H
#define RESET_COMPANY_CONTROLLER_H

#include <string>

#include "../company_type.h"

enum class ResetCompanyControllerStatus {
	Success,
	ErrorReturnTrue,
	ErrorReturnFalse,
};

ResetCompanyControllerStatus ResetCompanyController(CompanyID company_id, std::string &out_error);

#endif /* RESET_COMPANY_CONTROLLER_H */
