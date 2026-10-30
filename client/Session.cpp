#include "Session.hpp"

#include <algorithm>
#include <iostream>

namespace asio = boost::asio;
using asio::ip::tcp;

namespace
{
	// 수신 버퍼 크기. 처리되지 않은 바이트가 이만큼 쌓이면(끝나지 않는 데이터) 연결을 끊는다.
	constexpr std::size_t receiveBufferSize = 64 * 1024;
}

Session::Session(tcp::socket socket)
	: m_socket(std::move(socket)),
	m_receiveBuffer(receiveBufferSize)
{
}

void Session::start()
{
	startRead();
}

void Session::send(Buffer data)
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
	// 이미 쌓여 있는 바이트 뒤, 버퍼의 빈 곳에 받는다. 조금이라도 오면 바로 콜백이 호출된다.
	asio::mutable_buffer freeSpace = asio::buffer(m_receiveBuffer) + m_receivedLength;
	m_socket.async_read_some(freeSpace,
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

	m_receivedLength += length;

	// 1. 쌓인 바이트 전체를 넘긴다. 무엇인지, 어디까지가 패킷인지는 파생 클래스가 정한다.
	const std::size_t processed = onReceive(std::span<const std::uint8_t>(m_receiveBuffer.data(), m_receivedLength));

	if (m_closed) {
		return;
	}

	if (processed > m_receivedLength) {
		std::cerr << "onReceive processed more bytes than received, closing\n";
		close();
		return;
	}

	// 2. 처리한 만큼 지우고, 남은 바이트(덜 온 패킷)를 버퍼 앞으로 옮긴다.
	// (처리한 게 없으면 옮길 필요가 없다. std::copy는 같은 자리로 복사하는 것을 허용하지 않는다.)
	if (processed > 0) {
		std::copy(m_receiveBuffer.begin() + processed, m_receiveBuffer.begin() + m_receivedLength, m_receiveBuffer.begin());
		m_receivedLength -= processed;
	}

	// 3. 버퍼가 꽉 찼는데 처리된 게 없으면 더 받을 곳이 없다.
	if (m_receivedLength == m_receiveBuffer.size()) {
		std::cerr << "Receive buffer full, closing\n";
		close();
		return;
	}

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
