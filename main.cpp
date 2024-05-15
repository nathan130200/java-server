#include <SFML/Network.hpp>
#include "minecraft.h"

void start_client(sf::TcpSocket*);

using namespace minecraft;

int main()
{
	sf::TcpListener listener;
	listener.listen(25565, sf::IpAddress::Any);


	while (true)
	{
		auto* client = new sf::TcpSocket;

		if (listener.accept(*client))
			delete client;
		else
		{
			auto func = [](sf::TcpSocket* client) -> void
				{
					start_client(client);
					delete client;
				};

			sf::Thread thread(func, client);
			thread.launch();
		}
	}

	return 0;
}


void start_client(sf::TcpSocket* socket)
{
	int32_t length;
	CHECK_READ(read_varint32(socket, length));

	int32_t packetId, nb;
	CHECK_READ(read_varint32(socket, packetId, &nb));

	if (packetId != 0)
		return;

	printf("packet id: %d (%d)\n", packetId, nb);

	length -= nb;

	PacketHandshakeIn pkt{};
	read_packet(socket, &pkt);

	printf("protocol: %d\n", pkt.protocolVersion);
	printf("hostname: %s:%d\n", pkt.hostname.c_str(), pkt.port);
	printf("next state: %d (%s)\n", pkt.nextState, pkt.nextState == 1 ? "status"
		: (pkt.nextState == 2
			? "login" : "unknown"));

	printf("\n");
}