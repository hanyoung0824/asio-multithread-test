#include "SessionManager.hpp"

#include "Session.hpp"

#include <iostream>

SessionManager::SessionId SessionManager::add(std::shared_ptr<Session> session)
{
	const SessionId id = m_nextId++;
	m_sessions.emplace(id, std::move(session));
	std::cout << "Session " << id << " added (" << m_sessions.size() << " connected)\n";
	return id;
}

void SessionManager::remove(SessionId id)
{
	if (m_sessions.erase(id) > 0) {
		std::cout << "Session " << id << " removed (" << m_sessions.size() << " connected)\n";
	}
}

void SessionManager::broadcast(const std::shared_ptr<const std::string>& data)
{
	for (const auto& [id, session] : m_sessions) {
		session->send(data);
	}
}

void SessionManager::closeAll()
{
	// Session::close()가 remove()를 호출해 목록을 바꾸므로,
	// 순회하는 동안 목록이 바뀌지 않도록 먼저 꺼내 놓고 닫는다.
	auto sessions = std::move(m_sessions);
	m_sessions.clear();

	std::cout << "Closing " << sessions.size() << " sessions\n";
	for (const auto& [id, session] : sessions) {
		session->close();
	}
}
