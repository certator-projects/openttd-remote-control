#ifndef UDS_BRIDGE_SERVER_HPP
#define UDS_BRIDGE_SERVER_HPP

#include "../ottd_uds_ipc/ipc_protocol.hpp"
#include "plugin_interface.h"

#include <boost/asio.hpp>

#include <memory>
#include <string>

class UdsBridgeServer
{
public:
	UdsBridgeServer();
	~UdsBridgeServer();

	bool Start(const std::string &socket_path);
	void Shutdown();
	int32_t Poll(RPCHandler handler, HostOps &host_ops);

private:
	bool ProcessClient(boost::asio::local::stream_protocol::socket &client, RPCHandler handler, HostOps &host_ops);

	boost::asio::io_context io_context_;
	std::unique_ptr<boost::asio::local::stream_protocol::acceptor> acceptor_;
	std::string socket_path_;
};

#endif /* UDS_BRIDGE_SERVER_HPP */
