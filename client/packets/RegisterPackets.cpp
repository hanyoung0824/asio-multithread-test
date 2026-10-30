#include "packets/RegisterPackets.hpp"

#include "PacketSession.hpp"
#include "packets/ChatNotifyPacket.hpp"

#include <memory>

void registerPackets()
{
	PacketSession::addHandler(std::make_unique<ChatNotifyPacket>());
}
