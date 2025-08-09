#include "client/connection_manager.h"
#include <cstring>

connection_manager::connection_manager(boost::asio::io_context& io_context,
									   const std::string& username,
									   const std::string& password)
	: resolver_(io_context)
	, socket_(io_context)
	, stdin_(io_context, ::dup(STDIN_FILENO))
	, username_(username)
	, password_(password)
	, msg_seq_(0)
{ }

void connection_manager::start()
{
	resolve_connection();
}

void connection_manager::resolve_connection()
{
	auto self(shared_from_this());
	resolver_.async_resolve("127.0.0.1",
							"12345",
							[this, self](const boost::system::error_code& ec,
										 const tcp::resolver::results_type& endpoint) {
								if(!ec)
								{
									establish_connection(endpoint);
								}
								else
								{
									std::cerr << "Resolve error: " << ec.message() << "\n";
									return;
								}
							});
}

void connection_manager::establish_connection(const tcp::resolver::results_type& endpoint)
{
	auto self(shared_from_this());
	boost::asio::async_connect(
		socket_, endpoint, [this, self](const boost::system::error_code& ec, const tcp::endpoint&) {
			if(!ec)
			{
				std::cout << "Connected to server.\n";
				send_login_request();
				read_packet_header();
			}
			else
			{
				std::cerr << "Connect error: " << ec.message() << "\n";
				return;
			}
		});
}

void connection_manager::send_login_request()
{
	LoginRequest login;
	login.header.msg_type = LOGIN_REQUEST;
	login.header.msg_seq = msg_seq_++;
	login.header.msg_size = sizeof(LoginRequest);

	std::memset(login.username, 0, sizeof(login.username));
	std::strncpy(login.username, username_.c_str(), 28); // ensure we copy only 28 bytes with null term

	std::memset(login.password, 0, sizeof(login.password));
	std::strncpy(login.password, password_.c_str(), 4); // same but 4

	auto checksum = [](const char* data, size_t max_len) -> uint8_t {
		uint32_t sum = 0;
		for(size_t i = 0; i < max_len && data[i] != '\0'; ++i)
			sum += static_cast<unsigned char>(data[i]);
		return static_cast<uint8_t>(sum & 0xFF);
	};
	username_sum_ = checksum(login.username, sizeof(login.username));
	password_sum_ = checksum(login.password, sizeof(login.password));

	login.header.msg_size = htons(login.header.msg_size);

	auto buffer = std::make_shared<std::vector<char>>(
		reinterpret_cast<char*>(&login), reinterpret_cast<char*>(&login) + sizeof(LoginRequest));

	auto self(shared_from_this());
	boost::asio::async_write(
		socket_,
		boost::asio::buffer(*buffer),
		[this, self, buffer](const boost::system::error_code& ec, std::size_t) {
			if(ec)
			{
				std::cerr << "Login send error: " << ec.message() << "\n";
				return;
			}
		});
}

void connection_manager::read_packet_header()
{
	auto self(shared_from_this());
	boost::asio::async_read(socket_,
							boost::asio::buffer(&in_header_, sizeof(PacketHeader)),
							[this, self](const boost::system::error_code& ec, std::size_t length) {
								if(!ec && length == sizeof(PacketHeader))
								{
									in_header_.msg_size = ntohs(in_header_.msg_size);

									read_packet_body();
								}
								else
								{
									if(ec != boost::asio::error::operation_aborted)
									{
										std::cerr << "Header read error: " << ec.message() << "\n";
										return;
									}
								}
							});
}

void connection_manager::read_packet_body()
{
	auto self(shared_from_this());
	size_t body_size = in_header_.msg_size - sizeof(PacketHeader);

	if(body_size > max_length)
	{
		std::cerr << "Packet too large: " << body_size << " bytes\n";
		return;
	}

	in_body_.resize(body_size);

	boost::asio::async_read(socket_,
							boost::asio::buffer(in_body_),
							[this, self](const boost::system::error_code& ec, std::size_t length) {
								if(!ec)
								{
									handle_server_packet();
									read_packet_header();
								}
								else
								{
									std::cerr << "Body read error: " << ec.message() << "\n";
									return;
								}
							});
}

void connection_manager::handle_server_packet()
{
	switch(in_header_.msg_type)
	{
	case LOGIN_RESPONSE:
		handle_login_response();
		break;
	case ECHO_RESPONSE:
		handle_echo_response();
		break;
	default:
		std::cerr << "Unknown message type: " << static_cast<int>(in_header_.msg_type) << "\n";
		break;
	}
}

void connection_manager::handle_login_response()
{
	if(in_body_.size() < sizeof(LoginResponse) - sizeof(PacketHeader))
	{
		std::cerr << "Invalid login response size\n";
		return;
	}

	uint16_t status_code =
		(static_cast<uint16_t>(static_cast<unsigned char>(in_body_.data()[0])) << 8 |
		 static_cast<uint16_t>(static_cast<unsigned char>(in_body_.data()[1])));

	if(status_code == 1)
	{
		std::cout << "Login to server successful !\n";
		std::cout << "Enter message: ";
		std::cout.flush();
		start_reading_input();
	}
	else
	{
		std::cerr << "Login failed with status: " << status_code << "\n";
		socket_.close();
		return;
	}
}

void connection_manager::handle_echo_response()
{
	if(in_body_.size() < sizeof(uint16_t))
	{
		std::cerr << "Invalid echo response size\n";
		return;
	}

	uint16_t msg_size = ntohs(*reinterpret_cast<const uint16_t*>(in_body_.data()));
	if(msg_size > in_body_.size() - sizeof(uint16_t))
	{
		std::cerr << "Message size mismatch" << in_body_.size() << " | " << msg_size << "\n";
		return;
	}

	const char* msg_start = in_body_.data() + sizeof(uint16_t);
	std::string message(msg_start, msg_size);

	std::cout << "Server responded with: " << message << "\n";
	std::cout << "Enter message: ";
	std::cout.flush();
}

void connection_manager::start_reading_input()
{
	auto self(shared_from_this());
	stdin_.async_read_some(boost::asio::buffer(input_buffer_),
						   [this, self](const boost::system::error_code& ec, std::size_t length) {
							   if(!ec)
							   {
								   std::string input(input_buffer_.data(), length);

								   if(!input.empty() && input.back() == '\n')
								   {
									   input.pop_back();
								   }

								   if(!input.empty())
								   {
									   send_echo_request(input);
								   }

								   start_reading_input();
							   }
							   else if(ec != boost::asio::error::operation_aborted)
							   {
								   std::cerr << "Input error: " << ec.message() << "\n";
							   }
						   });
}

void connection_manager::send_echo_request(const std::string& message)
{
	EchoRequest header{};
	header.header.msg_type = ECHO_REQUEST;
	header.header.msg_seq = msg_seq_;
	uint16_t payload_len = static_cast<uint16_t>(message.size());
	header.msg_size = payload_len;

	uint8_t username_sum = username_sum_;
	uint8_t password_sum = password_sum_;

	uint32_t key_state = compute_initial_key(static_cast<uint32_t>(msg_seq_),
											 static_cast<uint32_t>(username_sum),
											 static_cast<uint32_t>(password_sum));

	std::vector<uint8_t> cipher = xor_operation(payload_len, key_state, message);

	uint16_t total_size =
		static_cast<uint16_t>(sizeof(PacketHeader) + sizeof(uint16_t) + payload_len);
	header.header.msg_size = htons(total_size);
	header.msg_size = htons(payload_len);
	msg_seq_++;

	auto buffer = std::make_shared<std::vector<char>>();
	buffer->reserve(total_size);
	buffer->insert(buffer->end(),
				   reinterpret_cast<const char*>(&header),
				   reinterpret_cast<const char*>(&header) + sizeof(EchoRequest));
	buffer->insert(buffer->end(), cipher.begin(), cipher.end());

	auto self(shared_from_this());
	boost::asio::async_write(
		socket_,
		boost::asio::buffer(*buffer),
		[this, self, buffer](const boost::system::error_code& ec, std::size_t) {
			if(ec)
			{
				std::cerr << "Echo send error: " << ec.message() << "\n";
				return;
			}
		});
}