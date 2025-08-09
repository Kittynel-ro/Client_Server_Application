/**
* @file connection_manager.h
* @brief Manages connection to server.
*
* This header contains the class that manages the client's 
* connection with the server and tells what requests can be
* sent to the server.
*/

#ifndef CONNECTION_MANAGER_H
#define CONNECTION_MANAGER_H

#include <boost/asio.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <iostream>
#include <memory>
#include <netinet/in.h>

#include "utils/crypto.hpp"
#include "utils/types.h"

using boost::asio::ip::tcp;
namespace posix = boost::asio::posix;

class connection_manager : public std::enable_shared_from_this<connection_manager>
{
public:
	connection_manager(boost::asio::io_context& io_context,
					   const std::string& username,
					   const std::string& password);

	void start();

private:
	tcp::resolver resolver_;
	tcp::socket socket_;
	posix::stream_descriptor stdin_;
	std::array<char, 512> input_buffer_;
	PacketHeader in_header_;
	std::vector<char> in_body_;
	std::string username_;
	std::string password_;
	uint8_t msg_seq_;
	uint8_t username_sum_{0};
	uint8_t password_sum_{0};

	static constexpr size_t max_length = 512;

	/// @brief Based on an address and a port tries to find if
	/// the connection can be established and provides an endpoint
	/// that will be associated with a socket in the callback.
	void resolve_connection();

	/// @brief Based on a provided endpoint establish a connection and
	/// associate the endpoint with a socket.
	void establish_connection(const tcp::resolver::results_type& endpoint);

	/// @brief Creates a packet that contains the username and password
	/// and sends it to the server to try and log in.
	void send_login_request();

	/// @brief Method that ensures the looping of the client, it reads
	/// from stdin to a buffer and in a callback it removes the newline,
	///	then it send the packet and goes back to the same state.
	void start_reading_input();

	/// @brief Method to send a message to the server, based on the lcg
	/// provided it computes a key to encrypt the plain text and writes
	/// it to the socket.
	void send_echo_request(const std::string& message);
	
	/// @brief Reads from the socket the length of the header and if no 
	/// error is caught transalte form network to host and proceed in 
	/// reading the body.
	void read_packet_header();
	
	/// @brief Reads the size received in the header for the payload and
	/// in a callback ensure the packet is handled and after it is handled
	/// continue reading the next packet.
	void read_packet_body();
	
	/// @brief Handles the packet based on the type received in the header.
	void handle_server_packet();
	
	/// @brief Based on the response either start reading from stdin and
	///  enable the packet processing loop or disconnect the client
	void handle_login_response();
	
	/// @brief Receive the data and prints it in stdout.
	void handle_echo_response();
};

#endif // CONNECTION_MANAGER_H