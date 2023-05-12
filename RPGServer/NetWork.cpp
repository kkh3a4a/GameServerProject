
#include "NetWork.h"
#include "Player.h"
#include<iostream>

std::array<Object*, MAXOBJECT> objects;
HANDLE h_iocp;
bool IsNight;


SOCKET g_s_socket, g_c_socket;


WSA_OVER_EX::WSA_OVER_EX()
{
	return;
}

WSA_OVER_EX::WSA_OVER_EX(IOCPOP iocpop, unsigned char byte, void* buf)
{
	ZeroMemory(&_wsaover, sizeof(_wsaover));
	_iocpop = iocpop;
	_wsabuf.buf = reinterpret_cast<char*>(buf);
	_wsabuf.len = byte;
}

void WSA_OVER_EX::processpacket(int client_id, char* pk)
{
	unsigned char packet_type = pk[1];
	Player* player = reinterpret_cast<Player*>(objects[client_id]);

	switch (packet_type)
	{
	case CS_LOGIN:
	{
		CS_LOGIN_PACKET* packet = reinterpret_cast<CS_LOGIN_PACKET*>(pk);
		std::cout << client_id << " login : " << packet->name << std::endl;
		player->_x = rand() % W_WIDTH;
		player->_y = rand() % W_HEIGHT;
		{
			std::lock_guard<std::mutex> ll{ player->_s_lock };
			player->_state = ST_INGAME;
		}
		strcpy_s(player->_name, packet->name);
		player->send_login_info_packet();

		/*strcpy_s(clients[c_id]._name, p->name);
				clients[c_id].x = rand() % W_WIDTH;
				clients[c_id].y = rand() % W_HEIGHT;
				clients[c_id].send_login_info_packet();
				{
					lock_guard<mutex> ll{ clients[c_id]._s_lock };
					clients[c_id]._state = ST_INGAME;
				}
				for (auto& pl : clients) {
					{
						lock_guard<mutex> ll(pl._s_lock);
						if (pl._state == ST_INGAME)
						{
						}
						else if (pl._state == ST_FREE || pl._state == ST_ALLOC)
						{
							continue;
						}
						else if (pl._state == ST_UNUSED)
						{
							continue;
						}
					}
		
					if (pl._id == c_id) continue;
					if (can_see(c_id, pl._id) == false) continue;
		
					pl.send_add_player_packet(c_id);
					clients[c_id].send_add_player_packet(pl._id);
					if (pl._id >= MAX_USER)
					{
						bool before_wake = clients[pl._id]._n_wake;
						if (before_wake == false)
						{
							if (CAS(&clients[pl._id]._n_wake, before_wake, true))
								wake_up_npc(pl._id);
						}
					}
				}
				break;*/
		break;
	}
	//case CS_PACKET_MOVE:
	//{
	//	cs_packet_move* packet = reinterpret_cast<cs_packet_move*>(pk);
	//	player->w = packet->w;
	//	player->a = packet->a;
	//	player->s = packet->s;
	//	player->d = packet->d;
	//	break;
	//}
	//case CS_PACKET_CITIZENPLACEMENT:
	//{
	//	//추후 건물도 똑같이 작동할거니깐 건물도 추가해주면 좋음

	//	cs_packet_citizenplacement* packet = reinterpret_cast<cs_packet_citizenplacement*>(pk);
	//	if (packet->objectid >= RESOURCESTART && packet->objectid < RESOURCESTART + MAXRESOURCE)
	//	{
	//		Resource* resource = reinterpret_cast<Resource*> (objects[packet->objectid]);
	//		resource->set_resource_citizen_placement(client_id, packet->isplus);
	//	}
	//	else if (packet->objectid >= BUILDINGSTART && packet->objectid < BUILDINGSTART + MAXBUILDING)
	//	{
	//		Building* building = reinterpret_cast<Building*>(objects[packet->objectid]);
	//		building->set_building_citizen_placement(packet->isplus);
	//	}
	//	break;
	//}
	//case CS_PACKET_BUILDABLE:
	//{
	//	cs_packet_buildable* cs_packet = reinterpret_cast<cs_packet_buildable*>(pk);
	//	//std::cout << "buildable" << std::endl;
	//	sc_packet_buildable sc_packet;
	//	for (auto& obj : objects) {
	//		if (obj == nullptr)
	//			continue;
	//		if (obj->_x == 0) {
	//			continue;
	//		}
	//		if (obj->_x < cs_packet->x + 800 && obj->_x > cs_packet->x - 800 && obj->_y < cs_packet->y + 800 && obj->_y > cs_packet->y - 800) {
	//			sc_packet.buildable = false;
	//			break;
	//		}
	//		sc_packet.buildable = true;
	//	}
	//	player->send_packet(&sc_packet);
	//	break;
	//}
	//case CS_PACKET_BUILD:
	//{
	//	cs_packet_build* cs_packet = reinterpret_cast<cs_packet_build*>(pk);
	//	sc_packet_build sc_packet;

	//	for (int i = BUILDINGSTART + PLAYERBUILDINGCOUNT * player->_id; i < BUILDINGSTART + PLAYERBUILDINGCOUNT * player->_id + PLAYERBUILDINGCOUNT; ++i) {
	//		Building* building = reinterpret_cast<Building*>(objects[i]);
	//		if (building->_type != -1)
	//			continue;
	//		sc_packet.id = building->_id;
	//		sc_packet.x = cs_packet->x;
	//		sc_packet.y = cs_packet->y;
	//		sc_packet.building_type = cs_packet->building_type;
	//		sc_packet.do_build = building->_create_building(cs_packet->x, cs_packet->y, cs_packet->building_type, i);
	//		break;
	//	}

	//	for (int player_num = 0; player_num < MAXPLAYER; player_num++) {	//모든 플레이어들에게 전송
	//		Player* this_player = reinterpret_cast<Player*>(objects[player_num]);
	//		this_player->send_packet(&sc_packet);
	//	}
	//	break;
	//}
	//case CS_PACKET_MINIMAP:
	//{
	//	cs_packet_minimap* packet = reinterpret_cast<cs_packet_minimap*>(pk);

	//	//std::cout<<packet->x << ", " << packet->y << std::endl;
	//	player->playerMinimapLocation(packet->x, packet->y);
	//	break;
	//}

	default:
	{
		closesocket(reinterpret_cast<Player*>(objects[client_id])->_socket);
		DebugBreak();
		break;
	}
	}
}

void WSA_OVER_EX::disconnect(int p_id)
{
}

void WSA_OVER_EX::do_npc_ai(int n_id)
{
}




void WSA_OVER_EX::set_accept_over()
{
	_iocpop = IOCPOP::OP_ACCEPT;
}


bool CAS(volatile int* addr, int expected, int update)
{
	return std::atomic_compare_exchange_strong(reinterpret_cast <volatile std::atomic_int*>(addr), &expected, update);
}


int get_new_player_id()
{
	for (int i = 0; i < MAX_USER; ++i) {
		Player* player = reinterpret_cast<Player*>(objects[i]);
		std::lock_guard <std::mutex> ll{ player->_s_lock };
		if (player->_state == ST_FREE || player->_state == ST_UNUSED)
			return i;
	}
	return -1;
}