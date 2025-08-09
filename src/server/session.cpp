#include "server/session.h"
#include "utils/crypto.hpp"

using boost::asio::ip::tcp;

session::session(tcp::socket socket)
	: socket_(std::move(socket))
	, client_id("default")
{ }

void session::start()
{
	do_read_header();
}

void session::do_read_header()
{
	auto self(shared_from_this());
	boost::asio::async_read(socket_,
							boost::asio::buffer(&in_header_, sizeof(PacketHeader)),
							[this, self](boost::system::error_code ec, std::size_t length) {
								if(!ec && length == sizeof(PacketHeader))
								{
									in_header_.msg_size = ntohs(in_header_.msg_size);

									if(in_header_.msg_size > max_length)
									{
										std::cerr << "Invalid header size: "
												  << static_cast<int>(in_header_.msg_size) << "\n";
										return;
									}

									do_read_body();
								}
							});
}

void session::do_read_body()
{
	auto self(shared_from_this());
	in_body_.resize(in_header_.msg_size - sizeof(PacketHeader));

	boost::asio::async_read(socket_,
							boost::asio::buffer(in_body_),
							[this, self](boost::system::error_code ec, std::size_t length) {
								if(!ec)
								{
									handle_packet();
								}
							});
}

void session::handle_packet()
{
	switch(in_header_.msg_type)
	{
	case LOGIN_REQUEST:
		handle_login();
		break;
	case ECHO_REQUEST:
		handle_echo();
		break;
	default:
		std::cerr << "Unknown message type: " << static_cast<int>(in_header_.msg_type) << "\n";
		break;
	}
}

void session::handle_login()
{
	if(in_body_.size() < sizeof(LoginRequest) - sizeof(PacketHeader))
	{
		std::cerr << "Invalid login request size: " << in_body_.size() << "\n";
		return;
	}

	const char* username_data = in_body_.data();
	const char* password_data = in_body_.data() + 28;
	std::string username(username_data, 28);
	std::string password(password_data, 4);
	client_id = username;

	username_sum_ = compute_checksum_cstr(username_data, 28);
	password_sum_ = compute_checksum_cstr(password_data, 4);

	LoginResponse response;
	response.header.msg_type = LOGIN_RESPONSE;
	response.header.msg_seq = in_header_.msg_seq;

	if(username.empty() || password.empty())
	{
		response.status_code = 0;
		std::cout << "Failed login with username: " << username << " and password: " << password
				  << "\n";
	}
	else
	{
		response.status_code = 1;
		std::cout << "User: " << username << " has logged on\n";
	}

	response.header.msg_size = htons(sizeof(LoginResponse));
	response.status_code = htons(response.status_code);

	auto buffer = std::make_shared<std::array<char, sizeof(LoginResponse)>>();

	memcpy(buffer->data(), &response, sizeof(LoginResponse));

	send_packet(buffer, sizeof(LoginResponse));
}

void session::handle_echo()
{
	uint16_t payload_len = ntohs(*reinterpret_cast<const uint16_t*>(in_body_.data()));

	if(payload_len > in_body_.size() - sizeof(uint16_t))
	{
		std::cerr << "Invalid echo request size: " << payload_len << "\n";
		return;
	}

	const std::string cipher = reinterpret_cast<const char*>(in_body_.data() + sizeof(uint16_t));
	std::cout << "Ciphered payload from "<< client_id << ": ";
	print_string_as_hex(cipher);

	uint32_t key_state = compute_initial_key(static_cast<uint32_t>(in_header_.msg_seq),
											 static_cast<uint32_t>(username_sum_),
											 static_cast<uint32_t>(password_sum_));
	std::vector<uint8_t> decrypted = xor_operation(payload_len, key_state, cipher);
	std::string plain_text(decrypted.begin(), decrypted.end());

	std::cout << client_id << " sent message: " << plain_text << "\n";

	EchoResponse response_header;
	response_header.header.msg_type = ECHO_RESPONSE;
	response_header.header.msg_seq = in_header_.msg_seq;
	response_header.msg_size = htons(static_cast<uint16_t>(plain_text.size()));

	uint16_t total_size = sizeof(EchoResponse) + plain_text.size();
	response_header.header.msg_size = htons(total_size);

	auto buffer = std::make_shared<std::vector<char>>();
	buffer->reserve(total_size);

	buffer->insert(buffer->end(),
				   reinterpret_cast<char*>(&response_header),
				   reinterpret_cast<char*>(&response_header) + sizeof(EchoResponse));

	buffer->insert(buffer->end(), plain_text.begin(), plain_text.end());

	send_packet(buffer, total_size);
}

template <typename buffer_T>
void session::send_packet(std::shared_ptr<buffer_T> buffer, size_t length)
{
	auto self(shared_from_this());
	boost::asio::async_write(
		socket_,
		boost::asio::buffer(*buffer, length),
		[this, self, buffer](boost::system::error_code ec, std::size_t length) {
			if(!ec)
			{
				do_read_header();
			}
		});
}
