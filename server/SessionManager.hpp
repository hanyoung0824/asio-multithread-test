#pragma once

#include "Session.hpp"

#include <boost/asio.hpp>

#include <memory>
#include <string>
#include <unordered_map>

// 세션을 만들고, 접속 중인 세션 목록을 관리하는 역할 (싱글톤).
// 목록이 shared_ptr를 들고 있으므로, 세션은 목록에서 빠지고 진행 중인 비동기 작업도 끝나야 소멸한다.
// 세션이 받은 패킷의 처리는 setPacketHandler로 등록한 함수에 맡긴다.
//
// 지금은 io_context.run()을 실행하는 스레드 하나에서만 접근하므로 잠금(mutex)이 필요 없다.
class SessionManager
{
public:
	static SessionManager& instance();

	SessionManager(const SessionManager&) = delete;
	SessionManager& operator=(const SessionManager&) = delete;

	// 세션이 패킷을 받았을 때 호출할 함수를 등록한다. create()보다 먼저 호출한다.
	void setPacketHandler(Session::PacketCallback handler);

	// 연결된 소켓으로 세션을 만들어 목록에 넣고 시작시킨다.
	void create(boost::asio::ip::tcp::socket socket);

	// 접속 중인 모든 세션에게 packet을 보낸다.
	// packet은 모든 세션이 같은 메모리를 공유하므로, 세션 수만큼 복사하지 않는다.
	void broadcast(const std::shared_ptr<const PacketBuffer>& packet);

	// 모든 세션을 닫는다 (서버 종료용).
	void closeAll();

	std::size_t size() const { return m_sessions.size(); }

private:
	SessionManager() = default;

	// 세션이 닫혔을 때 목록에서 뺀다. 이미 없으면 아무 일도 하지 않는다.
	void remove(Session::SessionId id);

private:
	std::unordered_map<Session::SessionId, std::shared_ptr<Session>> m_sessions; // 접속 중인 세션
	Session::SessionId m_nextId = 1; // 다음에 줄 세션 ID
	Session::PacketCallback m_packetHandler; // 세션이 받은 패킷을 처리할 함수
};
