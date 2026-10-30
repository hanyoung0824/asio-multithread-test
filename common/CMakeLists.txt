# 서버와 클라이언트가 함께 쓰는 코드 (패킷 정의, 만들기, 읽기)
add_library(common STATIC
	Packet.hpp
	PacketWriter.hpp
	PacketWriter.cpp
	PacketReader.hpp
	PacketReader.cpp
)

# common을 링크하는 쪽에서 #include "Packet.hpp"처럼 쓸 수 있도록 이 폴더를 include 경로로 공개한다.
target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
