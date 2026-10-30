add_executable(client
	main.cpp
	Connector.hpp
	Connector.cpp
	Session.hpp
	Session.cpp
	PacketHandler.hpp
	PacketHandler.cpp
)
target_link_libraries(client PRIVATE common Boost::asio Threads::Threads)
