#include "packets/ChatPacket.hpp"

#include "PacketSession.hpp"
#include "PacketWriter.hpp"
#include "Protocol.hpp"
#include "SessionManager.hpp"

#include <iostream>
#include <optional>
#include <string>
#include <thread>

bool ChatPacket::handle(const std::shared_ptr<PacketSession>& session, PacketReader& reader)
{
	// 1. 본문 읽기: message(string). 받은 데이터는 믿을 수 없으므로 형식과 길이를 모두 검사한다.
	std::string message;
	if (!reader.readString(message) || !reader.isEnd() || message.size() > maxChatMessageLength) {
		return false;
	}

	std::cout << "[thread " << std::this_thread::get_id() << "] " << session->getName() << ": " << message << '\n';

	// 2. ChatNotify(보낸 사람, 메시지)를 만든다.
	PacketWriter writer(PacketId::ChatNotify);
	writer.writeString(session->getName());
	writer.writeString(message);
	std::optional<PacketBuffer> packet = writer.finish();
	if (!packet) {
		std::cerr << "ChatNotify packet too large, dropped\n";
		return true;
	}

	// 3. 모든 클라이언트에게 보낸다. 패킷은 한 번만 만들고 모든 세션이 공유한다.
	SessionManager::instance().broadcast(std::make_shared<const PacketBuffer>(std::move(*packet)));
	return true;
}
