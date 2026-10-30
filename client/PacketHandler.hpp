#pragma once

#include "Packet.hpp"
#include "PacketReader.hpp"

#include <memory>

class Session;

// 서버에게 받은 패킷을 처리하는 역할 (싱글톤).
// 패킷 종류(type)에 따라 처리 함수를 나눈다. 모르는 종류나 형식이 잘못된 패킷이 오면 연결을 끊는다.
//
// 네트워크 스레드에서만 호출된다.
class PacketHandler
{
public:
	static PacketHandler& instance();

	PacketHandler(const PacketHandler&) = delete;
	PacketHandler& operator=(const PacketHandler&) = delete;

	// 서버가 보낸 패킷 하나를 처리한다. reader는 본문을 가리킨다.
	void handle(const std::shared_ptr<Session>& session, PacketType type, PacketReader& reader);

private:
	PacketHandler() = default;

	// ChatNotify: "[보낸 사람] 메시지"로 출력한다.
	void handleChatNotify(const std::shared_ptr<Session>& session, PacketReader& reader);
};
