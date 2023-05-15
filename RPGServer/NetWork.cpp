
#include "NetWork.h"
#include "Player.h"
#include"Zone.h"
#include<iostream>

std::array<Object*, MAXOBJECT> objects;
HANDLE h_iocp;
bool IsNight; 

SOCKET g_s_socket, g_c_socket;
std::array <std::array<class ZoneManager*, ZONE_Y>, ZONE_X> zone;
concurrency::concurrent_priority_queue <EVENT> timer_queue;


//std::shared_lock<std::shared_mutex> lock(player->_s_lock);
//std::unique_lock<std::shared_mutex> lock(player->_s_lock);

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

void WSA_OVER_EX::processpacket(int o_id, char* pk)
{
	unsigned char packet_type = pk[1];



	switch (packet_type)
	{
	case CS_LOGIN:
	{
		Player* player = reinterpret_cast<Player*>(objects[o_id]);
		CS_LOGIN_PACKET* packet = reinterpret_cast<CS_LOGIN_PACKET*>(pk);
		player->_x = rand() % W_WIDTH;
		player->_y = rand() % W_HEIGHT;

		/*player->_x = 49 + o_id * 1;
		player->_y = 49 + o_id * 1;*/
		int my_zoneY, my_zoneX;
		my_zoneY = player->_y / ZONE_SEC;
		my_zoneX = player->_x / ZONE_SEC;
		zone[my_zoneY][my_zoneX]->ADD(player->_id);

		strcpy_s(player->_name, packet->name);
		player->send_login_info_packet();
		{
			std::unique_lock<std::shared_mutex> lock(player->_s_lock);
			player->_state = ST_INGAME;
		}
		set<int> z_list;
		zone_check(player->_x, player->_y, z_list);
		for (auto& p_id : z_list) {
			Player* pl = reinterpret_cast<Player*>(objects[p_id]);
			{
				std::shared_lock<std::shared_mutex> lock(pl->_s_lock);
				if (pl->_state != ST_INGAME)
				{
					continue;
				}
			}

			if (pl->_id == player->_id) continue;
			if (can_see(player->_id, pl->_id) == false) continue;

			pl->send_add_object_packet(player->_id);
			player->send_add_object_packet(pl->_id);
			/*if (pl._id >= MAX_USER)
			{
				bool before_wake = clients[pl._id]._n_wake;
				if (before_wake == false)
				{
					if (CAS(&clients[pl._id]._n_wake, before_wake, true))
						wake_up_npc(pl._id);
				}
			}*/
		}
		break;
	}
	case CS_MOVE:
	{
		Player* player = reinterpret_cast<Player*>(objects[o_id]);
		CS_MOVE_PACKET* packet = reinterpret_cast<CS_MOVE_PACKET*>(pk);
		player->_last_move_time = packet->move_time;
		short x = player->_x;
		short y = player->_y;
		switch (packet->direction) {
		case 0: if (y > 0) y--; break;
		case 1: if (y < W_HEIGHT - 1) y++; break;
		case 2: if (x > 0) x--; break;
		case 3: if (x < W_WIDTH - 1) x++; break;
		}
		player->_x = x;
		player->_y = y;
		unordered_set <int> old_vl;
		{
			std::shared_lock<std::shared_mutex> lock(player->_vl);
			old_vl = player->_view_list;
		}

		unordered_set <int> new_vl;

		set<int> z_list;
		zone_check(player->_x, player->_y, z_list);
		for (auto& p_id : z_list) {
			if (!is_NPC(p_id))
			{
				Player* pl = reinterpret_cast<Player*>(objects[p_id]);
				if (player->_id == 0)
					cout << p_id << endl;
				{
					shared_lock<shared_mutex> lock(pl->_s_lock);
					if (pl->_state == ST_INGAME)
					{
					}
					else if (pl->_state == ST_FREE || pl->_state == ST_ALLOC)
					{
						continue;
					}
				}
				if (pl->_id == o_id) continue;
			}


			if (can_see(p_id, o_id)) {
				new_vl.insert(p_id);
			}
		}


		for (auto& o : new_vl) {
			if (!is_NPC(o))
			{
				Player* pl = reinterpret_cast<Player*>(objects[o]);

				if (old_vl.count(o) == 0) {
					pl->send_add_object_packet(o_id);
					player->send_add_object_packet(o);
				}
				else {
					pl->send_move_packet(o_id);
					player->send_move_packet(o);
				}
			}
			/*else
			{
				bool before_wake = clients[o]._n_wake;
				if (before_wake == false)
				{
					if (cas(&clients[o]._n_wake, before_wake, true))
						wake_up_npc(o);
				}
			}*/
		}
		player->send_move_packet(o_id);

		for (auto& p_id : z_list) {

			if (!is_NPC(p_id))
			{
				Player* pl = reinterpret_cast<Player*>(objects[p_id]);
				if (pl->_state != ST_INGAME) continue;
				if (pl->_id == o_id) continue;
				if (new_vl.count(p_id) == 0)
				{
					player->send_remove_object_packet(p_id);
					if (!is_NPC(p_id))
					{
						pl->send_remove_object_packet(o_id);
					}
				}
			}
			else
			{
				if (objects[p_id]->_id == player->_id) continue;
				if (new_vl.count(p_id) == 0)
				{
					player->send_remove_object_packet(p_id);

				}

			}
			
		}
		break;
	}
	default:
	{
		if (o_id >= 0 && o_id < MAX_USER)
			closesocket(reinterpret_cast<Player*>(objects[o_id])->_socket);
		DebugBreak();
		break;
	}
	}
}


void WSA_OVER_EX::disconnect(int o_id)	//시야처리 안된듯?
{
	Player* player = reinterpret_cast<Player*>(objects[o_id]);
	for (int p_id = 0; p_id < MAX_USER; ++p_id) {
		Player* pl = reinterpret_cast<Player*>(objects[p_id]);
		{
			shared_lock<shared_mutex> lock(pl->_s_lock);
			if (pl->_state != ST_INGAME)
			{
				continue;
			}
		}
		if (pl->_id == o_id) continue;
		pl->send_remove_object_packet(o_id);
		
	}
	closesocket(player->_socket);
	{
		std::unique_lock<std::shared_mutex> lock(player->_s_lock);
		player->_state = ST_FREE;
	}

	
}

void WSA_OVER_EX::do_npc_ramdom_move(int n_id)
{
}




void WSA_OVER_EX::set_accept_over()
{
	_iocpop = IOCPOP::OP_ACCEPT;
}

void WSA_OVER_EX::zone_check(int x, int y, set<int>& z_list)
{
	int my_zoneY, my_zoneX;
	my_zoneY = y / ZONE_SEC;
	my_zoneX = x / ZONE_SEC;

	zone[my_zoneY][my_zoneX]->Zonelist(z_list);	//자기 zone에 있는 object 리스트에 추가
	
	char Zonediagonal_X = 0;
	char Zonediagonal_Y = 0;

	if (y % ZONE_SEC < VIEW_RANGE)
	{
		if (my_zoneY)
		{
			Zonediagonal_Y = -1;
			zone[my_zoneY + Zonediagonal_Y][my_zoneX]->Zonelist(z_list);
		}
	}
	else if (ZONE_SEC - (y % ZONE_SEC) < VIEW_RANGE)
	{
		if (my_zoneY < ZONE_Y - 1)
		{
			Zonediagonal_Y = 1;
			zone[my_zoneY + Zonediagonal_Y][my_zoneX]->Zonelist(z_list);
		}
	}


	if (x % ZONE_SEC < VIEW_RANGE)
	{
		if (my_zoneX)
		{
			Zonediagonal_X = -1;
			zone[my_zoneY][my_zoneX + Zonediagonal_X]->Zonelist(z_list);
		}
	}
	else if (ZONE_SEC - (y % ZONE_SEC) < VIEW_RANGE)
	{
		if (my_zoneX < ZONE_X - 1)
		{
			Zonediagonal_X = 1;
			zone[my_zoneY][my_zoneX + Zonediagonal_X]->Zonelist(z_list);
		}
	}

	if (Zonediagonal_X != 0 && Zonediagonal_Y != 0)
	{
		zone[my_zoneY + Zonediagonal_Y][my_zoneX + Zonediagonal_X]->Zonelist(z_list);
	}

	
}


bool CAS(volatile int* addr, int expected, int update)
{
	return std::atomic_compare_exchange_strong(reinterpret_cast <volatile std::atomic_int*>(addr), &expected, update);
}


int get_new_player_id()
{
	for (int i = 0; i < MAX_USER; ++i) {
		Player* player = reinterpret_cast<Player*>(objects[i]);
		std::shared_lock<std::shared_mutex> lock(player->_s_lock);
		if (player->_state == ST_FREE)
			return i;
	}
	return -1;
}

bool can_see(int o1, int o2)
{
	if (abs(objects[o1]->_x - objects[o2]->_x) > VIEW_RANGE) return false;
	if (abs(objects[o1]->_y - objects[o2]->_y) > VIEW_RANGE) return false;
	return true;
}

bool is_NPC(int _id) {
	if (_id < MAX_USER)
		return false;
	return true;
}

EVENT::EVENT()
{
}
