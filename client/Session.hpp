#pragma once

#include <boost/asio.hpp>

#include <atomic>
#include <deque>
#include <memory>
#include <string>

// 접속한 서버와 데이터를 주고받는 역할.
// 서버가 보낸 줄을 받는 대로 출력하고, send()로 들어온 데이터를 서버로 보낸다.
//
// 소켓은 여러 스레드가 동시에 건드리면 안전하지 않다. 그래서 이 클래스의 함수는
// io_context.run()을 실행하는 네트워크 스레드에서만 호출한다 (다른 스레드에서는 asio::post로 넘긴다).
// 예외로 isClosed()는 atomic 변수만 읽으므로 어느 스레드에서 불러도 된다.
class Session : public std::enable_shared_from_this<Session>
{
public:
	explicit Session(boost::asio::ip::tcp::socket socket);

	// 받기를 시작한다. 바로 반환하고, 이후는 콜백으로 진행된다.
	void start();

	// data를 보낸다. 쓰기 큐에 쌓고, async_write는 한 번에 하나씩만 진행한다.
	void send(std::string data);

	// 연결을 닫는다. 여러 번 호출해도 된다.
	void close();

	// 연결이 닫혔는지 (서버가 끊었거나 close()를 호출했거나). 어느 스레드에서 불러도 된다.
	bool isClosed() const { return m_closed.load(); }

private:
	// 한 줄('\n'까지)이 오기를 비동기로 기다린다.
	void startRead();

	// 한 줄을 받으면(또는 연결이 끊기면) 호출된다.
	void onRead(const boost::system::error_code& ec, std::size_t length);

	// 쓰기 큐의 맨 앞 데이터를 비동기로 보낸다.
	void startWrite();

	// 다 보내면(또는 실패하면) 호출된다. 큐에 남은 게 있으면 이어서 보낸다.
	void onWrite(const boost::system::error_code& ec, std::size_t length);

private:
	boost::asio::ip::tcp::socket m_socket; // 서버와 연결된 소켓
	std::atomic<bool> m_closed{ false }; // 연결이 닫혔는지 (메인 스레드도 읽는다)
	std::string m_readBuffer; // 받은 데이터. 한 줄을 꺼내고 남은 것은 다음 읽기에 이어서 쓴다.
	std::deque<std::string> m_writeQueue; // 보낼 데이터. 맨 앞이 지금 보내는 중인 것.
};
