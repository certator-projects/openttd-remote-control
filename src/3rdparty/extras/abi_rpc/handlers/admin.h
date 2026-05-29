/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file handlers/admin.h RPC handlers for Admin service methods */

#ifndef ABI_RPC_PROXY_ADMIN_H
#define ABI_RPC_PROXY_ADMIN_H

#include "abi_rpc/admin.pb.h"
#include "abi_rpc/script_generic.pb.h"

void HandleGetMode(const openttd::GetGameModeRequest &request, openttd::GetGameModeReply &response, openttd::GenericError *error);
void HandleStartNetworkServer(const openttd::StartNetworkServerRequest &request, openttd::StartNetworkServerReply &response, openttd::GenericError *error);
void HandleGetNetworkClients(const openttd::GetNetworkClientsRequest &request, openttd::GetNetworkClientsReply &response, openttd::GenericError *error);
void HandleSendChatMessage(const openttd::SendChatMessageRequest &request, openttd::SendChatMessageReply &response, openttd::GenericError *error);
void HandleSendPrivateChatMessage(const openttd::SendPrivateChatMessageRequest &request, openttd::SendPrivateChatMessageReply &response, openttd::GenericError *error);
void HandlePollLastChatMessages(const openttd::PollLastChatMessagesRequest &request, openttd::PollLastChatMessagesReply &response, openttd::GenericError *error);
void HandleGetCompanyList(const openttd::GetCompanyListRequest &request, openttd::GetCompanyListReply &response, openttd::GenericError *error);

#endif /* ABI_RPC_PROXY_ADMIN_H */
