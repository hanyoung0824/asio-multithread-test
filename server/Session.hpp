#pragma once

#include <boost/asio.hpp>

#include <array>
#include <memory>

// 접속한 클라이언트 하나와 데이터를 주고받는 역할.
// 받은 데이터를 그대로 돌려준다(echo).
//
// 비동기 작업(async_read_some, async_write)이 끝나기 전에 Session이 사라지면 안 되므로
// 반드시 std::make_shared로 만들고, 콜백마다 shared_from_this()로 자신을 붙잡아 둔다.
// 진행 중인 작업이 하나도 없으면 아무도 붙잡지 않게 되어 자동으로 소멸한다(이때 소켓도 닫힌다).
class Session : public std::enable_shared_from_this<Session>
{
public:
	explicit Session(boost::asio::ip::tcp::socket socket);
	~Session();

	// 데이터 받기를 시작한다. 바로 반환하고, 이후는 콜백으로 진행된다.
	void start();

private:
	// 데이터가 오기를 비동기로 기다린다.
	void startRead();

	// 데이터가 오면(또는 연결이 끊기면) 호출된다.
	void onRead(const boost::system::error_code& ec, std::size_t length);

	// 받은 length 바이트를 비동기로 돌려준다.
	void startWrite(std::size_t length);

	// 다 보내면(또는 실패하면) 호출된다.
	void onWrite(const boost::system::error_code& ec, std::size_t length);

private:
	boost::asio::ip::tcp::socket m_socket; // 이 클라이언트 전용 소켓
	boost::asio::ip::tcp::endpoint m_remote; // 클라이언트 주소:포트 (로그용)
	std::array<char, 1024> m_buffer; // 받은 데이터. 비동기 작업 동안 살아 있어야 해서 멤버로 둔다.
};
