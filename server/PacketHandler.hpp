#pragma once

#include <memory>
#include <string>

class Session;

// 세션이 받은 데이터를 처리하는 역할 (싱글톤).
// 지금은 채팅 하나뿐이다: 받은 줄 앞에 보낸 사람을 붙여 모든 클라이언트에게 보낸다.
//
// 지금은 io_context.run()을 실행하는 스레드 하나에서만 호출된다.
class PacketHandler
{
public:
	static PacketHandler& instance();

	PacketHandler(const PacketHandler&) = delete;
	PacketHandler& operator=(const PacketHandler&) = delete;

	// session이 보낸 한 줄을 처리한다 ('\n' 제외).
	void handle(const std::shared_ptr<Session>& session, std::string line);

private:
	PacketHandler() = default;
};
