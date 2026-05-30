/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file handlers/abi_internal.h Internal ABI-RPC handlers (plugin wire only) */

#ifndef HANDLERS_ABI_INTERNAL_H
#define HANDLERS_ABI_INTERNAL_H

#include "abi_rpc/abi_internal.pb.h"
#include "abi_rpc/script_generic.pb.h"

void HandleAbiInternal_PollDeferredResult(const openttd::PollDeferredResultRequest &request, openttd::PollDeferredResultReply &response, openttd::GenericError *error);

#endif /* HANDLERS_ABI_INTERNAL_H */
