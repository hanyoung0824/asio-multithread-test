// 1단계: 동기(blocking) TCP echo 클라이언트
// 사용법: step01_client [host] [port]   (기본값 127.0.0.1 12345)
// 한 줄 입력 -> 서버로 전송 -> 돌아온 응답 출력. 빈 줄을 입력하면 종료.

#include <boost/asio.hpp>

#include <iostream>
#include <string>

namespace asio = boost::asio;
using asio::ip::tcp;

int main(int argc, char* argv[])
{
	const std::string host = argc > 1 ? argv[1] : "127.0.0.1";
	const unsigned short port = argc > 2 ? static_cast<unsigned short>(std::stoi(argv[2])) : 12345;

	asio::io_context ioContext;
	boost::system::error_code ec;

	const asio::ip::address address = asio::ip::make_address(host, ec);
	if (ec) {
		std::cerr << "Invalid address " << host << ": " << ec.message() << '\n';
		return 1;
	}
	const tcp::endpoint endPoint(address, port);

	// 1. connect: 서버에 접속할 때까지 blocking. 서버가 없으면 "connection refused".
	tcp::socket socket(ioContext);
	socket.connect(endPoint, ec);
	if (ec) {
		std::cerr << "Failed to connect to " << endPoint << ": " << ec.message() << '\n';
		return 1;
	}

	std::cout << "Connected to " << endPoint << " (empty line to quit)\n";

	std::string line;
	while (std::cout << "> " && std::getline(std::cin, line) && !line.empty()) {
		// 2. write: 입력한 줄을 전부 보낸다.
		asio::write(socket, asio::buffer(line), ec);
		if (ec) {
			std::cerr << "Write error: " << ec.message() << '\n';
			break;
		}

		// 3. read: echo 서버라서 보낸 길이만큼 돌아온다는 걸 알고 있다.
		// asio::read는 버퍼가 가득 찰 때까지 여러 번 나눠 받아서라도 기다린다.
		std::string reply(line.size(), '\0');
		asio::read(socket, asio::buffer(reply), ec);
		if (ec) {
			std::cerr << "Read error: " << ec.message() << '\n';
			break;
		}

		std::cout << "Echo: " << reply << '\n';
	}

	// 4. 종료: 더 보낼 게 없다고 알리고(shutdown) 닫는다. 서버는 read에서 eof를 받는다.
	socket.shutdown(tcp::socket::shutdown_both, ec);
	socket.close(ec);
	std::cout << "Disconnected\n";
}
