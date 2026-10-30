#include "PacketHandler.hpp"

#include "Session.hpp"

#include <iostream>
#include <string>

PacketHandler& PacketHandler::instance()
{
	static PacketHandler instance;
	return instance;
}

void PacketHandler::handle(const std::shared_ptr<Session>& session, PacketType type, PacketReader& reader)
{
	switch (type) {
	case PacketType::ChatNotify:
		handleChatNotify(session, reader);
		break;

	default:
		std::cerr << "Unknown packet type " << static_cast<int>(type) << " from server, closing\n";
		session->close();
		break;
	}
}

void PacketHandler::handleChatNotify(const std::shared_ptr<Session>& session, PacketReader& reader)
{
	std::string sender;
	std::string message;
	if (!reader.readString(sender) || !reader.readString(message) || !reader.isEnd()) {
		std::cerr << "Invalid ChatNotify packet from server, closing\n";
		session->close();
		return;
	}

	std::cout << '[' << sender << "] " << message << '\n' << std::flush;
}
