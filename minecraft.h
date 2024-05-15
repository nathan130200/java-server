#pragma once

#include <string>
#include <SFML/Network.hpp>

struct PacketHandshakeIn
{
public:
	int32_t protocolVersion;
	std::string hostname;
	uint16_t port;
	int32_t nextState;
};

namespace minecraft
{
	namespace detail
	{
		template <typename ValueType, ValueType MaxBits>
		bool _read_varint(sf::TcpSocket* socket, ValueType& value, int32_t* numBytes = nullptr)
		{
			value = (ValueType)0;

			size_t nb;
			char position = 0;
			char count = 0;
			unsigned char buf[1];

			while (true)
			{
				if (socket->receive((void*)buf, 1, nb) != sf::Socket::Done || nb != 1)
					return false;

				count++;
				value |= (buf[0] & (unsigned char)127) << position;

				if (!(buf[0] & (unsigned char)128))
					break;

				if (position >= MaxBits)
					return false;
			}

			if (numBytes != nullptr)
				*numBytes = (int32_t)count;

			return true;
		}
	}

	template<typename ValueType, size_t Len = sizeof(ValueType)>
	bool read_value(sf::TcpSocket* socket, ValueType& result)
	{
		char buf[Len];

		size_t numRead;

		if (socket->receive(buf, Len, numRead) != sf::Socket::Done || numRead != Len)
			return false;

		{
			sf::Packet p;
			p.append(buf, Len);
			p >> result;
		}

		return true;
	}

	bool read_varint32(sf::TcpSocket* socket, int32_t& result, int32_t* numBytes = nullptr)
	{
		return detail::_read_varint<int32_t, 32>(socket, result, numBytes);
	}

	bool read_varint64(sf::TcpSocket* socket, int64_t& result, int32_t* numBytes = nullptr)
	{
		return detail::_read_varint<int64_t, 64L>(socket, result, numBytes);
	}

	bool read_string(sf::TcpSocket* socket, std::string& result)
	{
		int32_t len;

		if (!read_varint32(socket, len))
			return false;

		auto ptr = new char[len];

		size_t numBytes;

		if (socket->receive((void*)ptr, len, numBytes) != sf::Socket::Done || numBytes != len)
		{
			delete[] ptr;
			return false;
		}

		result.assign(ptr, len);

		delete[] ptr;

		return true;
	}

	template<size_t Len>
	bool read_byte_array(sf::TcpSocket* socket, char out[Len])
	{
		size_t n;
		if (socket->receive((void*)out, Len, n) != sf::Socket::Done
			|| n != Len)
			return false;

		return true;
	}

	enum packet_result
	{
		ok,
		failed
	};

	template<typename PacketType>
	packet_result read_packet(sf::TcpSocket* socket, PacketType* result)
	{
		return failed;
	}
}

#define IMPLEMENT_DESERIALIZER(Type) template<>\
minecraft::packet_result minecraft::read_packet(sf::TcpSocket* socket, Type* result)

#define CHECK_READ_VALUE(Val) if (!Val) {\
	printf(#Val ": failed\n ");\
	return packet_result::failed;\
}

#define CHECK_READ(Val) if (!Val) {\
	printf(#Val ": failed\n ");\
	return;\
}

#define READ_SOCKET(SocketPtr, Buf, Len) do {\
	size_t numRead;\
	if (SocketPtr->receive((void*)Buf, (size_t)Len, numRead) != sf::Socket::Done || numRead != Len) {\
		printf("socket->receive() failed\n");\
		return;\
	}\
} while(0);

IMPLEMENT_DESERIALIZER(PacketHandshakeIn)
{
	CHECK_READ_VALUE(minecraft::read_varint32(socket, result->protocolVersion));
	CHECK_READ_VALUE(minecraft::read_string(socket, result->hostname));
	CHECK_READ_VALUE(minecraft::read_value<uint16_t>(socket, result->port));
	CHECK_READ_VALUE(minecraft::read_varint32(socket, result->nextState));
}
