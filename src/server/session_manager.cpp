#include "server/session_manager.h"

using boost::asio::ip::tcp;

session_manager::session_manager(boost::asio::io_context& io_context)
	: acceptor_(io_context, tcp::endpoint(tcp::v4(), 12345))
{
	do_accept();
}

void session_manager::do_accept()
{
	/// When succesfull it provides the socket and starts the session
	acceptor_.async_accept([this](boost::system::error_code ec, tcp::socket socket) {
		if(!ec)
		{
			std::make_shared<session>(std::move(socket))->start();
		}

		do_accept();
	});
}
