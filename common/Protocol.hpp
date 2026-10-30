#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

// 채팅 프로젝트의 패킷 형식 (콘텐츠 계층).
// 네트워크 계층(Session)은 이 형식을 모른다. Session을 상속한 PacketSession이 받은 바이트를 이 형식으로 자른다.
//
//   | size (2바이트) | id (2바이트) | body (size - 4 바이트) |
//
// - size: 헤더를 포함한 패킷 전체 길이. 이 값으로 패킷 하나의 끝을 안다.
// - id: 무슨 패킷인지 (PacketId)
// - 모든 정수는 little-endian. 문자열은 길이(2바이트) + 바이트들.
//
// 서버와 클라이언트가 이 정의를 함께 쓴다.

// 헤더(size + id) 크기
constexpr std::size_t packetHeaderSize = 4;

// 패킷 하나의 최대 크기 (헤더 포함). 이보다 크다는 size가 오면 연결을 끊는다.
constexpr std::size_t maxPacketSize = 4096;

// 완성된 패킷(헤더 포함)을 담는 바이트 배열
using PacketBuffer = std::vector<std::uint8_t>;

enum class PacketId : std::uint16_t
{
	Chat = 1, // 클라이언트 -> 서버: message(string)
	ChatNotify = 2, // 서버 -> 모든 클라이언트: sender(string), message(string)
};

// 채팅 메시지 최대 길이 (바이트). ChatNotify(보낸 사람 + 메시지)가 maxPacketSize를 넘지 않도록 넉넉히 작게 잡는다.
constexpr std::size_t maxChatMessageLength = 1024;
