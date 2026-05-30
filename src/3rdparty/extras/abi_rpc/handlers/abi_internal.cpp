/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file handlers/abi_internal.cpp Internal ABI-RPC handlers (plugin wire only) */

#include "abi_internal.h"
#include "../deferred_result.h"
#include "abi_rpc/script_generic.pb.h"

void HandleAbiInternal_PollDeferredResult(const openttd::PollDeferredResultRequest &request, openttd::PollDeferredResultReply &response, openttd::GenericError *error)
{
	int32_t value = 0;
	const DeferredResultStatus status = DeferredResult_Poll(request.deferred_result_id(), &value);

	switch (status)
	{
		case DeferredResultStatus::Pending:
			response.set_ready(false);
			break;

		case DeferredResultStatus::Resolved:
			response.set_ready(true);
			response.set_int_value(value);
			break;

		case DeferredResultStatus::Failed:
			response.set_ready(true);
			if (error != nullptr)
			{
				error->set_error_code(openttd::ERROR_INTERNAL);
				error->set_error_summary("Deferred script command failed");
			}
			break;

		case DeferredResultStatus::NotFound:
			response.set_ready(true);
			if (error != nullptr)
			{
				error->set_error_code(openttd::ERROR_NOT_FOUND);
				error->set_error_summary("Deferred result not found");
			}
			break;
	}
}
