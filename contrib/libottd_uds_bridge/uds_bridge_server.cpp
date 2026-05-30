#include "uds_bridge_server.hpp"

#include <spdlog/spdlog.h>

#include <cerrno>
#include <cstring>
#include <filesystem>
#include <unistd.h>

namespace
{

bool EnsureParentDirectory(const std::string &socket_path)
{
	const std::filesystem::path path(socket_path);
	if (path.has_parent_path())
	{
		std::error_code ec;
		std::filesystem::create_directories(path.parent_path(), ec);
		if (ec)
		{
			spdlog::error("[UDS Bridge] Failed to create socket directory {}: {}", path.parent_path().string(), ec.message());
			return false;
		}
	}
	return true;
}

void RemoveSocketFile(const std::string &socket_path)
{
	if (socket_path.empty())
	{
		return;
	}

	if (::unlink(socket_path.c_str()) != 0 && errno != ENOENT)
	{
		spdlog::warn("[UDS Bridge] Failed to remove stale socket {}: {}", socket_path, std::strerror(errno));
	}
}

} // namespace

UdsBridgeServer::UdsBridgeServer() = default;

UdsBridgeServer::~UdsBridgeServer()
{
	Shutdown();
}

bool UdsBridgeServer::Start(const std::string &socket_path)
{
	if (acceptor_ != nullptr)
	{
		return true;
	}

	if (!EnsureParentDirectory(socket_path))
	{
		return false;
	}

	RemoveSocketFile(socket_path);

	try
	{
		boost::asio::local::stream_protocol::endpoint endpoint(socket_path);
		acceptor_ = std::make_unique<boost::asio::local::stream_protocol::acceptor>(io_context_, endpoint);
		acceptor_->set_option(boost::asio::socket_base::reuse_address(true));
		acceptor_->non_blocking(true);
		socket_path_ = socket_path;
		spdlog::info("[UDS Bridge] Listening on {}", socket_path);
		return true;
	}
	catch (const std::exception &e)
	{
		spdlog::error("[UDS Bridge] Failed to bind socket {}: {}", socket_path, e.what());
		acceptor_.reset();
		return false;
	}
}

void UdsBridgeServer::Shutdown()
{
	if (acceptor_ != nullptr)
	{
		boost::system::error_code ec;
		acceptor_->close(ec);
		acceptor_.reset();
	}

	RemoveSocketFile(socket_path_);
	socket_path_.clear();
}

int32_t UdsBridgeServer::Poll(HostOps &host_ops)
{
	if (acceptor_ == nullptr || host_ops.handle_rpc == nullptr)
	{
		return 0;
	}

	int32_t processed = 0;

	while (true)
	{
		boost::system::error_code ec;
		boost::asio::local::stream_protocol::socket client(io_context_);
		acceptor_->accept(client, ec);

		if (ec == boost::asio::error::would_block || ec == boost::asio::error::try_again)
		{
			break;
		}

		if (ec)
		{
			spdlog::warn("[UDS Bridge] Accept failed: {}", ec.message());
			break;
		}

		if (ProcessClient(client, host_ops))
		{
			processed++;
		}
	}

	return processed;
}

bool UdsBridgeServer::ProcessClient(boost::asio::local::stream_protocol::socket &client, HostOps &host_ops)
{
	int32_t method_id = 0;
	std::vector<uint8_t> request;
	if (!ottd::ipc::ReadRpcRequest(client, method_id, request))
	{
		spdlog::warn("[UDS Bridge] Failed to read RPC request");
		return false;
	}

	ScopedMemoryManager *mem_mgr = nullptr;
	int32_t create_result = host_ops.memory.create(&mem_mgr);
	if (create_result != 0 || mem_mgr == nullptr)
	{
		ottd::ipc::RpcResponse rpc_response{};
		rpc_response.status = create_result != 0 ? create_result : 103;
		ottd::ipc::WriteRpcResponse(client, rpc_response);
		return false;
	}

	void *request_buffer = nullptr;
	if (!request.empty())
	{
		int32_t alloc_result = host_ops.memory.allocate(mem_mgr, request.size(), &request_buffer);
		if (alloc_result != 0)
		{
			host_ops.memory.destroy(mem_mgr);
			ottd::ipc::RpcResponse rpc_response{};
			rpc_response.status = alloc_result;
			ottd::ipc::WriteRpcResponse(client, rpc_response);
			return false;
		}
		std::memcpy(request_buffer, request.data(), request.size());
	}

	void *response_buffer = nullptr;
	size_t response_size = 0;
	void *error_buffer = nullptr;
	size_t error_size = 0;

	int32_t status = host_ops.handle_rpc(mem_mgr, method_id, request_buffer, request.size(),
										 &response_buffer, &response_size, &error_buffer, &error_size);

	ottd::ipc::RpcResponse rpc_response{};
	rpc_response.status = status;
	if (response_buffer != nullptr && response_size > 0)
	{
		const auto *response_bytes = static_cast<const uint8_t *>(response_buffer);
		rpc_response.response.assign(response_bytes, response_bytes + response_size);
	}
	if (error_buffer != nullptr && error_size > 0)
	{
		const auto *error_bytes = static_cast<const uint8_t *>(error_buffer);
		rpc_response.error.assign(error_bytes, error_bytes + error_size);
	}

	host_ops.memory.destroy(mem_mgr);

	if (!ottd::ipc::WriteRpcResponse(client, rpc_response))
	{
		spdlog::warn("[UDS Bridge] Failed to write RPC response for method {}", method_id);
		return false;
	}

	return true;
}
