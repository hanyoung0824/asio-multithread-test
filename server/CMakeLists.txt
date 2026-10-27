add_executable(server
	main.cpp
	Listener.hpp
	Listener.cpp
	Session.hpp
	Session.cpp
	SessionManager.hpp
	SessionManager.cpp
)
target_link_libraries(server PRIVATE Boost::asio Threads::Threads)
