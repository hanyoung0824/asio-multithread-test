#include "packets/RegisterPackets.hpp"

#include "PacketSession.hpp"
#include "packets/ChatPacket.hpp"

#include <memory>

void registerPackets()
{
	PacketSession::addHandler(std::make_unique<ChatPacket>());
}
