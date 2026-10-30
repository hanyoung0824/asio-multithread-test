#pragma once

#include <boost/asio.hpp>

#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <vector>

// 접속한 클라이언트 하나와 바이트를 주고받는 역할 (네트워크 계층, 소켓 I/O만 담당하는 추상 클래스).
// 패킷의 형식(헤더, 패킷 경계)은 모른다. 받은 바이트를 수신 버퍼에 쌓아 onReceive()로 넘기고,
// onReceive()가 "앞에서부터 몇 바이트를 처리했는지" 돌려주면 그만큼만 버퍼에서 지운다.
// 덜 온 패킷의 나머지 바이트는 버퍼에 남아 다음에 받은 바이트와 이어진다.
// 패킷 형식은 프로젝트마다 다르므로, 이 클래스를 상속해 onReceive()를 구현한다 (예: PacketSession).
//
// 읽기와 쓰기는 서로 독립적으로 진행된다.
// - 읽기: 항상 하나가 걸려 있다. 받으면 콜백으로 넘기고 바로 다음 읽기를 건다.
// - 쓰기: send()로 들어온 데이터를 큐에 쌓고, async_write는 한 번에 하나씩만 진행한다.
//
// 비동기 작업이 끝나기 전에 Session이 사라지면 안 되므로 반드시 std::make_shared로 만든다.
// 세션 목록과, 진행 중인 비동기 작업의 콜백이 shared_ptr로 Session을 붙잡고 있다.
class Session : public std::enable_shared_from_this<Session>
{
public:
	using SessionId = std::uint64_t;

	// 주고받는 바이트 배열
	using Buffer = std::vector<std::uint8_t>;

	// 연결이 닫혔을 때 호출된다.
	using CloseCallback = std::function<void(SessionId id)>;

public:
	Session(boost::asio::ip::tcp::socket socket, SessionId id, CloseCallback onClosed);
	virtual ~Session();

	// 읽기를 시작한다. 바로 반환하고, 이후는 콜백으로 진행된다.
	void start();

	// data를 보낸다. 바로 반환하고, 실제 전송은 쓰기 큐를 통해 차례로 진행된다.
	// 여러 세션이 같은 데이터를 공유할 수 있도록 shared_ptr로 받는다 (보내는 동안 살아 있어야 한다).
	void send(std::shared_ptr<const Buffer> data);

	// 연결을 닫고 onClosed 콜백으로 알린다. 여러 번 호출해도 된다.
	void close();

	bool isClosed() const { return m_closed; }
	SessionId getId() const { return m_id; }
	const std::string& getName() const { return m_name; }

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
	boost::asio::ip::tcp::socket m_socket; // 이 클라이언트 전용 소켓
	const SessionId m_id; // 세션 ID
	std::string m_name; // "IP:포트" (로그, 보낸 사람 표시용)
	bool m_closed = false; // close()가 이미 호출됐는지

	CloseCallback m_onClosed; // 연결이 닫혔을 때

	Buffer m_receiveBuffer; // 수신 버퍼 (크기 고정). 앞쪽 m_receivedLength 바이트가 아직 처리되지 않은 데이터.
	std::size_t m_receivedLength = 0; // 수신 버퍼에 쌓인 바이트 수
	std::deque<std::shared_ptr<const Buffer>> m_writeQueue; // 보낼 데이터. 맨 앞이 지금 보내는 중인 것.
};
