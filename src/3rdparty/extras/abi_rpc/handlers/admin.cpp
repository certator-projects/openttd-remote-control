/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file handlers/admin.cpp Implementation of Admin service RPC handlers. */

#include "../../../openttd.h"
#include "../scoped_memory_manager.h"
#include "abi_rpc/admin.pb.h"
#include "abi_rpc/script_generic.pb.h"

#include "../../../network/network.h"
#include "../../../network/network_base.h"
#include "../../../network/network_func.h"
#include "../../../settings_type.h"
#include "../../../saveload/saveload.h"
#include "../../../fios.h"
#include "../../../fileio_type.h"
#include "../../../fileio_func.h"
#include "../../../company_type.h"
#include "../../../company_base.h"
#include "../../../command_func.h"
#include "../../../company_cmd.h"

#include "../chat_notifications.h"

#include <cstring>


/**
 * Handle GetMode RPC request.
 * @param request Deserialized request.
 * @param response Response to fill.
 */
void HandleGetMode(const openttd::GetGameModeRequest &request, openttd::GetGameModeReply &response, openttd::GenericError *error)
{
	(void)request;
	(void)error;
	// Map OpenTTD game mode to proto enum
	auto map_game_mode = [](GameMode mode) -> openttd::GameModeType
	{
		switch (mode)
		{
			case GM_MENU:
				return openttd::GameModeType::NEW_GAME_MENU;
			case GM_EDITOR:
				return openttd::GameModeType::SCENARIO_EDITOR;
			case GM_NORMAL:
				if (_network_server)
				{
					return openttd::GameModeType::MULTIPLAYER_SERVER;
				}
				else if (_networking)
				{
					return openttd::GameModeType::MULTIPLAYER_CLIENT;
				}
				return openttd::GameModeType::SINGLE_GAME;
			case GM_BOOTSTRAP:
			default:
				return openttd::GameModeType::UNKNOWN;
		}
	};

	// Map switch mode to game mode for consistency
	auto map_switch_mode = [](SwitchMode mode) -> openttd::GameModeType
	{
		switch (mode)
		{
			case SM_MENU:
				return openttd::GameModeType::NEW_GAME_MENU;
			case SM_EDITOR:
			case SM_LOAD_SCENARIO:
			case SM_LOAD_HEIGHTMAP:
			case SM_GENRANDLAND:
				return openttd::GameModeType::SCENARIO_EDITOR;
			case SM_NEWGAME:
			case SM_RESTARTGAME:
			case SM_RELOADGAME:
			case SM_LOAD_GAME:
			case SM_START_HEIGHTMAP:
			case SM_RESTART_HEIGHTMAP:
				return openttd::GameModeType::SINGLE_GAME;
			case SM_JOIN_GAME:
				return openttd::GameModeType::MULTIPLAYER_CLIENT;
			case SM_NONE:
			case SM_SAVE_GAME:
			case SM_SAVE_HEIGHTMAP:
			default:
				return openttd::GameModeType::UNKNOWN;
		}
	};

	response.set_current_mode(map_game_mode(_game_mode));
	response.set_new_mode(map_switch_mode(_switch_mode));
	response.set_is_dedicated(_network_dedicated);

	if (_settings_client.network.server_name.empty())
	{
		response.set_server_name("OpenTTD Server");
	}
	else {
		response.set_server_name(_settings_client.network.server_name);
	}
}

/**
  * Handle StartNetworkServer RPC request.
  * @param request Deserialized request.
  * @param response Response to fill.
  * @param error Optional error output parameter. If non-null and operation fails, error is set.
  */
void HandleStartNetworkServer(const openttd::StartNetworkServerRequest &request, openttd::StartNetworkServerReply &response, openttd::GenericError *error)
{
	// CRITICAL: Set this flag FIRST to tell SwitchToMode() we want to be a network server
	_is_network_server = true;

	// Set flag to prevent LoadFromConfig() from overwriting these settings
	_rpc_settings_override = true;

	// Set network server configuration
	_settings_client.network.server_name = request.server_name();
	_settings_client.network.client_name = request.spectator_player_name();
	_settings_client.network.max_clients = static_cast<uint8_t>(request.max_clients());
	_settings_client.network.max_companies = static_cast<uint8_t>(request.max_companies());

	// Set server password only if provided and not empty
	if (!request.server_password().empty())
	{
		_settings_client.network.server_password = request.server_password();
	}
	else
	{
		_settings_client.network.server_password.clear();
	}

	// Set visibility type
	switch (request.visibility_type())
	{
		case openttd::ServerVisibilityType::LOCAL:
			_settings_client.network.server_game_type = SERVER_GAME_TYPE_LOCAL;
			break;
		case openttd::ServerVisibilityType::PUBLIC:
			_settings_client.network.server_game_type = SERVER_GAME_TYPE_PUBLIC;
			break;
		case openttd::ServerVisibilityType::INVITE_ONLY:
			_settings_client.network.server_game_type = SERVER_GAME_TYPE_INVITE_ONLY;
			break;
		default:
			_settings_client.network.server_game_type = SERVER_GAME_TYPE_LOCAL;
			break;
	}

	// Load save file or scenario if a path is provided, otherwise generate a new game
	if (!request.save_path().empty())
	{
		FiosItem item;
		item.type = FIOS_TYPE_FILE;
		item.name = request.save_path();
		item.mtime = 0;
		_file_to_saveload.Set(item);
		_switch_mode = SM_LOAD_GAME;
	}
	else if (!request.scenario_path().empty())
	{
		FiosItem item;
		item.type = FIOS_TYPE_SCENARIO;
		item.name = request.scenario_path();
		item.mtime = 0;
		_file_to_saveload.Set(item);
		_switch_mode = SM_LOAD_GAME;
	}
	else
	{
		_switch_mode = SM_NEWGAME;
	}

	// No error - operation initiated successfully (fire-and-forget pattern)
	(void)error; // Unused - this operation doesn't fail
}

/**
  * Handle GetNetworkClients RPC request.
  * @param request Deserialized request.
  * @param response Response to fill.
  */
void HandleGetNetworkClients(const openttd::GetNetworkClientsRequest &request, openttd::GetNetworkClientsReply &response, openttd::GenericError *error)
{
	(void)request;
	(void)error;
	if (!_network_server)
	{
		return;
	}

	for (const NetworkClientInfo *ci : NetworkClientInfo::Iterate())
	{
		auto *client = response.add_clients();
		client->set_client_id(ci->client_id);
		client->set_client_name(ci->client_name);
		client->set_company_id(ci->client_playas.base());
		client->set_is_spectator(ci->client_playas == COMPANY_SPECTATOR);
	}
}

/**
  * Handle SendChatMessage RPC request.
  * @param request Deserialized request.
  * @param response Response to fill.
  * @param error Optional error output parameter. If non-null and operation fails, error is set.
  */
void HandleSendChatMessage(const openttd::SendChatMessageRequest &request, openttd::SendChatMessageReply &response, openttd::GenericError *error)
{
	if (!_network_server)
	{
		error->set_error_code(openttd::ERROR_INTERNAL);
		error->set_error_summary("Network server is not running");
		return;
	}

	NetworkServerSendChat(NETWORK_ACTION_CHAT, DESTTYPE_BROADCAST, 0, request.message(), CLIENT_ID_SERVER, 0, true);
}

/**
  * Handle SendPrivateChatMessage RPC request.
  * @param request Deserialized request.
  * @param response Response to fill.
  * @param error Optional error output parameter. If non-null and operation fails, error is set.
  */
void HandleSendPrivateChatMessage(const openttd::SendPrivateChatMessageRequest &request, openttd::SendPrivateChatMessageReply &response, openttd::GenericError *error)
{
	if (!_network_server)
	{
		error->set_error_code(openttd::ERROR_INTERNAL);
		error->set_error_summary("Network server is not running");
		return;
	}

	ClientID target_client = static_cast<ClientID>(request.client_id());
	NetworkClientInfo *ci = NetworkClientInfo::GetByClientID(target_client);
	if (ci == nullptr)
	{
		error->set_error_code(openttd::ERROR_NOT_FOUND);
		error->set_error_summary("Client not found: " + std::to_string(request.client_id()));
		return;
	}

	NetworkServerSendChat(NETWORK_ACTION_CHAT_CLIENT, DESTTYPE_CLIENT, target_client, request.message(), CLIENT_ID_SERVER, 0, true);
}

/**
 * Handle PollLastChatMessages RPC request.
 * Drains the in-memory chat notification queue maintained by OpenTTD.
 */
void HandlePollLastChatMessages(const openttd::PollLastChatMessagesRequest &request, openttd::PollLastChatMessagesReply &response, openttd::GenericError *error)
{
	(void)request;
	(void)error;
	std::vector<AbiRpcChatNotifications::ChatNotification> drained;
	AbiRpcChatNotifications::Drain(drained);

	for (const auto &msg : drained)
	{
		auto *out = response.add_messages();
		out->set_received_unix_ms(msg.received_unix_ms);
		out->set_action(static_cast<openttd::NetworkActionType>(msg.action));
		out->set_desttype(static_cast<openttd::ChatDestType>(msg.desttype));
		out->set_client_id(msg.client_id);
		out->set_message(msg.message);
	}
}

/**
  * Handle GetCompanyList RPC request.
  * @param request Deserialized request.
  * @param response Response to fill.
  */
void HandleGetCompanyList(const openttd::GetCompanyListRequest &request, openttd::GetCompanyListReply &response, openttd::GenericError *error)
{
	(void)request;
	(void)error;
	if (!_network_server)
	{
		return;
	}

	for (const Company *c : Company::Iterate())
	{
		auto *company = response.add_companies();
		company->set_company_id(c->index.base());
		company->set_company_name(c->name);

		for (const NetworkClientInfo *ci : NetworkClientInfo::Iterate())
		{
			if (ci->client_playas == c->index)
			{
				company->add_client_ids(ci->client_id);
			}
		}
	}
}
