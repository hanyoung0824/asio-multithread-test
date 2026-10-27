add_executable(client
	main.cpp
	Connector.hpp
	Connector.cpp
	Session.hpp
	Session.cpp
)
target_link_libraries(client PRIVATE Boost::asio Threads::Threads)
