add_executable(server
	main.cpp
	Listener.hpp
	Listener.cpp
	Session.hpp
	Session.cpp
	SessionManager.hpp
	SessionManager.cpp
	PacketHandler.hpp
	PacketHandler.cpp
)
target_link_libraries(server PRIVATE common Boost::asio Threads::Threads)
