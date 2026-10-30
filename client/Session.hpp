#pragma once

#include <boost/asio.hpp>

#include <atomic>
#include <cstdint>
#include <deque>
#include <memory>
#include <span>
#include <vector>

// 접속한 서버와 바이트를 주고받는 역할 (네트워크 계층, 소켓 I/O만 담당하는 추상 클래스).
// 패킷의 형식(헤더, 패킷 경계)은 모른다. 받은 바이트를 수신 버퍼에 쌓아 onReceive()로 넘기고,
// onReceive()가 "앞에서부터 몇 바이트를 처리했는지" 돌려주면 그만큼만 버퍼에서 지운다.
// 패킷 형식은 프로젝트마다 다르므로, 이 클래스를 상속해 onReceive()를 구현한다 (예: PacketSession).
//
// 소켓은 여러 스레드가 동시에 건드리면 안전하지 않다. 그래서 이 클래스의 함수는
// io_context.run()을 실행하는 네트워크 스레드에서만 호출한다 (다른 스레드에서는 asio::post로 넘긴다).
// 예외로 isClosed()는 atomic 변수만 읽으므로 어느 스레드에서 불러도 된다.
class Session : public std::enable_shared_from_this<Session>
{
public:
	// 주고받는 바이트 배열
	using Buffer = std::vector<std::uint8_t>;

public:
	explicit Session(boost::asio::ip::tcp::socket socket);
	virtual ~Session() = default;

	// 받기를 시작한다. 바로 반환하고, 이후는 콜백으로 진행된다.
	void start();

	// data를 보낸다. 쓰기 큐에 쌓고, async_write는 한 번에 하나씩만 진행한다.
	void send(Buffer data);

	// 연결을 닫는다. 여러 번 호출해도 된다.
	void close();

	// 연결이 닫혔는지 (서버가 끊었거나 close()를 호출했거나). 어느 스레드에서 불러도 된다.
	bool isClosed() const { return m_closed.load(); }

protected:
	// 바이트를 받았을 때 호출된다. data는 아직 처리되지 않은 바이트 전체이며 이 함수 안에서만 유효하다.
	// 앞에서부터 처리한 바이트 수를 반환한다 (0이면 아직 패킷 하나가 다 오지 않은 것).
	// 잘못된 데이터면 close()를 호출하고 반환한다.
	virtual std::size_t onReceive(std::span<const std::uint8_t> data) = 0;

private:
	// 수신 버퍼의 빈 곳으로 바이트가 오기를 비동기로 기다린다.
	void startRead();

	// 받으면 onReceive()로 넘기고, 처리한 만큼 버퍼에서 지운 뒤 다음 읽기를 건다.
	void onRead(const boost::system::error_code& ec, std::size_t length);

	// 쓰기 큐의 맨 앞 데이터를 비동기로 보낸다.
	void startWrite();

	// 다 보내면(또는 실패하면) 호출된다. 큐에 남은 게 있으면 이어서 보낸다.
	void onWrite(const boost::system::error_code& ec, std::size_t length);

private:
	boost::asio::ip::tcp::socket m_socket; // 서버와 연결된 소켓
	std::atomic<bool> m_closed{ false }; // 연결이 닫혔는지 (메인 스레드도 읽는다)

	Buffer m_receiveBuffer; // 수신 버퍼 (크기 고정). 앞쪽 m_receivedLength 바이트가 아직 처리되지 않은 데이터.
	std::size_t m_receivedLength = 0; // 수신 버퍼에 쌓인 바이트 수
	std::deque<Buffer> m_writeQueue; // 보낼 데이터. 맨 앞이 지금 보내는 중인 것.
};
