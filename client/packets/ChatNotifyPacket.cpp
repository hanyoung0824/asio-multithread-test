#include "packets/ChatNotifyPacket.hpp"

#include <iostream>
#include <string>

bool ChatNotifyPacket::handle(const std::shared_ptr<PacketSession>&, PacketReader& reader)
{
	std::string sender;
	std::string message;
	if (!reader.readString(sender) || !reader.readString(message) || !reader.isEnd()) {
		return false;
	}

	std::cout << '[' << sender << "] " << message << '\n' << std::flush;
	return true;
}
