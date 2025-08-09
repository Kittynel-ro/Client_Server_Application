#ifndef TYPES_H
#define TYPES_H

#include <cstdint>

#pragma pack(push, 1)
struct PacketHeader
{
	uint16_t msg_size;
	uint8_t msg_type;
	uint8_t msg_seq;
};

struct LoginRequest
{
	PacketHeader header;
	char username[28];
	char password[4];
};

struct LoginResponse
{
	PacketHeader header;
	uint16_t status_code;
};

// I moved the variable payload outside the network packet
struct EchoRequest
{
	PacketHeader header;
	uint16_t msg_size;
};

struct EchoResponse
{
	PacketHeader header;
	uint16_t msg_size; 
};


#pragma pack(pop)

enum MessageType : uint8_t
{
	LOGIN_REQUEST = 0,
	LOGIN_RESPONSE = 1,
	ECHO_REQUEST = 2,
	ECHO_RESPONSE = 3
};

#endif // TYPES_H