#pragma once

#include "Packet.hpp"

#include <cstdint>
#include <optional>
#include <string_view>

// 보낼 패킷을 만드는 역할.
//
//   PacketWriter writer(PacketType::Chat);
//   writer.writeString("hello");
//   std::optional<PacketBuffer> packet = writer.finish(); // 헤더의 size가 채워진 완성 패킷
class PacketWriter
{
public:
	// 헤더 자리를 비워 두고 type을 쓴다.
	explicit PacketWriter(PacketType type);

	void writeUInt16(std::uint16_t value);

	// 길이(2바이트) + 바이트들. 65535바이트보다 길면 실패로 표시된다.
	void writeString(std::string_view value);

	// 헤더의 size를 채워 완성된 패킷을 돌려준다.
	// maxPacketSize를 넘었거나 쓰는 중에 실패했으면 std::nullopt.
	std::optional<PacketBuffer> finish();

private:
	PacketBuffer m_buffer; // 헤더 + 지금까지 쓴 본문
	bool m_failed = false; // 쓰는 중에 실패했는지
};
