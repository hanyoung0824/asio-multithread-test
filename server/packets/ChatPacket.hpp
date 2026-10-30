#pragma once

#include "IPacketHandler.hpp"
#include "Protocol.hpp"

// Chat (클라이언트 -> 서버): message(string)
// 보낸 사람을 붙여 ChatNotify로 모든 클라이언트에게 보낸다.
class ChatPacket : public IPacketHandler
{
public:
	std::uint16_t getId() const override { return static_cast<std::uint16_t>(PacketId::Chat); }
	bool handle(const std::shared_ptr<PacketSession>& session, PacketReader& reader) override;
};
