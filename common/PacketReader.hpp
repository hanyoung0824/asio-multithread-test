#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

// 받은 패킷의 본문을 앞에서부터 차례로 읽는 역할.
// 받은 데이터는 믿을 수 없으므로, 읽기마다 남은 길이를 확인하고 모자라면 false를 반환한다.
// (예: "길이 1000짜리 문자열"이라고 해 놓고 5바이트만 보낸 경우에도 버퍼 밖을 읽지 않는다.)
//
//   PacketReader reader(body);
//   std::string message;
//   if (!reader.readString(message) || !reader.isEnd()) { /* 잘못된 패킷 */ }
class PacketReader
{
public:
	// body: 헤더를 뺀 본문. PacketReader는 복사하지 않고 가리키기만 하므로 읽는 동안 살아 있어야 한다.
	explicit PacketReader(std::span<const std::uint8_t> body);

	bool readUInt16(std::uint16_t& out);
	bool readString(std::string& out);

	// 본문을 끝까지 다 읽었는지. 남은 데이터가 있으면 형식이 맞지 않는 패킷이다.
	bool isEnd() const { return m_position == m_body.size(); }

private:
	std::size_t remaining() const { return m_body.size() - m_position; }

private:
	std::span<const std::uint8_t> m_body; // 읽을 본문
	std::size_t m_position = 0; // 다음에 읽을 위치
};
