#include "Session.hpp"

#include <iostream>
#include <sstream>
#include <thread>

namespace asio = boost::asio;
using asio::ip::tcp;

Session::Session(tcp::socket socket, SessionId id, PacketCallback onPacket, CloseCallback onClosed)
	: m_socket(std::move(socket)),
	m_id(id),
	m_onPacket(std::move(onPacket)),
	m_onClosed(std::move(onClosed))
{
	boost::system::error_code ec;
	std::ostringstream name;
	name << m_socket.remote_endpoint(ec);
	m_name = name.str();
}

Session::~Session()
{
	// 목록에서 빠지고 마지막 콜백도 끝나 아무도 이 Session을 붙잡지 않을 때 호출된다.
	std::cout << "Session destroyed: " << m_name << '\n';
}

void Session::start()
{
	std::cout << "[thread " << std::this_thread::get_id() << "] Client connected: " << m_name << '\n';
	startReadHeader();
}

void Session::send(std::shared_ptr<const PacketBuffer> packet)
{
	if (m_closed) {
		return;
	}

	// 1. 큐에 쌓는다.
	const bool writing = !m_writeQueue.empty();
	m_writeQueue.push_back(std::move(packet));

	// 2. 이미 보내는 중이면 onWrite가 이어서 보내므로 여기서는 시작하지 않는다.
	// 같은 소켓에 async_write를 동시에 두 개 걸면 두 패킷이 섞여 전송될 수 있다.
	if (!writing) {
		startWrite();
	}
}

void Session::close()
{
	if (m_closed) {
		return;
	}
	m_closed = true;

	// 소켓을 닫으면 걸려 있던 async_read / async_write는 operation_aborted로 끝난다.
	boost::system::error_code ec;
	m_socket.shutdown(tcp::socket::shutdown_both, ec);
	m_socket.close(ec);

	// 닫혔다고 알린다 (목록에서 빠진다). 남은 콜백들이 끝나면 Session이 소멸한다.
	if (m_onClosed) {
		m_onClosed(m_id);
	}
}

void Session::startReadHeader()
{
	// asio::async_read: 버퍼 크기(4바이트)만큼 정확히 다 받은 뒤 콜백을 호출한다.
	// TCP는 데이터를 나눠서 전달할 수 있지만, async_read가 다 모일 때까지 알아서 이어 받는다.
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

	// 1. size 검사: 헤더보다 작거나 최대 크기보다 크면 잘못된(또는 악의적인) 패킷이다.
	const PacketHeader header = parseHeader(m_header);
	if (header.size < packetHeaderSize || header.size > maxPacketSize) {
		std::cerr << "Invalid packet size " << header.size << " from " << m_name << ", closing\n";
		close();
		return;
	}

	// 2. 본문(size - 4바이트)을 받는다. 본문이 0바이트여도 async_read는 바로 완료된다.
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

	// 1. 처리는 콜백에 맡긴다. 무엇을 할지는 Session이 모른다.
	if (m_onPacket) {
		PacketReader reader(m_body);
		m_onPacket(shared_from_this(), m_bodyType, reader);
	}

	// 2. 처리 중에 닫히지 않았으면 바로 다음 패킷을 받는다.
	if (!m_closed) {
		startReadHeader();
	}
}

void Session::onReadError(const boost::system::error_code& ec)
{
	if (ec == asio::error::eof) {
		std::cout << "Client disconnected: " << m_name << '\n';
	}
	else if (ec != asio::error::operation_aborted) {
		std::cerr << "Read error from " << m_name << ": " << ec.message() << '\n';
	}
	close();
}

void Session::startWrite()
{
	// asio::async_write: 맨 앞 패킷을 전부 보내면 콜백이 호출된다.
	// 패킷은 큐 안의 shared_ptr가 붙잡고 있으므로 보내는 동안 사라지지 않는다.
	asio::async_write(m_socket, asio::buffer(*m_writeQueue.front()),
		[self = shared_from_this()](const boost::system::error_code& ec, std::size_t length) {
			self->onWrite(ec, length);
		});
}

void Session::onWrite(const boost::system::error_code& ec, std::size_t)
{
	if (ec) {
		if (ec != asio::error::operation_aborted) {
			std::cerr << "Write error to " << m_name << ": " << ec.message() << '\n';
		}
		close();
		return;
	}

	// 다 보낸 패킷을 빼고, 남은 게 있으면 이어서 보낸다.
	m_writeQueue.pop_front();
	if (!m_writeQueue.empty()) {
		startWrite();
	}
}
