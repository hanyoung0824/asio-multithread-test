// TCP 채팅 서버 (비동기, 싱글 스레드)
// 스레드 하나가 io_context.run()으로 모든 클라이언트를 동시에 처리한다.
// 한 클라이언트가 보낸 줄을 접속 중인 모든 클라이언트에게 보낸다(broadcast).

#include "Listener.hpp"
#include "Session.hpp"
#include "SessionManager.hpp"

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

	// 2. 세션 목록. 반드시 io_context보다 뒤에 선언한다.
	// 지역 변수는 선언의 역순으로 소멸하므로, 목록에 남은 세션(과 그 소켓)이 io_context보다 먼저 정리된다.
	SessionManager sessionManager;

	// 3. 서버 소켓 열기: open -> 옵션 -> bind -> listen
	Listener listener(ioContext, tcp::endpoint(tcp::v4(), port));
	if (!listener.open()) {
		return 1;
	}

	// 4. 접속 받기 시작: 접속이 올 때마다 Session을 만들어 시작시킨다 (start()에서 목록에 등록된다).
	listener.start([&sessionManager](tcp::socket socket) {
		std::make_shared<Session>(std::move(socket), sessionManager)->start();
	});

	// 5. 종료 처리: Ctrl+C(SIGINT) 또는 SIGTERM을 비동기로 기다린다.
	// 새 접속을 막고 모든 세션을 닫는다. 그러면 걸려 있던 작업들이 취소되어 끝나고,
	// 할 일이 없어진 io_context.run()이 스스로 반환한다 (stop()으로 강제로 멈추지 않는다).
	asio::signal_set signals(ioContext, SIGINT, SIGTERM);
	signals.async_wait([&](const boost::system::error_code& ec, int signalNumber) {
		if (ec) {
			return;
		}
		std::cout << "Signal " << signalNumber << " received, shutting down\n";
		listener.close();
		sessionManager.closeAll();
	});

	std::cout << "[thread " << std::this_thread::get_id() << "] Chat server listening on " << listener.getEndPoint() << '\n';

	// 6. 이벤트 루프: 완료된 비동기 작업의 콜백을 이 스레드에서 차례로 실행한다.
	ioContext.run();

	std::cout << "Server stopped\n";
}
