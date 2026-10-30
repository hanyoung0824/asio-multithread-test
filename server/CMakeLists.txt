add_executable(server
	main.cpp
	Listener.hpp
	Listener.cpp
	Session.hpp
	Session.cpp
	SessionManager.hpp
	SessionManager.cpp
	IPacketHandler.hpp
	PacketSession.hpp
	PacketSession.cpp
	packets/ChatPacket.hpp
	packets/ChatPacket.cpp
	packets/RegisterPackets.hpp
	packets/RegisterPackets.cpp
)

# packets/ 안의 파일에서도 #include "Session.hpp"처럼 쓸 수 있도록 server 폴더를 include 경로에 넣는다.
target_include_directories(server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(server PRIVATE common Boost::asio Threads::Threads)
