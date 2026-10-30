#pragma once

#include "IPacketHandler.hpp"
#include "Session.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

// 이 프로젝트의 패킷 형식(Protocol.hpp)으로 서버와 주고받는 세션 (콘텐츠 계층).
// 범용 Session을 상속해, 받은 바이트를 패킷으로 잘라 id로 등록된 패킷 클래스(IPacketHandler)에 넘긴다.
// 형식이 잘못됐거나, 등록되지 않은 id이거나, 처리에 실패하면 스스로 연결을 끊는다.
class PacketSession : public Session
{
public:
	using Session::Session;

	// 패킷 클래스를 등록한다. id는 handler->getId()로 정해진다.
	// 같은 id가 이미 등록돼 있으면 등록하지 않고 false를 반환한다 (두 클래스에 같은 id를 준 실수).
	// 접속하기 전에 모두 등록한다.
	static bool addHandler(std::unique_ptr<IPacketHandler> handler);

protected:
	// 받은 바이트에서 완성된 패킷을 모두 처리하고, 처리한 바이트 수를 반환한다.
	// 끝에 덜 온 패킷이 있으면 그 앞까지만 처리한다 (나머지는 Session이 보관했다가 다음에 다시 넘긴다).
	std::size_t onReceive(std::span<const std::uint8_t> data) override;
};
