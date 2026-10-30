#include "Session.hpp"

#include <iostream>

namespace asio = boost::asio;
using asio::ip::tcp;

Session::Session(tcp::socket socket, PacketCallback onPacket)
	: m_socket(std::move(socket)),
	m_onPacket(std::move(onPacket))
{
}

void Session::start()
{
	startReadHeader();
}

void Session::send(PacketBuffer packet)
{
	if (m_closed) {
		return;
	}

	// 이미 보내는 중이면 큐에만 쌓고, onWrite가 이어서 보낸다.
	const bool writing = !m_writeQueue.empty();
	m_writeQueue.push_back(std::move(packet));
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

void Session::startReadHeader()
{
	// 헤더 4바이트를 정확히 다 받은 뒤 콜백을 호출한다.
	asio::async_read(m_socket, asio::buffer(m_header),
		[self = shared_from_this()](const boost::system::error_code& ec, std::size_t) {
			self->onReadHeader(ec);
		});
}

void Session::onReadHeader(const boost::system::error_code& ec)
{
	if (ec) {
		onReadError(ec);
		return;
	}

	// 서버가 보낸 데이터라도 검사한다. size가 잘못되면 이후 데이터의 경계를 알 수 없으므로 끊는다.
	const PacketHeader header = parseHeader(m_header);
	if (header.size < packetHeaderSize || header.size > maxPacketSize) {
		std::cerr << "Invalid packet size " << header.size << " from server, closing\n";
		close();
		return;
	}

	m_bodyType = header.type;
	m_body.resize(header.size - packetHeaderSize);
	asio::async_read(m_socket, asio::buffer(m_body),
		[self = shared_from_this()](const boost::system::error_code& ec, std::size_t) {
			self->onReadBody(ec);
		});
}

void Session::onReadBody(const boost::system::error_code& ec)
{
	if (ec) {
		onReadError(ec);
		return;
	}

	if (m_onPacket) {
		PacketReader reader(m_body);
		m_onPacket(shared_from_this(), m_bodyType, reader);
	}

	if (!m_closed) {
		startReadHeader();
	}
}

void Session::onReadError(const boost::system::error_code& ec)
{
	if (ec == asio::error::eof) {
		std::cout << "Server closed the connection (press Enter to exit)\n";
	}
	else if (ec != asio::error::operation_aborted) {
		std::cerr << "Read error: " << ec.message() << '\n';
	}
	close();
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
