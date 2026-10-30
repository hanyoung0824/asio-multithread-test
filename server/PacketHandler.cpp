#include "PacketHandler.hpp"

#include "Session.hpp"
#include "SessionManager.hpp"

#include <iostream>
#include <thread>

PacketHandler& PacketHandler::instance()
{
	static PacketHandler instance;
	return instance;
}

void PacketHandler::handle(const std::shared_ptr<Session>& session, std::string line)
{
	std::cout << "[thread " << std::this_thread::get_id() << "] " << session->getName() << ": " << line << '\n';

	// 모든 클라이언트에게 보낸다. 메시지는 한 번만 만들고 모든 세션이 공유한다.
	SessionManager::instance().broadcast(
		std::make_shared<const std::string>("[" + session->getName() + "] " + line + "\n"));
}
