// TCP 채팅 클라이언트
// 사용법: client [host] [port]   (기본값 127.0.0.1 12345)
// 입력한 줄을 Chat 패킷으로 서버에 보내고, 서버가 보내는 ChatNotify(다른 클라이언트의 메시지 포함)를 받는 대로 출력한다.
// 빈 줄을 입력하면 종료.
//
// 스레드 구성:
// - 네트워크 스레드: io_context.run()으로 받기/보내기를 비동기로 처리한다. 소켓은 이 스레드만 만진다.
// - 메인 스레드: 키보드 입력을 기다리다가, 보낼 줄을 asio::post로 네트워크 스레드에 넘긴다.

#include "Connector.hpp"
#include "Packet.hpp"
#include "PacketHandler.hpp"
#include "PacketWriter.hpp"
#include "Session.hpp"

#include <boost/asio.hpp>

#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <thread>

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

	// 3. 서버에 접속 (아직 네트워크 스레드가 없으므로 동기로 접속한다)
	Connector connector(tcp::endpoint(address, port));
	tcp::socket socket(ioContext);
	if (!connector.connect(socket)) {
		return 1;
	}

	std::cout << "Connected to " << connector.getEndPoint() << " (empty line to quit)\n";

	// 4. 받기 시작 후 네트워크 스레드에서 이벤트 루프 실행. 받은 패킷은 PacketHandler가 처리한다.
	auto session = std::make_shared<Session>(std::move(socket),
		[](const std::shared_ptr<Session>& session, PacketType type, PacketReader& reader) {
			PacketHandler::instance().handle(session, type, reader);
		});
	session->start();
	std::thread networkThread([&ioContext] { ioContext.run(); });

	// 5. 메인 스레드: 입력한 줄을 Chat 패킷으로 만들어 네트워크 스레드로 넘긴다.
	// asio::post: 함수를 io_context의 할 일 목록에 넣는다. 실제 실행은 run()을 돌리는 네트워크 스레드에서 한다.
	std::string line;
	while (std::getline(std::cin, line) && !line.empty() && !session->isClosed()) {
		// 서버도 길이를 검사해 넘으면 끊으므로, 보내기 전에 먼저 거른다.
		if (line.size() > maxChatMessageLength) {
			std::cerr << "Message too long (max " << maxChatMessageLength << " bytes)\n";
			continue;
		}

		PacketWriter writer(PacketType::Chat);
		writer.writeString(line);
		std::optional<PacketBuffer> packet = writer.finish();
		if (!packet) {
			continue;
		}

		asio::post(ioContext, [session, packet = std::move(*packet)]() mutable {
			session->send(std::move(packet));
		});
	}

	// 6. 종료: 닫기도 네트워크 스레드에서 하도록 넘긴다.
	// 소켓이 닫히면 걸려 있던 작업이 끝나고, 할 일이 없어진 run()이 반환해 네트워크 스레드가 끝난다.
	asio::post(ioContext, [session] { session->close(); });
	networkThread.join();

	std::cout << "Disconnected\n";
}
