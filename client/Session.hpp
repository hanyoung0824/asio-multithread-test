#pragma once

#include "Packet.hpp"
#include "PacketReader.hpp"

#include <boost/asio.hpp>

#include <array>
#include <atomic>
#include <deque>
#include <functional>
#include <memory>

// 접속한 서버와 패킷을 주고받는 역할 (소켓 I/O만 담당).
// 받은 패킷의 처리는 생성할 때 받은 콜백에 맡긴다.
//
// 소켓은 여러 스레드가 동시에 건드리면 안전하지 않다. 그래서 이 클래스의 함수는
// io_context.run()을 실행하는 네트워크 스레드에서만 호출한다 (다른 스레드에서는 asio::post로 넘긴다).
// 예외로 isClosed()는 atomic 변수만 읽으므로 어느 스레드에서 불러도 된다.
class Session : public std::enable_shared_from_this<Session>
{
public:
	// 패킷 하나를 받았을 때 호출된다. reader는 본문을 가리키며 콜백 안에서만 유효하다.
	using PacketCallback = std::function<void(const std::shared_ptr<Session>& session, PacketType type, PacketReader& reader)>;

public:
	Session(boost::asio::ip::tcp::socket socket, PacketCallback onPacket);

	// 받기를 시작한다. 바로 반환하고, 이후는 콜백으로 진행된다.
	void start();

	// 완성된 패킷(헤더 포함)을 보낸다. 쓰기 큐에 쌓고, async_write는 한 번에 하나씩만 진행한다.
	void send(PacketBuffer packet);

	// 연결을 닫는다. 여러 번 호출해도 된다.
	void close();

	// 연결이 닫혔는지 (서버가 끊었거나 close()를 호출했거나). 어느 스레드에서 불러도 된다.
	bool isClosed() const { return m_closed.load(); }

private:
	// 헤더(4바이트)가 오기를 비동기로 기다린다.
	void startReadHeader();

	// 헤더를 받으면 size를 검사하고 본문을 기다린다.
	void onReadHeader(const boost::system::error_code& ec);

	// 본문을 받으면 콜백으로 넘기고 다음 헤더를 기다린다.
	void onReadBody(const boost::system::error_code& ec);

	// 읽기 오류(연결 끊김 포함)를 출력하고 닫는다.
	void onReadError(const boost::system::error_code& ec);

	// 쓰기 큐의 맨 앞 패킷을 비동기로 보낸다.
	void startWrite();

	// 다 보내면(또는 실패하면) 호출된다. 큐에 남은 게 있으면 이어서 보낸다.
	void onWrite(const boost::system::error_code& ec, std::size_t length);

private:
	boost::asio::ip::tcp::socket m_socket; // 서버와 연결된 소켓
	std::atomic<bool> m_closed{ false }; // 연결이 닫혔는지 (메인 스레드도 읽는다)
	PacketCallback m_onPacket; // 패킷 하나를 받았을 때

	std::array<std::uint8_t, packetHeaderSize> m_header{}; // 받는 중인 헤더
	PacketType m_bodyType{}; // 받는 중인 본문의 패킷 종류
	PacketBuffer m_body; // 받는 중인 본문
	std::deque<PacketBuffer> m_writeQueue; // 보낼 패킷. 맨 앞이 지금 보내는 중인 것.
};
