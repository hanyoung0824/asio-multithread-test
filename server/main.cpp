// TCP echo 서버 (동기, 싱글 스레드)
// 한 번에 한 클라이언트만 처리한다. 받은 데이터를 그대로 돌려준다.

#include "Listener.hpp"
#include "Session.hpp"

#include <boost/asio.hpp>

#include <iostream>

namespace asio = boost::asio;
using asio::ip::tcp;

int main()
{
	constexpr unsigned short port = 12345;

	// 1. io_context: Asio의 모든 I/O 객체(소켓, acceptor 등)는 io_context 하나에 속한다.
	asio::io_context ioContext;

	// 2. 서버 소켓 열기: open -> 옵션 -> bind -> listen
	Listener listener(ioContext, tcp::endpoint(tcp::v4(), port));
	if (!listener.open()) {
		return 1;
	}

	std::cout << "Echo server listening on " << listener.getEndPoint() << '\n';

	for (;;) {
		// 3. 접속 대기: 클라이언트가 접속하면 그 클라이언트 전용 socket을 받는다.
		tcp::socket socket(ioContext);
		if (!listener.accept(socket)) {
			continue;
		}

		// 4. 데이터 처리: 클라이언트가 끊을 때까지 echo.
		// 이 클라이언트를 처리하는 동안 다른 클라이언트는 accept되지 못하고 기다린다.
		Session session(std::move(socket));
		session.run();
	}
}
