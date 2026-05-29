#ifndef OTTD_UDS_IPC_PROTOCOL_HPP
#define OTTD_UDS_IPC_PROTOCOL_HPP

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>

namespace ottd::ipc
{

constexpr std::array<char, 4> kMagic = {'O', 'T', 'R', 'P'};
constexpr uint8_t kProtocolVersion = 1;

struct RequestHeader
{
	char magic[4];
	uint8_t version;
	int32_t method_id;
	uint32_t request_size;
};

struct ResponseHeader
{
	char magic[4];
	uint8_t version;
	int32_t status;
	uint32_t response_size;
	uint32_t error_size;
};

inline bool HeaderMagicValid(const char magic[4])
{
	return std::memcmp(magic, kMagic.data(), kMagic.size()) == 0;
}

inline bool ReadExact(boost::asio::local::stream_protocol::socket &socket, void *data, std::size_t size)
{
	std::size_t total = 0;
	auto *bytes = static_cast<uint8_t *>(data);

	while (total < size)
	{
		boost::system::error_code ec;
		std::size_t n = socket.read_some(boost::asio::buffer(bytes + total, size - total), ec);
		if (ec)
		{
			return false;
		}
		total += n;
	}

	return true;
}

inline bool WriteExact(boost::asio::local::stream_protocol::socket &socket, const void *data, std::size_t size)
{
	std::size_t total = 0;
	const auto *bytes = static_cast<const uint8_t *>(data);

	while (total < size)
	{
		boost::system::error_code ec;
		std::size_t n = socket.write_some(boost::asio::buffer(bytes + total, size - total), ec);
		if (ec)
		{
			return false;
		}
		total += n;
	}

	return true;
}

inline void FillRequestHeader(RequestHeader &header, int32_t method_id, uint32_t request_size)
{
	std::memcpy(header.magic, kMagic.data(), kMagic.size());
	header.version = kProtocolVersion;
	header.method_id = method_id;
	header.request_size = request_size;
}

inline void FillResponseHeader(ResponseHeader &header, int32_t status, uint32_t response_size, uint32_t error_size)
{
	std::memcpy(header.magic, kMagic.data(), kMagic.size());
	header.version = kProtocolVersion;
	header.status = status;
	header.response_size = response_size;
	header.error_size = error_size;
}

struct RpcResponse
{
	int32_t status = 0;
	std::vector<uint8_t> response;
	std::vector<uint8_t> error;
};

inline bool ReadRpcRequest(boost::asio::local::stream_protocol::socket &socket, int32_t &method_id,
						   std::vector<uint8_t> &request)
{
	RequestHeader header{};
	if (!ReadExact(socket, &header, sizeof(header)))
	{
		return false;
	}

	if (!HeaderMagicValid(header.magic) || header.version != kProtocolVersion)
	{
		return false;
	}

	method_id = header.method_id;
	request.resize(header.request_size);
	if (header.request_size > 0 && !ReadExact(socket, request.data(), request.size()))
	{
		return false;
	}

	return true;
}

inline bool WriteRpcResponse(boost::asio::local::stream_protocol::socket &socket, const RpcResponse &rpc_response)
{
	ResponseHeader header{};
	FillResponseHeader(header, rpc_response.status,
					   static_cast<uint32_t>(rpc_response.response.size()),
					   static_cast<uint32_t>(rpc_response.error.size()));

	if (!WriteExact(socket, &header, sizeof(header)))
	{
		return false;
	}

	if (!rpc_response.response.empty() && !WriteExact(socket, rpc_response.response.data(), rpc_response.response.size()))
	{
		return false;
	}

	if (!rpc_response.error.empty() && !WriteExact(socket, rpc_response.error.data(), rpc_response.error.size()))
	{
		return false;
	}

	return true;
}

inline bool PerformRpcCall(boost::asio::local::stream_protocol::socket &socket, int32_t method_id,
						   const void *request, std::size_t request_size, RpcResponse &rpc_response)
{
	RequestHeader header{};
	FillRequestHeader(header, method_id, static_cast<uint32_t>(request_size));

	if (!WriteExact(socket, &header, sizeof(header)))
	{
		return false;
	}

	if (request_size > 0 && !WriteExact(socket, request, request_size))
	{
		return false;
	}

	ResponseHeader response_header{};
	if (!ReadExact(socket, &response_header, sizeof(response_header)))
	{
		return false;
	}

	if (!HeaderMagicValid(response_header.magic) || response_header.version != kProtocolVersion)
	{
		return false;
	}

	rpc_response.status = response_header.status;
	rpc_response.response.resize(response_header.response_size);
	rpc_response.error.resize(response_header.error_size);

	if (response_header.response_size > 0 && !ReadExact(socket, rpc_response.response.data(), rpc_response.response.size()))
	{
		return false;
	}

	if (response_header.error_size > 0 && !ReadExact(socket, rpc_response.error.data(), rpc_response.error.size()))
	{
		return false;
	}

	return true;
}

class UdsRpcClient
{
public:
	explicit UdsRpcClient(boost::asio::io_context &io_context)
		: socket_(io_context)
	{
	}

	bool Connect(const std::string &socket_path)
	{
		boost::system::error_code ec;
		boost::asio::local::stream_protocol::endpoint endpoint(socket_path);
		socket_.connect(endpoint, ec);
		return !ec;
	}

	void Close()
	{
		boost::system::error_code ec;
		socket_.close(ec);
	}

	bool Call(int32_t method_id, const void *request, std::size_t request_size, RpcResponse &rpc_response)
	{
		return PerformRpcCall(socket_, method_id, request, request_size, rpc_response);
	}

private:
	boost::asio::local::stream_protocol::socket socket_;
};

inline bool CallRpc(const std::string &socket_path, int32_t method_id, const void *request,
					std::size_t request_size, RpcResponse &rpc_response)
{
	boost::asio::io_context io_context;
	UdsRpcClient client(io_context);
	if (!client.Connect(socket_path))
	{
		return false;
	}
	return client.Call(method_id, request, request_size, rpc_response);
}

} // namespace ottd::ipc

#endif /* OTTD_UDS_IPC_PROTOCOL_HPP */
