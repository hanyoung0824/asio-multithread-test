#pragma once

#include "Packet.hpp"
#include "PacketReader.hpp"

#include <memory>

class Session;

// 세션이 받은 패킷을 처리하는 역할 (싱글톤).
// 패킷 종류(type)에 따라 처리 함수를 나눈다. 모르는 종류나 형식이 잘못된 패킷이 오면 그 세션을 끊는다.
//
// 지금은 io_context.run()을 실행하는 스레드 하나에서만 호출된다.
class PacketHandler
{
public:
	static PacketHandler& instance();

	PacketHandler(const PacketHandler&) = delete;
	PacketHandler& operator=(const PacketHandler&) = delete;

	// session이 보낸 패킷 하나를 처리한다. reader는 본문을 가리킨다.
	void handle(const std::shared_ptr<Session>& session, PacketType type, PacketReader& reader);

private:
	PacketHandler() = default;

	// Chat: 보낸 사람을 붙여 ChatNotify로 모든 클라이언트에게 보낸다.
	void handleChat(const std::shared_ptr<Session>& session, PacketReader& reader);
};
