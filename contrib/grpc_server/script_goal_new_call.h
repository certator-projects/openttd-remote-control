#ifndef GRPC_SERVER_SCRIPT_GOAL_NEW_CALL_H
#define GRPC_SERVER_SCRIPT_GOAL_NEW_CALL_H

#include "call_base.h"
#include "pending_deferred_calls.h"
#include "plugin_interface.h"

#include "script_goal.grpc.pb.h"
#include "abi_internal.pb.h"

#include <chrono>
#include <spdlog/spdlog.h>

using grpc::ServerAsyncResponseWriter;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::Status;

class ScriptGoalNewCall : public CallBase
{
public:
	ScriptGoalNewCall(openttd::ScriptGoal::AsyncService *service, ServerCompletionQueue *cq)
		: service_(service), cq_(cq), responder_(&ctx_), status_(CREATE)
	{
		this->Proceed(true);
	}

	void Proceed(bool ok) override
	{
		if (this->status_ == CREATE)
		{
			this->status_ = PROCESS;
			this->service_->RequestNew(&this->ctx_, &this->request_, &this->responder_, this->cq_, this->cq_, this);
			return;
		}

		if (this->status_ == PROCESS)
		{
			if (!ok)
			{
				delete this;
				return;
			}

			new ScriptGoalNewCall(this->service_, this->cq_);

			std::string error_reason;
			openttd::GenericError generic_error;
			openttd::ScriptGoalNewAbiReply abi_reply;
			if (!this->ExecuteNewGoalRpc(abi_reply, error_reason, generic_error))
			{
				this->status_ = FINISH;
				this->responder_.FinishWithError(Status(grpc::INTERNAL, error_reason), this);
				return;
			}

			if (generic_error.error_code() != openttd::ERROR_NONE)
			{
				this->status_ = FINISH;
				grpc::StatusCode grpc_code = ErrorCodeToGrpcStatus(generic_error.error_code());
				this->responder_.FinishWithError(Status(grpc_code, generic_error.error_summary()), this);
				return;
			}

			if (abi_reply.deferred_result_id() != 0)
			{
				this->deferred_result_id_ = abi_reply.deferred_result_id();
				this->client_reply_.set_goal_id(abi_reply.goal_id());
				this->deadline_ = std::chrono::steady_clock::now() + std::chrono::seconds(30);
				this->status_ = WAIT_DEFERRED;
				RegisterPendingDeferredCall(this);
				return;
			}

			this->client_reply_.set_goal_id(abi_reply.goal_id());
			this->status_ = FINISH;
			this->responder_.Finish(this->client_reply_, Status::OK, this);
			return;
		}

		if (this->status_ == WAIT_DEFERRED)
		{
			if (std::chrono::steady_clock::now() >= this->deadline_)
			{
				UnregisterPendingDeferredCall(this);
				this->status_ = FINISH;
				this->responder_.FinishWithError(Status(grpc::DEADLINE_EXCEEDED, "Timed out waiting for goal creation"), this);
				return;
			}

			openttd::PollDeferredResultRequest poll_request;
			poll_request.set_deferred_result_id(this->deferred_result_id_);

			openttd::PollDeferredResultReply poll_reply;
			openttd::GenericError poll_error;
			std::string error_reason;
			if (!this->ExecutePollDeferredRpc(poll_request, poll_reply, poll_error, error_reason))
			{
				UnregisterPendingDeferredCall(this);
				this->status_ = FINISH;
				this->responder_.FinishWithError(Status(grpc::INTERNAL, error_reason), this);
				return;
			}

			if (poll_error.error_code() != openttd::ERROR_NONE)
			{
				UnregisterPendingDeferredCall(this);
				this->status_ = FINISH;
				grpc::StatusCode grpc_code = ErrorCodeToGrpcStatus(poll_error.error_code());
				this->responder_.FinishWithError(Status(grpc_code, poll_error.error_summary()), this);
				return;
			}

			if (!poll_reply.ready())
			{
				return;
			}

			this->client_reply_.set_goal_id(static_cast<uint32_t>(poll_reply.int_value()));
			UnregisterPendingDeferredCall(this);
			this->status_ = FINISH;
			this->responder_.Finish(this->client_reply_, Status::OK, this);
			return;
		}

		delete this;
	}

private:
	bool ExecuteNewGoalRpc(openttd::ScriptGoalNewAbiReply &abi_reply, std::string &error_reason, openttd::GenericError &generic_error)
	{
		return this->ExecuteRpc(RPC_SCRIPTGOAL_NEW, "RPC_SCRIPTGOAL_NEW", this->request_, abi_reply, generic_error, error_reason);
	}

	bool ExecutePollDeferredRpc(const openttd::PollDeferredResultRequest &request, openttd::PollDeferredResultReply &reply,
								openttd::GenericError &generic_error, std::string &error_reason)
	{
		return this->ExecuteRpc(RPC_ABI_INTERNAL_POLL_DEFERRED_RESULT, "RPC_ABI_INTERNAL_POLL_DEFERRED_RESULT",
								request, reply, generic_error, error_reason);
	}

	template <typename TRequest, typename TReply>
	bool ExecuteRpc(int32_t rpc_method_id, const char *method_name, const TRequest &request, TReply &reply,
					openttd::GenericError &generic_error, std::string &error_reason)
	{
		if (g_uds_socket_path.empty())
		{
			error_reason = std::string("UDS socket path not configured for ") + method_name;
			spdlog::error("[gRPC Server] {}", error_reason);
			return false;
		}

		size_t request_size = request.ByteSizeLong();
		std::vector<uint8_t> request_buffer(request_size);
		if (request_size > 0)
		{
			request.SerializeToArray(request_buffer.data(), request_size);
		}

		ottd::ipc::RpcResponse rpc_response{};
		if (!ottd::ipc::CallRpc(g_uds_socket_path, rpc_method_id, request_buffer.data(), request_size, rpc_response))
		{
			error_reason = std::string("UDS RPC call failed for ") + method_name;
			spdlog::error("[gRPC Server] {}", error_reason);
			return false;
		}

		if (rpc_response.status != 0)
		{
			error_reason = std::string(method_name) + " call failed with status " + std::to_string(rpc_response.status);
			spdlog::error("[gRPC Server] {}", error_reason);
			return false;
		}

		if (!rpc_response.error.empty())
		{
			if (!generic_error.ParseFromArray(rpc_response.error.data(), rpc_response.error.size()))
			{
				spdlog::warn("[gRPC Server] Failed to parse GenericError for {}", method_name);
			}
		}

		if (!reply.ParseFromArray(rpc_response.response.data(), rpc_response.response.size()))
		{
			error_reason = std::string("Failed to parse response for ") + method_name;
			spdlog::error("[gRPC Server] {}", error_reason);
			return false;
		}

		return true;
	}

	openttd::ScriptGoal::AsyncService *service_;
	ServerCompletionQueue *cq_;
	ServerContext ctx_;
	openttd::NewGoalRequest request_;
	openttd::NewGoalReply client_reply_;
	ServerAsyncResponseWriter<openttd::NewGoalReply> responder_;
	uint32_t deferred_result_id_ = 0;
	std::chrono::steady_clock::time_point deadline_;

	enum CallStatus
	{
		CREATE,
		PROCESS,
		WAIT_DEFERRED,
		FINISH
	};
	CallStatus status_;
};

inline void CreateScriptGoalNewHandler(openttd::ScriptGoal::AsyncService *service, ServerCompletionQueue *cq)
{
	new ScriptGoalNewCall(service, cq);
}

#endif /* GRPC_SERVER_SCRIPT_GOAL_NEW_CALL_H */
