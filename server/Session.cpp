#include "Session.hpp"

#include <iostream>
#include <thread>

namespace asio = boost::asio;
using asio::ip::tcp;

Session::Session(tcp::socket socket)
	: m_socket(std::move(socket))
{
	boost::system::error_code ec;
	m_remote = m_socket.remote_endpoint(ec);
}

Session::~Session()
{
	// 마지막 콜백이 끝나 아무도 이 Session을 붙잡지 않을 때 호출된다. m_socket도 여기서 닫힌다.
	std::cout << "Session destroyed: " << m_remote << '\n';
}

void Session::start()
{
	std::cout << "[thread " << std::this_thread::get_id() << "] Client connected: " << m_remote << '\n';
	startRead();
}

void Session::startRead()
{
	// async_read_some: 기다리지 않고 바로 반환한다. 데이터가 조금이라도 오면 콜백이 호출된다.
	// self(shared_ptr)를 람다에 담아 두어, 콜백이 호출될 때까지 Session이 살아 있게 한다.
	m_socket.async_read_some(asio::buffer(m_buffer),
		[self = shared_from_this()](const boost::system::error_code& ec, std::size_t length) {
			self->onRead(ec, length);
		});
}

void Session::onRead(const boost::system::error_code& ec, std::size_t length)
{
	if (ec == asio::error::eof) {
		// 상대가 연결을 정상 종료했다. 다음 작업을 걸지 않으므로 체인이 끊기고 Session이 소멸한다.
		std::cout << "Client disconnected: " << m_remote << '\n';
		return;
	}
	if (ec) {
		if (ec != asio::error::operation_aborted) {
			std::cerr << "Read error from " << m_remote << ": " << ec.message() << '\n';
		}
		return;
	}

	std::cout << "[thread " << std::this_thread::get_id() << "] Received " << length << " bytes from " << m_remote << '\n';

	startWrite(length);
}

void Session::startWrite(std::size_t length)
{
	// asio::async_write: 지정한 바이트를 전부 보내면 콜백이 호출된다 (내부에서 여러 번 나눠 보낼 수 있다).
	// 보내는 동안 m_buffer가 바뀌면 안 된다. 다 보낸 뒤에야 다음 읽기를 시작하므로 안전하다.
	asio::async_write(m_socket, asio::buffer(m_buffer.data(), length),
		[self = shared_from_this()](const boost::system::error_code& ec, std::size_t length) {
			self->onWrite(ec, length);
		});
}

void Session::onWrite(const boost::system::error_code& ec, std::size_t)
{
	if (ec) {
		if (ec != asio::error::operation_aborted) {
			std::cerr << "Write error to " << m_remote << ": " << ec.message() << '\n';
		}
		return;
	}

	// 다 보냈으면 다시 받기를 기다린다. read -> write -> read ... 콜백 체인이 이어진다.
	startRead();
}
