// TCP echo 클라이언트 (동기)
// 사용법: client [host] [port]   (기본값 127.0.0.1 12345)
// 한 줄 입력 -> 서버로 전송 -> 돌아온 응답 출력. 빈 줄을 입력하면 종료.

#include "Connector.hpp"
#include "Session.hpp"

#include <boost/asio.hpp>

#include <iostream>
#include <string>

namespace asio = boost::asio;
using asio::ip::tcp;

int main(int argc, char* argv[])
{
	const std::string host = argc > 1 ? argv[1] : "127.0.0.1";
	const unsigned short port = argc > 2 ? static_cast<unsigned short>(std::stoi(argv[2])) : 12345;

	// 1. 접속할 서버 주소 만들기
	boost::system::error_code ec;
	const asio::ip::address address = asio::ip::make_address(host, ec);
	if (ec) {
		std::cerr << "Invalid address " << host << ": " << ec.message() << '\n';
		return 1;
	}

	// 2. io_context: Asio의 모든 I/O 객체(소켓 등)는 io_context 하나에 속한다.
	asio::io_context ioContext;

	// 3. 서버에 접속
	Connector connector(tcp::endpoint(address, port));
	tcp::socket socket(ioContext);
	if (!connector.connect(socket)) {
		return 1;
	}

	std::cout << "Connected to " << connector.getEndPoint() << " (empty line to quit)\n";

	// 4. 데이터 주고받기: 빈 줄을 입력할 때까지
	Session session(std::move(socket));
	session.run();
}
