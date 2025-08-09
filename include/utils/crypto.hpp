/**
* @file crypto.hpp
* @brief This header contains methods for cryptographic operations.
*/

#ifndef CRYPTO_HPP
#define CRYPTO_HPP

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

/// @brief This returns a key based on provided LCG.
/// @param key This is the initial key or the previous key
inline uint32_t next_key(uint32_t key)
{
	return (key * 1103515245ul + 12345ul) % 0x7FFFFFFFul;
}

/// @brief Compute the initial key used for the key generator.
/// @param msg_seq_ This is the current seq for the sent packet
/// @param username_sum This is the checksum from the username
/// @param password_sum This is the checksum from the password
inline uint32_t compute_initial_key(uint32_t msg_seq_, uint32_t username_sum, uint32_t password_sum)
{
	return (msg_seq_ << 16) | (username_sum << 8) | password_sum;
}

/// @brief Compute the checksum for a given c_string using the sum complement.
/// @param data This is the c_string we want to calulate for
/// @param max_len This is the maximum we can compute for if we have no null termination
inline uint8_t compute_checksum_cstr(const char* data, size_t max_len)
{
	uint8_t sum = 0;
	for(size_t i = 0; i < max_len; ++i)
	{
		if(data[i] == '\0')
		{
			break;
		}
		sum += static_cast<unsigned char>(data[i]);
	}
	return sum & 0xFF;
}

/// @brief Applies XOR between the key and the message.
/// @param payload_len This is the length that we apply xor for
/// @param key_state This is the latest key generated
/// @param message This is the string that contains our plain text
inline std::vector<uint8_t>
xor_operation(uint16_t payload_len, uint32_t key_state, const std::string& message)
{
	std::vector<uint8_t> cipher(payload_len);

	for(size_t i = 0; i < payload_len; ++i)
	{
		key_state = next_key(key_state);
		uint8_t key_byte = static_cast<uint8_t>(key_state % 256);
		cipher[i] = static_cast<uint8_t>(static_cast<uint8_t>(message[i]) ^ key_byte);
	}
	return cipher;
}

inline void print_string_as_hex(const std::string& str)
{
	std::cout << std::hex << std::uppercase << std::setfill('0');

	for(unsigned char ch : str)
	{
		std::cout << std::setw(2) << static_cast<unsigned int>(ch) << " ";
	}

	std::cout << std::dec << std::nouppercase << std::setfill(' ') << std::endl;
}

#endif // CRYPTO_HPP