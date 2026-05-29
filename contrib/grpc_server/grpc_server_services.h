/*
 * gRPC Server for OpenTTD
 * Registers async gRPC services that forward to OpenTTD via UDS IPC.
 */

#include "call_base.h"

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include "admin.grpc.pb.h"
#include "console.grpc.pb.h"
#include "script_company.grpc.pb.h"
#include "script_generic.grpc.pb.h"
#include "script_goal.grpc.pb.h"
#include "script_map.grpc.pb.h"

#include "spdlog/spdlog.h"

#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>


void RegisterGRPCServices(grpc::ServerBuilder &builder);

void RegisterGRPCHandlers(grpc::ServerBuilder &builder, std::unique_ptr<grpc::ServerCompletionQueue> &g_completion_queue);
