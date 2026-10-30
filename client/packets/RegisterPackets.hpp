#pragma once

// 클라이언트가 처리할 패킷 클래스를 모두 등록한다.
// 새 패킷은 packets/에 IPacketHandler를 구현한 클래스를 만들고 RegisterPackets.cpp에 한 줄 추가한다.
// 시작할 때(세션이 생기기 전에) 한 번만 호출한다.
void registerPackets();
