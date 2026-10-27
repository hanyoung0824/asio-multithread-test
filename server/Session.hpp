#pragma once

#include "SessionManager.hpp"

#include <boost/asio.hpp>

#include <deque>
#include <memory>
#include <string>

// 접속한 클라이언트 하나와 데이터를 주고받는 역할.
// 받은 줄('\n'로 끝나는 메시지)을 SessionManager를 통해 모든 클라이언트에게 보낸다(broadcast).
//
// 읽기와 쓰기는 서로 독립적으로 진행된다.
// - 읽기: 항상 하나가 걸려 있다. 한 줄을 받으면 처리하고 바로 다음 읽기를 건다.
// - 쓰기: send()로 들어온 데이터를 큐에 쌓고, async_write는 한 번에 하나씩만 진행한다.
//
// 비동기 작업이 끝나기 전에 Session이 사라지면 안 되므로 반드시 std::make_shared로 만든다.
// SessionManager 목록과, 진행 중인 비동기 작업의 콜백이 shared_ptr로 Session을 붙잡고 있다.
class Session : public std::enable_shared_from_this<Session>
{
public:
	Session(boost::asio::ip::tcp::socket socket, SessionManager& manager);
	~Session();

	// 목록에 등록하고 읽기를 시작한다. 바로 반환하고, 이후는 콜백으로 진행된다.
	void start();

	// data를 보낸다. 바로 반환하고, 실제 전송은 쓰기 큐를 통해 차례로 진행된다.
	// 여러 세션이 같은 data를 공유할 수 있도록 shared_ptr로 받는다 (보내는 동안 살아 있어야 한다).
	void send(std::shared_ptr<const std::string> data);

	// 연결을 닫고 목록에서 뺀다. 여러 번 호출해도 된다.
	void close();

private:
	// 한 줄('\n'까지)이 오기를 비동기로 기다린다.
	void startRead();

	// 한 줄을 받으면(또는 연결이 끊기면) 호출된다. length는 '\n'까지 포함한 길이.
	void onRead(const boost::system::error_code& ec, std::size_t length);

	// 쓰기 큐의 맨 앞 데이터를 비동기로 보낸다.
	void startWrite();

	// 다 보내면(또는 실패하면) 호출된다. 큐에 남은 게 있으면 이어서 보낸다.
	void onWrite(const boost::system::error_code& ec, std::size_t length);

private:
	boost::asio::ip::tcp::socket m_socket; // 이 클라이언트 전용 소켓
	SessionManager& m_manager; // 세션 목록 (broadcast, 목록에서 빼기)
	SessionManager::SessionId m_id = 0; // 목록에서 받은 ID
	std::string m_name; // "IP:포트" (로그, 메시지 앞머리용)
	bool m_closed = false; // close()가 이미 호출됐는지

	std::string m_readBuffer; // 받은 데이터. 한 줄을 꺼내고 남은 것은 다음 읽기에 이어서 쓴다.
	std::deque<std::shared_ptr<const std::string>> m_writeQueue; // 보낼 데이터. 맨 앞이 지금 보내는 중인 것.
};
