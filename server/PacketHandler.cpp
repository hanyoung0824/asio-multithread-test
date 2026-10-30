#include "PacketHandler.hpp"

#include "PacketWriter.hpp"
#include "Session.hpp"
#include "SessionManager.hpp"

#include <iostream>
#include <string>
#include <thread>

PacketHandler& PacketHandler::instance()
{
	static PacketHandler instance;
	return instance;
}

void PacketHandler::handle(const std::shared_ptr<Session>& session, PacketType type, PacketReader& reader)
{
	switch (type) {
	case PacketType::Chat:
		handleChat(session, reader);
		break;

	default:
		// 클라이언트가 보내면 안 되는 패킷(ChatNotify 등)이나 모르는 종류
		std::cerr << "Unknown packet type " << static_cast<int>(type) << " from " << session->getName() << ", closing\n";
		session->close();
		break;
	}
}

void PacketHandler::handleChat(const std::shared_ptr<Session>& session, PacketReader& reader)
{
	// 1. 본문 읽기: message(string). 받은 데이터는 믿을 수 없으므로 형식과 길이를 모두 검사한다.
	std::string message;
	if (!reader.readString(message) || !reader.isEnd() || message.size() > maxChatMessageLength) {
		std::cerr << "Invalid Chat packet from " << session->getName() << ", closing\n";
		session->close();
		return;
	}

	std::cout << "[thread " << std::this_thread::get_id() << "] " << session->getName() << ": " << message << '\n';

	// 2. ChatNotify(보낸 사람, 메시지)를 만든다.
	PacketWriter writer(PacketType::ChatNotify);
	writer.writeString(session->getName());
	writer.writeString(message);
	std::optional<PacketBuffer> packet = writer.finish();
	if (!packet) {
		std::cerr << "ChatNotify packet too large, dropped\n";
		return;
	}

	// 3. 모든 클라이언트에게 보낸다. 패킷은 한 번만 만들고 모든 세션이 공유한다.
	SessionManager::instance().broadcast(std::make_shared<const PacketBuffer>(std::move(*packet)));
}
