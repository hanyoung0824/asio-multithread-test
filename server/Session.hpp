#pragma once

#include "Packet.hpp"
#include "PacketReader.hpp"

#include <boost/asio.hpp>

#include <array>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <string>

// 접속한 클라이언트 하나와 패킷을 주고받는 역할 (소켓 I/O만 담당).
// 받은 패킷을 어떻게 처리할지, 닫혔을 때 무엇을 할지는 모른다. 생성할 때 받은 콜백으로 알리기만 한다.
//
// 읽기와 쓰기는 서로 독립적으로 진행된다.
// - 읽기: 항상 하나가 걸려 있다. 헤더 -> 본문 순서로 패킷 하나를 받으면 콜백으로 넘기고 다시 헤더를 기다린다.
// - 쓰기: send()로 들어온 패킷을 큐에 쌓고, async_write는 한 번에 하나씩만 진행한다.
//
// 비동기 작업이 끝나기 전에 Session이 사라지면 안 되므로 반드시 std::make_shared로 만든다.
// 세션 목록과, 진행 중인 비동기 작업의 콜백이 shared_ptr로 Session을 붙잡고 있다.
class Session : public std::enable_shared_from_this<Session>
{
public:
	using SessionId = std::uint64_t;

	// 패킷 하나를 받았을 때 호출된다. reader는 본문을 가리키며 콜백 안에서만 유효하다.
	using PacketCallback = std::function<void(const std::shared_ptr<Session>& session, PacketType type, PacketReader& reader)>;

	// 연결이 닫혔을 때 호출된다.
	using CloseCallback = std::function<void(SessionId id)>;

public:
	Session(boost::asio::ip::tcp::socket socket, SessionId id, PacketCallback onPacket, CloseCallback onClosed);
	~Session();

	// 읽기를 시작한다. 바로 반환하고, 이후는 콜백으로 진행된다.
	void start();

	// 완성된 패킷(헤더 포함)을 보낸다. 바로 반환하고, 실제 전송은 쓰기 큐를 통해 차례로 진행된다.
	// 여러 세션이 같은 패킷을 공유할 수 있도록 shared_ptr로 받는다 (보내는 동안 살아 있어야 한다).
	void send(std::shared_ptr<const PacketBuffer> packet);

	// 연결을 닫고 onClosed 콜백으로 알린다. 여러 번 호출해도 된다.
	void close();

	SessionId getId() const { return m_id; }
	const std::string& getName() const { return m_name; }

private:
	// 헤더(4바이트)가 오기를 비동기로 기다린다.
	void startReadHeader();

	// 헤더를 받으면 size를 검사하고 본문을 기다린다.
	void onReadHeader(const boost::system::error_code& ec);

	// 본문을 받으면 콜백으로 넘기고 다음 헤더를 기다린다.
	void onReadBody(const boost::system::error_code& ec);

	// 읽기 오류(연결 끊김 포함)를 기록하고 닫는다.
	void onReadError(const boost::system::error_code& ec);

	// 쓰기 큐의 맨 앞 패킷을 비동기로 보낸다.
	void startWrite();

	// 다 보내면(또는 실패하면) 호출된다. 큐에 남은 게 있으면 이어서 보낸다.
	void onWrite(const boost::system::error_code& ec, std::size_t length);

private:
	boost::asio::ip::tcp::socket m_socket; // 이 클라이언트 전용 소켓
	const SessionId m_id; // 세션 ID
	std::string m_name; // "IP:포트" (로그, 보낸 사람 표시용)
	bool m_closed = false; // close()가 이미 호출됐는지

	PacketCallback m_onPacket; // 패킷 하나를 받았을 때
	CloseCallback m_onClosed; // 연결이 닫혔을 때

	std::array<std::uint8_t, packetHeaderSize> m_header{}; // 받는 중인 헤더
	PacketType m_bodyType{}; // 받는 중인 본문의 패킷 종류
	PacketBuffer m_body; // 받는 중인 본문
	std::deque<std::shared_ptr<const PacketBuffer>> m_writeQueue; // 보낼 패킷. 맨 앞이 지금 보내는 중인 것.
};
