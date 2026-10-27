#include "Session.hpp"

#include <iostream>

namespace asio = boost::asio;
using asio::ip::tcp;

Session::Session(tcp::socket socket)
	: m_socket(std::move(socket))
{
}

void Session::run()
{
	std::string line;
	while (std::cout << "> " && std::getline(std::cin, line) && !line.empty()) {
		// 1. 입력한 줄을 서버로 보낸다.
		if (!send(line)) {
			break;
		}

		// 2. echo 서버라서 보낸 길이만큼 돌아온다는 걸 알고 있다.
		std::string reply;
		if (!receive(reply, line.size())) {
			break;
		}

		std::cout << "Echo: " << reply << '\n';
	}

	// 3. 종료: 서버는 read에서 eof를 받는다.
	close();
}

bool Session::send(const std::string& data)
{
	// asio::write: 지정한 바이트를 전부 보낼 때까지 blocking.
	boost::system::error_code ec;
	asio::write(m_socket, asio::buffer(data), ec);
	if (ec) {
		std::cerr << "Write error: " << ec.message() << '\n';
		return false;
	}
	return true;
}

bool Session::receive(std::string& data, std::size_t length)
{
	// asio::read: 버퍼가 가득 찰 때까지 여러 번 나눠 받아서라도 기다린다.
	// (read_some은 조금이라도 오면 바로 돌아오므로, 길이를 알 때는 read를 쓴다.)
	boost::system::error_code ec;
	data.resize(length);
	asio::read(m_socket, asio::buffer(data), ec);
	if (ec) {
		std::cerr << "Read error: " << ec.message() << '\n';
		return false;
	}
	return true;
}

void Session::close()
{
	boost::system::error_code ec;
	m_socket.shutdown(tcp::socket::shutdown_both, ec);
	m_socket.close(ec);
	std::cout << "Disconnected\n";
}
