#include "Session.hpp"

#include <iostream>

namespace asio = boost::asio;
using asio::ip::tcp;

namespace
{
	// 한 줄의 최대 길이. 서버가 이보다 길게 '\n' 없이 보내면 연결을 끊는다.
	constexpr std::size_t maxLineLength = 4096;
}

Session::Session(tcp::socket socket)
	: m_socket(std::move(socket))
{
}

void Session::start()
{
	startRead();
}

void Session::send(std::string data)
{
	if (m_closed) {
		return;
	}

	// 이미 보내는 중이면 큐에만 쌓고, onWrite가 이어서 보낸다.
	const bool writing = !m_writeQueue.empty();
	m_writeQueue.push_back(std::move(data));
	if (!writing) {
		startWrite();
	}
}

void Session::close()
{
	if (m_closed.exchange(true)) {
		return;
	}

	// 서버는 read에서 eof를 받는다. 걸려 있던 작업은 operation_aborted로 끝난다.
	boost::system::error_code ec;
	m_socket.shutdown(tcp::socket::shutdown_both, ec);
	m_socket.close(ec);
}

void Session::startRead()
{
	asio::async_read_until(m_socket, asio::dynamic_buffer(m_readBuffer, maxLineLength), '\n',
		[self = shared_from_this()](const boost::system::error_code& ec, std::size_t length) {
			self->onRead(ec, length);
		});
}

void Session::onRead(const boost::system::error_code& ec, std::size_t length)
{
	if (ec) {
		if (ec == asio::error::eof) {
			std::cout << "Server closed the connection (press Enter to exit)\n";
		}
		else if (ec != asio::error::operation_aborted) {
			std::cerr << "Read error: " << ec.message() << '\n';
		}
		close();
		return;
	}

	// 한 줄을 꺼내 출력하고('\n' 포함), 바로 다음 줄을 기다린다.
	std::cout << m_readBuffer.substr(0, length) << std::flush;
	m_readBuffer.erase(0, length);

	startRead();
}

void Session::startWrite()
{
	asio::async_write(m_socket, asio::buffer(m_writeQueue.front()),
		[self = shared_from_this()](const boost::system::error_code& ec, std::size_t length) {
			self->onWrite(ec, length);
		});
}

void Session::onWrite(const boost::system::error_code& ec, std::size_t)
{
	if (ec) {
		if (ec != asio::error::operation_aborted) {
			std::cerr << "Write error: " << ec.message() << '\n';
		}
		close();
		return;
	}

	m_writeQueue.pop_front();
	if (!m_writeQueue.empty()) {
		startWrite();
	}
}
