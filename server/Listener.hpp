#pragma once

#include <boost/asio.hpp>

// 서버 소켓(acceptor)을 열고 클라이언트 접속을 기다리는 역할.
// 접속을 받으면 그 클라이언트 전용 socket을 넘겨줄 뿐, 데이터는 주고받지 않는다.
class Listener
{
public:
	Listener(boost::asio::io_context& ioContext, const boost::asio::ip::tcp::endpoint& endPoint);

	// acceptor를 열고 접속을 받을 수 있는 상태(listen)로 만든다.
	// 실패하면 이유를 출력한 뒤 false를 반환한다.
	bool open();

	// 클라이언트가 접속할 때까지 멈춰(blocking) 기다린다.
	// 접속하면 그 클라이언트 전용 socket을 socket에 채우고 true를 반환한다.
	bool accept(boost::asio::ip::tcp::socket& socket);

	const boost::asio::ip::tcp::endpoint& getEndPoint() const { return m_endPoint; }

private:
	boost::asio::ip::tcp::acceptor m_acceptor; // 접속을 받는 전용 소켓
	const boost::asio::ip::tcp::endpoint m_endPoint; // 대기할 주소:포트
};
