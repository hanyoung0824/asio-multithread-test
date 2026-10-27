// TCP echo 서버 (비동기, 싱글 스레드)
// 스레드 하나가 io_context.run()으로 모든 클라이언트를 동시에 처리한다. 받은 데이터를 그대로 돌려준다.

#include "Listener.hpp"
#include "Session.hpp"

#include <boost/asio.hpp>

#include <csignal>
#include <iostream>
#include <memory>
#include <thread>

namespace asio = boost::asio;
using asio::ip::tcp;

int main()
{
	constexpr unsigned short port = 12345;

	// 1. io_context: Asio의 모든 I/O 객체(소켓, acceptor 등)는 io_context 하나에 속한다.
	// 비동기 작업이 끝나면 그 콜백을 io_context가 실행한다.
	asio::io_context ioContext;

	// 2. 서버 소켓 열기: open -> 옵션 -> bind -> listen
	Listener listener(ioContext, tcp::endpoint(tcp::v4(), port));
	if (!listener.open()) {
		return 1;
	}

	// 3. 접속 받기 시작: 접속이 올 때마다 Session을 만들어 시작시킨다.
	// Session은 shared_ptr로 만들고, 이후의 수명은 Session 자신의 콜백들이 관리한다.
	listener.start([](tcp::socket socket) {
		std::make_shared<Session>(std::move(socket))->start();
	});

	// 4. 종료 처리: Ctrl+C(SIGINT) 또는 SIGTERM을 비동기로 기다린다.
	// 받으면 새 접속을 막고 이벤트 루프를 멈춘다.
	asio::signal_set signals(ioContext, SIGINT, SIGTERM);
	signals.async_wait([&](const boost::system::error_code& ec, int signalNumber) {
		if (ec) {
			return;
		}
		std::cout << "Signal " << signalNumber << " received, shutting down\n";
		listener.close();
		ioContext.stop();
	});

	std::cout << "[thread " << std::this_thread::get_id() << "] Echo server listening on " << listener.getEndPoint() << '\n';

	// 5. 이벤트 루프: 완료된 비동기 작업의 콜백을 이 스레드에서 차례로 실행한다.
	// stop()이 호출되거나 할 일이 하나도 없어질 때까지 반환하지 않는다.
	ioContext.run();

	// 남아 있던 Session들은 io_context가 소멸하면서 함께 정리된다.
	std::cout << "Server stopped\n";
}
