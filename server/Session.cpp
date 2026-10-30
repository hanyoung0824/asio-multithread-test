#include "Session.hpp"

#include <iostream>
#include <sstream>
#include <thread>

namespace asio = boost::asio;
using asio::ip::tcp;

namespace
{
	// 한 줄의 최대 길이. 이보다 길게 '\n' 없이 보내면 연결을 끊는다.
	// 제한이 없으면 악의적인 클라이언트가 줄바꿈 없이 계속 보내 서버 메모리를 다 쓰게 만들 수 있다.
	constexpr std::size_t maxLineLength = 1024;
}

Session::Session(tcp::socket socket, SessionId id, MessageCallback onMessage, CloseCallback onClosed)
	: m_socket(std::move(socket)),
	m_id(id),
	m_onMessage(std::move(onMessage)),
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
	startRead();
}

void Session::send(std::shared_ptr<const std::string> data)
{
	if (m_closed) {
		return;
	}

	// 1. 큐에 쌓는다.
	const bool writing = !m_writeQueue.empty();
	m_writeQueue.push_back(std::move(data));

	// 2. 이미 보내는 중이면 onWrite가 이어서 보내므로 여기서는 시작하지 않는다.
	// 같은 소켓에 async_write를 동시에 두 개 걸면 두 데이터가 섞여 전송될 수 있다.
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

	// 소켓을 닫으면 걸려 있던 async_read_until / async_write는 operation_aborted로 끝난다.
	boost::system::error_code ec;
	m_socket.shutdown(tcp::socket::shutdown_both, ec);
	m_socket.close(ec);

	// 닫혔다고 알린다 (목록에서 빠진다). 남은 콜백들이 끝나면 Session이 소멸한다.
	if (m_onClosed) {
		m_onClosed(m_id);
	}
}

void Session::startRead()
{
	// async_read_until: '\n'이 나올 때까지 받은 뒤 콜백을 호출한다.
	// m_readBuffer에 이미 '\n'이 들어 있으면(앞서 여러 줄이 한꺼번에 왔으면) 바로 완료된다.
	// maxLineLength를 넘도록 '\n'이 없으면 not_found 오류로 끝난다.
	asio::async_read_until(m_socket, asio::dynamic_buffer(m_readBuffer, maxLineLength), '\n',
		[self = shared_from_this()](const boost::system::error_code& ec, std::size_t length) {
			self->onRead(ec, length);
		});
}

void Session::onRead(const boost::system::error_code& ec, std::size_t length)
{
	if (ec) {
		if (ec == asio::error::eof) {
			std::cout << "Client disconnected: " << m_name << '\n';
		}
		else if (ec == asio::error::not_found) {
			std::cerr << "Line too long from " << m_name << ", closing\n";
		}
		else if (ec != asio::error::operation_aborted) {
			std::cerr << "Read error from " << m_name << ": " << ec.message() << '\n';
		}
		close();
		return;
	}

	// 1. 버퍼에서 한 줄을 꺼낸다 ('\n' 제외). '\n' 뒤에 더 받은 데이터는 버퍼에 남겨 다음 줄로 쓴다.
	std::string line = m_readBuffer.substr(0, length - 1);
	m_readBuffer.erase(0, length);

	// 2. 처리는 콜백에 맡긴다. 무엇을 할지는 Session이 모른다.
	if (m_onMessage) {
		m_onMessage(shared_from_this(), std::move(line));
	}

	// 3. 처리 중에 닫히지 않았으면 바로 다음 줄을 받는다.
	if (!m_closed) {
		startRead();
	}
}

void Session::startWrite()
{
	// asio::async_write: 맨 앞 데이터를 전부 보내면 콜백이 호출된다.
	// 데이터는 큐 안의 shared_ptr가 붙잡고 있으므로 보내는 동안 사라지지 않는다.
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

	// 다 보낸 데이터를 빼고, 남은 게 있으면 이어서 보낸다.
	m_writeQueue.pop_front();
	if (!m_writeQueue.empty()) {
		startWrite();
	}
}
