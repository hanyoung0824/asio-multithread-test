#pragma once

#include "PacketReader.hpp"

#include <cstdint>
#include <memory>

class PacketSession;

// 패킷 종류 하나를 처리하는 인터페이스 (콘텐츠 계층).
// 패킷 종류마다 이 인터페이스를 구현한 클래스를 만들고 PacketSession::addHandler로 등록한다.
// 어떤 id를 처리할지는 클래스가 getId()로 직접 밝힌다 (id와 클래스의 짝이 한 곳에만 있도록).
class IPacketHandler
{
public:
	virtual ~IPacketHandler() = default;

	// 이 클래스가 처리할 패킷 id
	virtual std::uint16_t getId() const = 0;

	// session이 보낸 패킷의 본문(id 뒤)을 해석해서 처리한다.
	// 형식이 잘못됐으면 false를 반환한다 (PacketSession이 연결을 끊는다).
	virtual bool handle(const std::shared_ptr<PacketSession>& session, PacketReader& reader) = 0;
};
