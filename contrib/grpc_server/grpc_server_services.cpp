/*
 * gRPC Server Plugin for OpenTTD
 * This plugin provides a gRPC interface to OpenTTD by acting as a bridge
 * between external gRPC clients and OpenTTD's C-ABI RPC interface.
 */

#include "call_base.h"
#include "script_goal_new_call.h"
#include "pending_deferred_calls.h"

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include "admin.grpc.pb.h"
#include "console.grpc.pb.h"
#include "script_company.grpc.pb.h"
#include "script_goal.grpc.pb.h"
#include "script_map.grpc.pb.h"

#include "spdlog/spdlog.h"

#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>


/**
 * Macro to simplify gRPC handler registration.
 */
#define REGISTER_GRPC_HANDLER(service_var, service_type, req_type, reply_type, rpc_id, method_name)             \
	CreateABIProxyHandler<openttd::req_type, openttd::reply_type, openttd::service_type::AsyncService, rpc_id>( \
		&service_var, g_completion_queue.get(),                                                                 \
		rpc_id, #rpc_id,                                                                                        \
		[](auto *svc, auto *ctx, auto *req, auto *resp, auto *cq, auto *ncq, void *tag) { svc->Request##method_name(ctx, req, resp, cq, ncq, tag); })

// Async service instances
static openttd::Admin::AsyncService g_admin_service;
static openttd::Console::AsyncService g_console_service;
static openttd::ScriptCompany::AsyncService g_script_company_service;
static openttd::ScriptGoal::AsyncService g_script_goal_service;
static openttd::ScriptMap::AsyncService g_script_map_service;

void RegisterGRPCServices(grpc::ServerBuilder &builder)
{
	builder.RegisterService(&g_admin_service);
	builder.RegisterService(&g_console_service);
	builder.RegisterService(&g_script_company_service);
	builder.RegisterService(&g_script_goal_service);
	builder.RegisterService(&g_script_map_service);
}

void RegisterGRPCHandlers(grpc::ServerBuilder &builder, std::unique_ptr<grpc::ServerCompletionQueue> &g_completion_queue)
{
	// Admin
	REGISTER_GRPC_HANDLER(g_admin_service, Admin, GetGameModeRequest, GetGameModeReply, RPC_GET_MODE, GetGameMode);
	REGISTER_GRPC_HANDLER(g_admin_service, Admin, StartNetworkServerRequest, StartNetworkServerReply, RPC_START_NETWORK_SERVER, StartNetworkServer);
	REGISTER_GRPC_HANDLER(g_admin_service, Admin, GetNetworkClientsRequest, GetNetworkClientsReply, RPC_GET_NETWORK_CLIENTS, GetNetworkClients);
	REGISTER_GRPC_HANDLER(g_admin_service, Admin, SendChatMessageRequest, SendChatMessageReply, RPC_SEND_CHAT_MESSAGE, SendChatMessage);
	REGISTER_GRPC_HANDLER(g_admin_service, Admin, SendPrivateChatMessageRequest, SendPrivateChatMessageReply, RPC_SEND_PRIVATE_CHAT_MESSAGE, SendPrivateChatMessage);
	REGISTER_GRPC_HANDLER(g_admin_service, Admin, PollLastChatMessagesRequest, PollLastChatMessagesReply, RPC_POLL_LAST_CHAT_MESSAGES, PollLastChatMessages);
	REGISTER_GRPC_HANDLER(g_admin_service, Admin, GetCompanyListRequest, GetCompanyListReply, RPC_GET_COMPANY_LIST, GetCompanyList);

	// Console
	REGISTER_GRPC_HANDLER(g_console_service, Console, ResetCompanyRequest, ResetCompanyReply, RPC_RESET_COMPANY, ResetCompany);
	REGISTER_GRPC_HANDLER(g_console_service, Console, SettingNewgameRequest, SettingNewgameReply, RPC_SETTING_NEWGAME, SettingNewgame);
	REGISTER_GRPC_HANDLER(g_console_service, Console, SettingRequest, SettingReply, RPC_SETTING, Setting);
	REGISTER_GRPC_HANDLER(g_console_service, Console, RestartRequest, RestartReply, RPC_RESTART, Restart);

	// ScriptCompany
	REGISTER_GRPC_HANDLER(g_script_company_service, ScriptCompany, GetQuarterlyCompanyValueRequest, GetQuarterlyCompanyValueReply, RPC_SCRIPTCOMPANY_GET_QUARTERLY_COMPANY_VALUE, GetQuarterlyCompanyValue);

	// ScriptGoal
	CreateScriptGoalNewHandler(&g_script_goal_service, g_completion_queue.get());
	REGISTER_GRPC_HANDLER(g_script_goal_service, ScriptGoal, RemoveGoalRequest, RemoveGoalReply, RPC_SCRIPTGOAL_REMOVE, Remove);
	REGISTER_GRPC_HANDLER(g_script_goal_service, ScriptGoal, SetGoalTextRequest, SetGoalTextReply, RPC_SCRIPTGOAL_SET_TEXT, SetText);
	REGISTER_GRPC_HANDLER(g_script_goal_service, ScriptGoal, SetGoalCompletedRequest, SetGoalCompletedReply, RPC_SCRIPTGOAL_SET_COMPLETED, SetCompleted);
	REGISTER_GRPC_HANDLER(g_script_goal_service, ScriptGoal, IsGoalCompletedRequest, IsGoalCompletedReply, RPC_SCRIPTGOAL_IS_COMPLETED, IsCompleted);
	REGISTER_GRPC_HANDLER(g_script_goal_service, ScriptGoal, QuestionRequest, QuestionReply, RPC_SCRIPTGOAL_QUESTION, Question);
	REGISTER_GRPC_HANDLER(g_script_goal_service, ScriptGoal, QuestionClientRequest, QuestionClientReply, RPC_SCRIPTGOAL_QUESTION_CLIENT, QuestionClient);
	REGISTER_GRPC_HANDLER(g_script_goal_service, ScriptGoal, CloseQuestionRequest, CloseQuestionReply, RPC_SCRIPTGOAL_CLOSE_QUESTION, CloseQuestion);

	// ScriptMap
	REGISTER_GRPC_HANDLER(g_script_map_service, ScriptMap, GetMapSizeXRequest, GetMapSizeXReply, RPC_SCRIPTMAP_GET_MAP_SIZE_X, GetMapSizeX);
	REGISTER_GRPC_HANDLER(g_script_map_service, ScriptMap, GetMapSizeYRequest, GetMapSizeYReply, RPC_SCRIPTMAP_GET_MAP_SIZE_Y, GetMapSizeY);
	REGISTER_GRPC_HANDLER(g_script_map_service, ScriptMap, GetTileIndexRequest, GetTileIndexReply, RPC_SCRIPTMAP_GET_TILE_INDEX, GetTileIndex);
}
