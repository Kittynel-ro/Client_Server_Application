/**
* @file session_manager.h
* @brief Manages new connections.
*
* This class creates a 'session' object for each accepted
* connection and ensures the server listens for other connections.
*/

#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include "session.h"

using boost::asio::ip::tcp;

class session_manager
{
public:
	session_manager(boost::asio::io_context& io_context);

private:
	tcp::acceptor acceptor_;

	/**
	* @brief Waits for new client connections
	*/
	void do_accept();
};

#endif // SESSION_MANAGER_H