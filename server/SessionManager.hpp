#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

class Session;

// 접속 중인 세션 목록을 관리하는 역할.
// 목록이 shared_ptr를 들고 있으므로, 세션은 목록에서 빠지고 진행 중인 비동기 작업도 끝나야 소멸한다.
//
// 지금은 io_context.run()을 실행하는 스레드 하나에서만 접근하므로 잠금(mutex)이 필요 없다.
class SessionManager
{
public:
	using SessionId = std::uint64_t;

public:
	// 세션을 목록에 넣고 새 ID를 돌려준다.
	SessionId add(std::shared_ptr<Session> session);

	// 세션을 목록에서 뺀다. 이미 없으면 아무 일도 하지 않는다.
	void remove(SessionId id);

	// 접속 중인 모든 세션에게 data를 보낸다.
	// data는 모든 세션이 같은 메모리를 공유하므로, 세션 수만큼 복사하지 않는다.
	void broadcast(const std::shared_ptr<const std::string>& data);

	// 모든 세션을 닫는다 (서버 종료용).
	void closeAll();

	std::size_t size() const { return m_sessions.size(); }

private:
	std::unordered_map<SessionId, std::shared_ptr<Session>> m_sessions; // 접속 중인 세션
	SessionId m_nextId = 1; // 다음에 줄 세션 ID
};
