#pragma once

#include <boost/asio.hpp>

#include <string>

// 접속한 서버와 데이터를 주고받는 역할.
// 콘솔에서 한 줄씩 입력받아 서버로 보내고, 돌아온 echo를 출력한다.
class Session
{
public:
	explicit Session(boost::asio::ip::tcp::socket socket);

	// 빈 줄을 입력하거나 오류가 날 때까지 입력 -> 전송 -> 수신 -> 출력을 반복하고, 연결을 닫는다.
	void run();

private:
	// 데이터를 전부 보낸다. 실패하면 이유를 출력한 뒤 false를 반환한다.
	bool send(const std::string& data);

	// 정확히 length 바이트를 받는다. 실패하면 이유를 출력한 뒤 false를 반환한다.
	bool receive(std::string& data, std::size_t length);

	// 더 보낼 게 없다고 알리고(shutdown) 소켓을 닫는다.
	void close();

private:
	boost::asio::ip::tcp::socket m_socket; // 서버와 연결된 소켓
};
