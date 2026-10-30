#include "PacketSession.hpp"

#include "PacketReader.hpp"
#include "Protocol.hpp"

#include <iostream>
#include <unordered_map>

namespace
{
	// id -> 패킷 클래스. 함수 안의 static으로 두어, 처음 쓸 때 만들어지도록 한다.
	std::unordered_map<std::uint16_t, std::unique_ptr<IPacketHandler>>& handlers()
	{
		static std::unordered_map<std::uint16_t, std::unique_ptr<IPacketHandler>> instance;
		return instance;
	}
}

bool PacketSession::addHandler(std::unique_ptr<IPacketHandler> handler)
{
	const std::uint16_t id = handler->getId();

	// emplace는 같은 키가 이미 있으면 넣지 않는다 (덮어쓰지 않는다).
	if (!handlers().emplace(id, std::move(handler)).second) {
		std::cerr << "Packet id " << id << " is already registered\n";
		return false;
	}
	return true;
}

std::size_t PacketSession::onReceive(std::span<const std::uint8_t> data)
{
	// 패킷 클래스에 넘길 자기 자신. Session이 enable_shared_from_this를 상속하므로 파생 타입으로 바꿔 쓴다.
	const auto self = std::static_pointer_cast<PacketSession>(shared_from_this());

	// 한 번에 받은 바이트에 패킷이 여러 개 붙어 있을 수 있으므로, 완성된 패킷이 있는 동안 반복한다.
	std::size_t processed = 0;
	while (!isClosed()) {
		const std::span<const std::uint8_t> rest = data.subspan(processed);

		// 1. 헤더(size + id)가 다 오지 않았으면 다음에 이어서 처리한다.
		if (rest.size() < packetHeaderSize) {
			break;
		}

		PacketReader header(rest.first(packetHeaderSize));
		std::uint16_t size = 0;
		std::uint16_t id = 0;
		header.readUInt16(size);
		header.readUInt16(id);

		// 2. 서버가 보낸 데이터라도 검사한다. size가 잘못되면 이후 패킷의 경계를 알 수 없으므로 끊는다.
		if (size < packetHeaderSize || size > maxPacketSize) {
			std::cerr << "Invalid packet size " << size << " from server, closing\n";
			close();
			break;
		}

		// 3. 패킷 전체가 다 오지 않았으면 다음에 이어서 처리한다.
		if (rest.size() < size) {
			break;
		}

		// 4. 등록된 패킷 클래스 찾기
		const auto it = handlers().find(id);
		if (it == handlers().end()) {
			std::cerr << "Unknown packet id " << id << " from server, closing\n";
			close();
			break;
		}

		// 5. 처리. 본문만 넘긴다. 형식이 잘못됐으면 끊는다.
		PacketReader body(rest.subspan(packetHeaderSize, size - packetHeaderSize));
		if (!it->second->handle(self, body)) {
			std::cerr << "Invalid packet id " << id << " from server, closing\n";
			close();
			break;
		}

		processed += size;
	}
	return processed;
}
