add_executable(client
	main.cpp
	Connector.hpp
	Connector.cpp
	Session.hpp
	Session.cpp
	IPacketHandler.hpp
	PacketSession.hpp
	PacketSession.cpp
	packets/ChatNotifyPacket.hpp
	packets/ChatNotifyPacket.cpp
	packets/RegisterPackets.hpp
	packets/RegisterPackets.cpp
)

# packets/ 안의 파일에서도 #include "Session.hpp"처럼 쓸 수 있도록 client 폴더를 include 경로에 넣는다.
target_include_directories(client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(client PRIVATE common Boost::asio Threads::Threads)
