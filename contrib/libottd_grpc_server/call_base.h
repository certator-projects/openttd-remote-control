#ifndef GRPC_PLUGIN_CALL_BASE_H
#define GRPC_PLUGIN_CALL_BASE_H

#include <grpcpp/grpcpp.h>
#include "admin.grpc.pb.h"
#include "script_generic.pb.h"
#include "plugin_interface.h"
#include "spdlog/spdlog.h"
#include <functional>

using grpc::ServerAsyncResponseWriter;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::Status;
using openttd::Admin;

extern HostOps g_host_ops;
extern RPCHandler g_rpc_handler;

class CallBase
{
public:
	virtual void Proceed(bool ok) = 0;
	virtual ~CallBase() {}
};

/**
 * Convert GenericError error_code to gRPC status code.
 */
inline grpc::StatusCode ErrorCodeToGrpcStatus(openttd::ErrorCode error_code)
{
	switch (error_code)
	{
		case openttd::ERROR_NONE:
			return grpc::OK;
		case openttd::ERROR_INVALID_ARGUMENT:
			return grpc::INVALID_ARGUMENT;
		case openttd::ERROR_OUT_OF_RANGE:
			return grpc::OUT_OF_RANGE;
		case openttd::ERROR_NOT_FOUND:
			return grpc::NOT_FOUND;
		case openttd::ERROR_INTERNAL:
			return grpc::INTERNAL;
		case openttd::ERROR_UNIMPLEMENTED:
			return grpc::UNIMPLEMENTED;
		default:
			return grpc::INTERNAL;
	}
}

template <typename TRequest, typename TReply, typename Derived, typename TService = Admin::AsyncService>
class AsyncCallABIProxyHandler : public CallBase
{
public:
	AsyncCallABIProxyHandler(TService *service, ServerCompletionQueue *cq)
		: service_(service), cq_(cq), responder_(&ctx_), status_(CREATE)
	{
		Proceed(true);
	}

	void Proceed(bool ok) override
	{
		if (status_ == CREATE)
		{
			status_ = PROCESS;
			Derived::RegisterRequest(service_, &ctx_, &request_, &responder_, cq_, cq_, this);
		}
		else if (status_ == PROCESS)
		{
			if (!ok)
			{
				delete this;
				return;
			}

			new Derived(service_, cq_);

			std::string error_reason;
			openttd::GenericError generic_error;
			bool success = ExecuteRPCCallDirect(
				static_cast<Derived *>(this)->GetRPCMethodID(),
				static_cast<Derived *>(this)->GetRPCMethodName(),
				error_reason,
				generic_error);

			status_ = FINISH;

			if (!success)
			{
				responder_.FinishWithError(Status(grpc::INTERNAL, error_reason), this);
			}
			else if (generic_error.error_code() != openttd::ERROR_NONE)
			{
				grpc::StatusCode grpc_code = ErrorCodeToGrpcStatus(generic_error.error_code());
				responder_.FinishWithError(Status(grpc_code, generic_error.error_summary()), this);
			}
			else {
				responder_.Finish(reply_, Status::OK, this);
			}
		}
		else
		{
			delete this;
		}
	}

protected:
	bool ExecuteRPCCallDirect(int32_t rpc_method_id, const char *method_name,
							  std::string &error_reason, openttd::GenericError &generic_error)
	{
		if (g_rpc_handler == nullptr)
		{
			error_reason = std::string("RPCHandler not available for ") + method_name;
			spdlog::error("[gRPC Plugin] {}", error_reason);
			return false;
		}

		ScopedMemoryManager *mem_mgr = nullptr;
		int32_t result = g_host_ops.memory.create(&mem_mgr);
		if (result != 0 || mem_mgr == nullptr)
		{
			error_reason = std::string("Failed to create memory manager for ") + method_name;
			spdlog::error("[gRPC Plugin] {}", error_reason);
			return false;
		}

		size_t request_size = request_.ByteSizeLong();
		void *request_buffer = nullptr;

		result = g_host_ops.memory.allocate(mem_mgr, request_size, &request_buffer);
		if (result != 0)
		{
			g_host_ops.memory.destroy(mem_mgr);
			error_reason = std::string("Failed to allocate memory for ") + method_name;
			spdlog::error("[gRPC Plugin] {}", error_reason);
			return false;
		}

		if (request_size > 0)
		{
			request_.SerializeToArray(request_buffer, request_size);
		}

		void *response_buffer = nullptr;
		size_t response_size = 0;
		void *error_buffer = nullptr;
		size_t error_size = 0;

		result = g_rpc_handler(mem_mgr, rpc_method_id, request_buffer, request_size,
							   &response_buffer, &response_size, &error_buffer, &error_size);

		if (result != 0)
		{
			g_host_ops.memory.destroy(mem_mgr);
			error_reason = std::string(method_name) + " call failed: " +
						   (g_host_ops.get_last_error ? g_host_ops.get_last_error(result) : "unknown error");
			spdlog::error("[gRPC Plugin] {}", error_reason);
			return false;
		}

		if (error_buffer != nullptr && error_size > 0)
		{
			if (!generic_error.ParseFromArray(error_buffer, error_size))
			{
				spdlog::warn("[gRPC Plugin] Failed to parse GenericError for {}", method_name);
			}
		}

		if (!reply_.ParseFromArray(response_buffer, response_size))
		{
			g_host_ops.memory.destroy(mem_mgr);
			error_reason = std::string("Failed to parse response for ") + method_name;
			spdlog::error("[gRPC Plugin] {}", error_reason);
			return false;
		}

		g_host_ops.memory.destroy(mem_mgr);
		return true;
	}

	TService *service_;
	ServerCompletionQueue *cq_;
	ServerContext ctx_;
	TRequest request_;
	TReply reply_;
	ServerAsyncResponseWriter<TReply> responder_;

private:
	enum CallStatus
	{
		CREATE,
		PROCESS,
		FINISH
	};
	CallStatus status_;
};

template <typename TRequest, typename TReply, typename TService, int UniqueID>
class GenericABIProxyHandler : public AsyncCallABIProxyHandler<TRequest, TReply, GenericABIProxyHandler<TRequest, TReply, TService, UniqueID>, TService>
{
public:
	using Base = AsyncCallABIProxyHandler<TRequest, TReply, GenericABIProxyHandler<TRequest, TReply, TService, UniqueID>, TService>;
	using RegisterFunc = void (*)(TService *, ServerContext *, TRequest *, ServerAsyncResponseWriter<TReply> *, ServerCompletionQueue *, ServerCompletionQueue *, void *);

	GenericABIProxyHandler(TService *service, ServerCompletionQueue *cq)
		: Base(service, cq)
	{
	}

	static void RegisterRequest(TService *service, ServerContext *ctx,
								TRequest *request, ServerAsyncResponseWriter<TReply> *responder,
								ServerCompletionQueue *cq, ServerCompletionQueue *notify_cq, void *tag)
	{
		s_register_func(service, ctx, request, responder, cq, notify_cq, tag);
	}

	int32_t GetRPCMethodID() const { return s_method_id; }
	const char *GetRPCMethodName() const { return s_method_name; }

	static RegisterFunc s_register_func;
	static int32_t s_method_id;
	static const char *s_method_name;
};

template <typename TRequest, typename TReply, typename TService, int UniqueID>
typename GenericABIProxyHandler<TRequest, TReply, TService, UniqueID>::RegisterFunc
	GenericABIProxyHandler<TRequest, TReply, TService, UniqueID>::s_register_func = nullptr;

template <typename TRequest, typename TReply, typename TService, int UniqueID>
int32_t GenericABIProxyHandler<TRequest, TReply, TService, UniqueID>::s_method_id = 0;

template <typename TRequest, typename TReply, typename TService, int UniqueID>
const char *GenericABIProxyHandler<TRequest, TReply, TService, UniqueID>::s_method_name = nullptr;

template <typename TRequest, typename TReply, typename TService, int UniqueID>
void CreateABIProxyHandler(
	TService *service,
	ServerCompletionQueue *cq,
	int32_t rpc_method_id,
	const char *method_name,
	typename GenericABIProxyHandler<TRequest, TReply, TService, UniqueID>::RegisterFunc register_func)
{
	using Handler = GenericABIProxyHandler<TRequest, TReply, TService, UniqueID>;

	Handler::s_register_func = register_func;
	Handler::s_method_id = rpc_method_id;
	Handler::s_method_name = method_name;

	new Handler(service, cq);
}

#endif /* GRPC_PLUGIN_CALL_BASE_H */
