#pragma once

#include <boost/asio.hpp>

// 접속한 클라이언트 하나와 데이터를 주고받는 역할.
// 받은 데이터를 그대로 돌려준다(echo).
class Session
{
public:
	explicit Session(boost::asio::ip::tcp::socket socket);

	// 클라이언트가 연결을 끊을 때까지 받기 -> 돌려주기를 반복한다.
	// 그동안 이 스레드는 멈춰(blocking) 있으므로 다른 클라이언트는 처리하지 못한다.
	void run();

private:
	boost::asio::ip::tcp::socket m_socket; // 이 클라이언트 전용 소켓
	boost::asio::ip::tcp::endpoint m_remote; // 클라이언트 주소:포트 (로그용)
};
