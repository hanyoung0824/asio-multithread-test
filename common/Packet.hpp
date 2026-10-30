#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

// 패킷 = 헤더(4바이트) + 본문
//
//   | size (2바이트) | type (2바이트) | body (size - 4 바이트) |
//
// - size: 헤더를 포함한 패킷 전체 길이. 받는 쪽은 이 값으로 패킷 하나의 끝을 안다.
// - type: 무슨 패킷인지 (PacketType)
// - 모든 정수는 little-endian. 문자열은 길이(2바이트) + 바이트들.
//
// 서버와 클라이언트가 이 정의를 함께 쓴다.

enum class PacketType : std::uint16_t
{
	Chat = 1, // 클라이언트 -> 서버: message(string)
	ChatNotify = 2, // 서버 -> 모든 클라이언트: sender(string), message(string)
};

// 헤더 크기
constexpr std::size_t packetHeaderSize = 4;

// 패킷 하나의 최대 크기 (헤더 포함). 이보다 크다는 헤더가 오면 연결을 끊는다.
// 제한이 없으면 악의적인 상대가 큰 size만 보내 받는 쪽 메모리를 잡아먹게 할 수 있다.
constexpr std::size_t maxPacketSize = 4096;

// 채팅 메시지 최대 길이 (바이트). ChatNotify(보낸 사람 + 메시지)가 maxPacketSize를 넘지 않도록 넉넉히 작게 잡는다.
constexpr std::size_t maxChatMessageLength = 1024;

// 패킷 하나를 담는 바이트 배열 (헤더 포함)
using PacketBuffer = std::vector<std::uint8_t>;

struct PacketHeader
{
	std::uint16_t size; // 헤더 포함 전체 길이
	PacketType type;
};

// 받은 헤더 4바이트를 해석한다.
inline PacketHeader parseHeader(const std::array<std::uint8_t, packetHeaderSize>& data)
{
	return {
		static_cast<std::uint16_t>(data[0] | (data[1] << 8)),
		static_cast<PacketType>(data[2] | (data[3] << 8)),
	};
}
