/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file rpc_handler.cpp Implementation of RPC handler. */

#include "rpc_handler.h"
#include "abi_rpc/console.pb.h"
#include "abi_rpc/script_generic.pb.h"
#include "plugin_interface.h"
#include "scoped_memory_manager.h"
#include "host_errors.h"

// Include handlers
#include "handlers/admin.h"
#include "handlers/rpc_console.h"
#include "handlers/script_company.h"
#include "handlers/script_goal.h"
#include "handlers/script_map.h"
#include "handlers/abi_internal.h"

#include <cstring>
#include <cstdio>
#include <unordered_map>
#include <functional>


/**
 * Last error state for RPC handler operations.
 */
static thread_local int32_t g_last_rpc_error = RPC_SUCCESS;

/**
 * Template function to handle RPC method invocations.
 * Parses the request, invokes the handler (which may set a GenericError),
 * and serializes the response and optional error payload.
 *
 * @tparam RequestType Protobuf request message type.
 * @tparam ResponseType Protobuf response message type.
 * @param memory_manager Memory manager for allocations.
 * @param request Serialized request data.
 * @param request_size Size of request data.
 * @param out_response Output parameter for response buffer.
 * @param out_response_size Output parameter for response size.
 * @param out_error Output parameter for error buffer (nullptr if no error).
 * @param out_error_size Output parameter for error size.
 * @param handler Function to process the request, fill the response, and optionally set error.
 * @return 0 on success, error code on failure.
 */
template <typename RequestType, typename ResponseType>
static int32_t HandleRPCMethod(
	ScopedMemoryManager *memory_manager,
	const void *request,
	size_t request_size,
	void **out_response,
	size_t *out_response_size,
	void **out_error,
	size_t *out_error_size,
	void (*handler)(const RequestType &, ResponseType &, openttd::GenericError *))
{
	RequestType req;
	ResponseType resp;
	openttd::GenericError err;

	*out_error = nullptr;
	*out_error_size = 0;

	// Parse request from byte array
	if (!req.ParseFromArray(request, static_cast<int>(request_size)))
	{
		g_last_rpc_error = RPC_ERROR_PARSE_FAILED;
		return RPC_ERROR_PARSE_FAILED;
	}

	// Call the specific handler with error parameter
	handler(req, resp, &err);

	// If handler set an error, serialize it
	if (err.error_code() != openttd::ERROR_NONE)
	{
		size_t err_size = err.ByteSizeLong();
		void *err_buffer = nullptr;
		int32_t err_alloc_result = ScopedMemoryManager_Allocate(memory_manager, err_size, &err_buffer);
		if (err_alloc_result != 0)
		{
			g_last_rpc_error = RPC_ERROR_ALLOCATION_FAILED;
			return RPC_ERROR_ALLOCATION_FAILED;
		}

		if (!err.SerializeToArray(err_buffer, static_cast<int>(err_size)))
		{
			ScopedMemoryManager_Deallocate(memory_manager, err_buffer);
			g_last_rpc_error = RPC_ERROR_SERIALIZE_FAILED;
			return RPC_ERROR_SERIALIZE_FAILED;
		}

		*out_error = err_buffer;
		*out_error_size = err_size;
	}

	// Allocate buffer for response
	size_t size = resp.ByteSizeLong();
	void *buffer = nullptr;
	int32_t alloc_result = ScopedMemoryManager_Allocate(memory_manager, size, &buffer);
	if (alloc_result != 0)
	{
		g_last_rpc_error = RPC_ERROR_ALLOCATION_FAILED;
		return RPC_ERROR_ALLOCATION_FAILED;
	}

	// Serialize response to buffer
	if (!resp.SerializeToArray(buffer, static_cast<int>(size)))
	{
		ScopedMemoryManager_Deallocate(memory_manager, buffer);
		g_last_rpc_error = RPC_ERROR_SERIALIZE_FAILED;
		return RPC_ERROR_SERIALIZE_FAILED;
	}

	// Set output parameters
	*out_response = buffer;
	*out_response_size = size;
	g_last_rpc_error = RPC_SUCCESS;
	return RPC_SUCCESS;
}

using RPCHandlerFunc = std::function<int32_t(
	ScopedMemoryManager *memory_manager,
	const void *request,
	size_t request_size,
	void **out_response,
	size_t *out_response_size,
	void **out_error,
	size_t *out_error_size)>;

static std::unordered_map<int32_t, RPCHandlerFunc> g_rpc_handler_registry;

#define REGISTER_RPC_HANDLER(method_id, req_type, resp_type, handler_func)                                                                                \
	g_rpc_handler_registry[method_id] = [](ScopedMemoryManager *mm, const void *req, size_t req_sz, void **out_resp, size_t *out_resp_sz, void **out_err, \
										   size_t *out_err_sz) -> int32_t { return HandleRPCMethod<req_type, resp_type>(mm, req, req_sz, out_resp, out_resp_sz, out_err, out_err_sz, handler_func); }

/**
 * Initialize the RPC handler system.
 * Registers all available RPC method handlers in the global registry.
 */
void RPCHandler_Initialize()
{
	// Admin methods
	REGISTER_RPC_HANDLER(RPC_GET_MODE, openttd::GetGameModeRequest, openttd::GetGameModeReply, HandleGetMode);
	REGISTER_RPC_HANDLER(RPC_START_NETWORK_SERVER, openttd::StartNetworkServerRequest, openttd::StartNetworkServerReply, HandleStartNetworkServer);
	REGISTER_RPC_HANDLER(RPC_GET_NETWORK_CLIENTS, openttd::GetNetworkClientsRequest, openttd::GetNetworkClientsReply, HandleGetNetworkClients);
	REGISTER_RPC_HANDLER(RPC_SEND_CHAT_MESSAGE, openttd::SendChatMessageRequest, openttd::SendChatMessageReply, HandleSendChatMessage);
	REGISTER_RPC_HANDLER(RPC_SEND_PRIVATE_CHAT_MESSAGE, openttd::SendPrivateChatMessageRequest, openttd::SendPrivateChatMessageReply, HandleSendPrivateChatMessage);
	REGISTER_RPC_HANDLER(RPC_GET_COMPANY_LIST, openttd::GetCompanyListRequest, openttd::GetCompanyListReply, HandleGetCompanyList);
	REGISTER_RPC_HANDLER(RPC_POLL_LAST_CHAT_MESSAGES, openttd::PollLastChatMessagesRequest, openttd::PollLastChatMessagesReply, HandlePollLastChatMessages);

	// Console methods (game console command–backed RPCs)
	REGISTER_RPC_HANDLER(RPC_RESET_COMPANY, openttd::ResetCompanyRequest, openttd::ResetCompanyReply, HandleConsole_ResetCompany);
	REGISTER_RPC_HANDLER(RPC_SETTING_NEWGAME, openttd::SettingNewgameRequest, openttd::SettingNewgameReply, HandleConsole_SettingNewgame);
	REGISTER_RPC_HANDLER(RPC_SETTING, openttd::SettingRequest, openttd::SettingReply, HandleConsole_Setting);
	REGISTER_RPC_HANDLER(RPC_RESTART, openttd::RestartRequest, openttd::RestartReply, HandleConsole_Restart);

	// ScriptCompany methods
	REGISTER_RPC_HANDLER(RPC_SCRIPTCOMPANY_GET_QUARTERLY_COMPANY_VALUE, openttd::GetQuarterlyCompanyValueRequest, openttd::GetQuarterlyCompanyValueReply, HandleScriptCompany_GetQuarterlyCompanyValue);

	// ScriptGoal methods
	REGISTER_RPC_HANDLER(RPC_SCRIPTGOAL_NEW, openttd::NewGoalRequest, openttd::ScriptGoalNewAbiReply, HandleScriptGoal_New);
	REGISTER_RPC_HANDLER(RPC_SCRIPTGOAL_QUESTION, openttd::QuestionRequest, openttd::QuestionReply, HandleScriptGoal_Question);
	REGISTER_RPC_HANDLER(RPC_SCRIPTGOAL_QUESTION_CLIENT, openttd::QuestionClientRequest, openttd::QuestionClientReply, HandleScriptGoal_QuestionClient);
	REGISTER_RPC_HANDLER(RPC_SCRIPTGOAL_CLOSE_QUESTION, openttd::CloseQuestionRequest, openttd::CloseQuestionReply, HandleScriptGoal_CloseQuestion);

	// ScriptMap methods
	REGISTER_RPC_HANDLER(RPC_SCRIPTMAP_GET_MAP_SIZE_X, openttd::GetMapSizeXRequest, openttd::GetMapSizeXReply, HandleScriptMap_GetMapSizeX);
	REGISTER_RPC_HANDLER(RPC_SCRIPTMAP_GET_MAP_SIZE_Y, openttd::GetMapSizeYRequest, openttd::GetMapSizeYReply, HandleScriptMap_GetMapSizeY);
	REGISTER_RPC_HANDLER(RPC_SCRIPTMAP_GET_TILE_INDEX, openttd::GetTileIndexRequest, openttd::GetTileIndexReply, HandleScriptMap_GetTileIndex);

	REGISTER_RPC_HANDLER(RPC_ABI_INTERNAL_POLL_DEFERRED_RESULT, openttd::PollDeferredResultRequest, openttd::PollDeferredResultReply, HandleAbiInternal_PollDeferredResult);
}

// ============================================================
// RPC Handler Implementation
// ============================================================

/**
 * Host RPC handler implementation.
 */
int32_t RPCHandler_Handle(
	ScopedMemoryManager *memory_manager,
	int32_t rpc_method_id,
	const void *request,
	size_t request_size,
	void **out_response,
	size_t *out_response_size,
	void **out_error,
	size_t *out_error_size)
{
	if (memory_manager == nullptr || out_response == nullptr || out_response_size == nullptr ||
		out_error == nullptr || out_error_size == nullptr)
	{
		g_last_rpc_error = RPC_ERROR_NULL_PARAMETER;
		if (out_response != nullptr) *out_response = nullptr;
		if (out_response_size != nullptr) *out_response_size = 0;
		if (out_error != nullptr) *out_error = nullptr;
		if (out_error_size != nullptr) *out_error_size = 0;
		return RPC_ERROR_NULL_PARAMETER;
	}

	*out_response = nullptr;
	*out_response_size = 0;
	*out_error = nullptr;
	*out_error_size = 0;

	try
	{
		auto it = g_rpc_handler_registry.find(rpc_method_id);
		if (it == g_rpc_handler_registry.end())
		{
			g_last_rpc_error = RPC_ERROR_UNKNOWN_METHOD;
			return RPC_ERROR_UNKNOWN_METHOD;
		}

		return it->second(memory_manager, request, request_size,
						  out_response, out_response_size, out_error, out_error_size);
	}
	catch (...)
	{
		// Handle any exceptions
		g_last_rpc_error = RPC_ERROR_SERIALIZE_FAILED;
		return RPC_ERROR_SERIALIZE_FAILED;
	}
}
