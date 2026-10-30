#pragma once

#include "IPacketHandler.hpp"
#include "Protocol.hpp"

// ChatNotify (서버 -> 클라이언트): sender(string), message(string)
// "[보낸 사람] 메시지"로 출력한다.
class ChatNotifyPacket : public IPacketHandler
{
public:
	std::uint16_t getId() const override { return static_cast<std::uint16_t>(PacketId::ChatNotify); }
	bool handle(const std::shared_ptr<PacketSession>& session, PacketReader& reader) override;
};
