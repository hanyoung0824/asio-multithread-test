#include "Listener.hpp"

#include <iostream>

namespace asio = boost::asio;
using asio::ip::tcp;

Listener::Listener(asio::io_context& ioContext, const tcp::endpoint& endPoint)
	: m_acceptor(ioContext),
	m_endPoint(endPoint)
{
}

bool Listener::open()
{
	// 생성자 한 줄로도 열 수 있지만(open -> reuse_address(true) -> bind -> listen),
	// bind 전에 OS별 옵션을 넣어야 해서 단계별로 연다. 단계마다 error_code로 실패를 확인한다.
	boost::system::error_code ec;

	// 1. open: 소켓을 만든다 (IPv4 / IPv6 중 endpoint의 프로토콜로).
	m_acceptor.open(m_endPoint.protocol(), ec);
	if (ec) {
		std::cerr << "Failed to open acceptor: " << ec.message() << '\n';
		return false;
	}

	// 2. 옵션 (bind 전에 설정해야 한다)
#ifdef _WIN32
	// Windows: SO_REUSEADDR는 사용 중인 포트에도 bind를 허용하므로 끈다.
	// 소켓 기본값도 false지만 의도를 드러내려고 명시한다.
	m_acceptor.set_option(tcp::acceptor::reuse_address(false), ec);
#else
	// Linux: SO_REUSEADDR는 TIME_WAIT가 남은 포트의 재사용만 허용한다.
	// 켜 두면 서버를 바로 재시작할 수 있고, listen 중인 포트를 가로채지는 못한다.
	m_acceptor.set_option(tcp::acceptor::reuse_address(true), ec);
#endif
	if (ec) {
		std::cerr << "Failed to configure acceptor: " << ec.message() << '\n';
		return false;
	}

	// 3. bind: 소켓을 주소:포트에 묶는다. 포트를 이미 누가 쓰고 있으면 여기서 실패한다.
	m_acceptor.bind(m_endPoint, ec);
	if (ec) {
		std::cerr << "Failed to bind " << m_endPoint << ": " << ec.message() << '\n';
		return false;
	}

	// 4. listen: 이때부터 클라이언트가 접속할 수 있다.
	// 운영체제가 accept 전의 연결을 쌓아 두는 대기열(backlog) 크기도 정한다.
	m_acceptor.listen(asio::socket_base::max_listen_connections, ec);
	if (ec) {
		std::cerr << "Failed to listen: " << ec.message() << '\n';
		return false;
	}

	return true;
}

bool Listener::accept(tcp::socket& socket)
{
	boost::system::error_code ec;
	m_acceptor.accept(socket, ec);
	if (ec) {
		std::cerr << "Failed to accept: " << ec.message() << '\n';
		return false;
	}
	return true;
}
