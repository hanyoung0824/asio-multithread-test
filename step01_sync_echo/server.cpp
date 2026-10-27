// 1단계: 동기(blocking) TCP echo 서버
// 한 번에 한 클라이언트만 처리한다. 받은 데이터를 그대로 돌려준다.

#include <boost/asio.hpp>

#include <array>
#include <iostream>

namespace asio = boost::asio;
using asio::ip::tcp;

// acceptor를 열고 접속을 받을 수 있는 상태(listen)로 만든다.
// 생성자 한 줄로도 되지만(open -> reuse_address(true) -> bind -> listen),
// bind 전에 OS별 옵션을 넣어야 해서 단계별로 연다. 단계마다 error_code로 실패를 확인하고,
// 실패하면 이유를 출력한 뒤 false를 반환한다.
bool openAcceptor(tcp::acceptor& acceptor, const tcp::endpoint& endPoint)
{
	boost::system::error_code ec;

	// 1. open: 소켓을 만든다 (IPv4 / IPv6 중 endpoint의 프로토콜로).
	acceptor.open(endPoint.protocol(), ec);
	if (ec) {
		std::cerr << "Failed to open acceptor: " << ec.message() << '\n';
		return false;
	}

	// 2. 옵션 (bind 전에 설정해야 한다)
#ifdef _WIN32
	// Windows: SO_REUSEADDR는 사용 중인 포트에도 bind를 허용하므로 끈다.
	// 소켓 기본값도 false지만 의도를 드러내려고 명시한다.
	acceptor.set_option(tcp::acceptor::reuse_address(false), ec);
#else
	// Linux: SO_REUSEADDR는 TIME_WAIT가 남은 포트의 재사용만 허용한다.
	// 켜 두면 서버를 바로 재시작할 수 있고, listen 중인 포트를 가로채지는 못한다.
	acceptor.set_option(tcp::acceptor::reuse_address(true), ec);
#endif
	if (ec) {
		std::cerr << "Failed to configure acceptor: " << ec.message() << '\n';
		return false;
	}

	// 3. bind: 소켓을 주소:포트에 묶는다. 포트를 이미 누가 쓰고 있으면 여기서 실패한다.
	acceptor.bind(endPoint, ec);
	if (ec) {
		std::cerr << "Failed to bind " << endPoint << ": " << ec.message() << '\n';
		return false;
	}

	// 4. listen: 이때부터 클라이언트가 접속할 수 있다.
	// 운영체제가 accept 전의 연결을 쌓아 두는 대기열(backlog) 크기도 정한다.
	acceptor.listen(asio::socket_base::max_listen_connections, ec);
	if (ec) {
		std::cerr << "Failed to listen: " << ec.message() << '\n';
		return false;
	}

	return true;
}

int main()
{
	constexpr unsigned short port = 12345;

	// 1. io_context: Asio의 모든 I/O 객체(소켓, acceptor 등)는 io_context 하나에 속한다.
	asio::io_context ioContext;

	// 2. acceptor 준비: open -> 옵션 -> bind -> listen (openAcceptor 참고)
	tcp::acceptor acceptor(ioContext);
	const tcp::endpoint endPoint(tcp::v4(), port);
	if (!openAcceptor(acceptor, endPoint)) {
		return 1;
	}

	std::cout << "Echo server listening on " << endPoint << '\n';

	boost::system::error_code ec;

	for (;;) {
		// 3. accept: 클라이언트가 접속할 때까지 여기서 멈춰(blocking) 기다린다.
		// 접속하면 그 클라이언트 전용 socket이 만들어진다. acceptor는 계속 접속만 받는다.
		tcp::socket socket(ioContext);
		acceptor.accept(socket, ec);
		if (ec) {
			std::cerr << "Failed to accept: " << ec.message() << '\n';
			continue;
		}

		const tcp::endpoint remote = socket.remote_endpoint(ec);
		std::cout << "Client connected: " << remote << '\n';

		// 4. read -> write 반복
		std::array<char, 1024> buffer;
		for (;;) {
			// read_some: 데이터가 조금이라도 도착할 때까지 blocking.
			// 보낸 쪽이 한 번에 보낸 크기와 받는 크기가 같다는 보장은 없다 (TCP는 바이트 스트림).
			const std::size_t length = socket.read_some(asio::buffer(buffer), ec);
			if (ec == asio::error::eof) {
				// 상대가 연결을 정상 종료했다.
				std::cout << "Client disconnected: " << remote << '\n';
				break;
			}
			if (ec) {
				std::cerr << "Read error from " << remote << ": " << ec.message() << '\n';
				break;
			}

			std::cout << "Received " << length << " bytes from " << remote << '\n';

			// asio::write: 지정한 바이트를 전부 보낼 때까지 blocking.
			asio::write(socket, asio::buffer(buffer.data(), length), ec);
			if (ec) {
				std::cerr << "Write error to " << remote << ": " << ec.message() << '\n';
				break;
			}
		}

		// 5. socket이 범위를 벗어나며 닫히고, 다음 accept로 돌아간다.
		// 이 클라이언트를 처리하는 동안 다른 클라이언트는 accept되지 못하고 기다린다.
	}
}
