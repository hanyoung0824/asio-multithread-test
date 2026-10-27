#include "Session.hpp"

#include <array>
#include <iostream>

namespace asio = boost::asio;
using asio::ip::tcp;

Session::Session(tcp::socket socket)
	: m_socket(std::move(socket))
{
	boost::system::error_code ec;
	m_remote = m_socket.remote_endpoint(ec);
}

void Session::run()
{
	std::cout << "Client connected: " << m_remote << '\n';

	boost::system::error_code ec;
	std::array<char, 1024> buffer;

	for (;;) {
		// 1. read_some: 데이터가 조금이라도 도착할 때까지 blocking.
		// 보낸 쪽이 한 번에 보낸 크기와 받는 크기가 같다는 보장은 없다 (TCP는 바이트 스트림).
		const std::size_t length = m_socket.read_some(asio::buffer(buffer), ec);
		if (ec == asio::error::eof) {
			// 상대가 연결을 정상 종료했다.
			std::cout << "Client disconnected: " << m_remote << '\n';
			break;
		}
		if (ec) {
			std::cerr << "Read error from " << m_remote << ": " << ec.message() << '\n';
			break;
		}

		std::cout << "Received " << length << " bytes from " << m_remote << '\n';

		// 2. asio::write: 받은 바이트를 전부 보낼 때까지 blocking.
		asio::write(m_socket, asio::buffer(buffer.data(), length), ec);
		if (ec) {
			std::cerr << "Write error to " << m_remote << ": " << ec.message() << '\n';
			break;
		}
	}

	// Session이 소멸하면서 m_socket이 닫힌다.
}
