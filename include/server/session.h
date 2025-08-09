/**
* @file session.h
* @brief Class definition for connections.
*
* This header contains the session class which define what operations
* can be handled by the server and what responses to give based
* on the request received from the clients.
*/

#ifndef SESSION_H
#define SESSION_H

#include <boost/asio.hpp>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <ostream>

#include "utils/crypto.hpp"
#include "utils/types.h"

using boost::asio::ip::tcp;

class session : public std::enable_shared_from_this<session>
{
public:
	session(tcp::socket socket);
	
	/// @brief Starts the state machine of the server
	void start();

private:
	tcp::socket socket_;
	PacketHeader in_header_;
	std::vector<char> in_body_;
	std::string client_id;
	uint8_t username_sum_{0};
	uint8_t password_sum_{0};

	static constexpr size_t max_length = 512;

	/// @brief Reads from the socket the length of the header and if no error is caught
	/// transalte form network to host and proceed in reading the body.
	void do_read_header();

	/// @brief Reads from the socket the size of the payload and ensures the server handles it.
	void do_read_body();

	/// @brief Decides how to process the packet data based on it's type.
	void handle_packet();

	/// @brief Since all credentials are accepted we just compute the checksums
	///  and send back a confirmation packet to tell the client that he logged in.
	void handle_login();

	/// @brief Based on the provided LCG variant compute the key to decrypt the cipher
	/// received from the client and send it back to the client (echo).
	void handle_echo();

	/// @brief Method that sends a response to the client and ensures our server goes
	/// in the state to read a new packet.
	/// @param buffer A buffer_T type owned pointer that accepts different buffers
	/// @param length A size_t variable that tells how much to write to the socket
	template <typename buffer_T>
	void send_packet(std::shared_ptr<buffer_T> buffer, size_t length);
};

#endif // SESSION_H