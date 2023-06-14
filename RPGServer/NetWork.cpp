
#include "NetWork.h"
#include "Player.h"
#include"DefaultNPC.h"
#include"Zone.h"
#include<iostream>

std::array<Object*, MAXOBJECT> objects;
HANDLE h_iocp;
bool IsNight; 

SOCKET g_s_socket, g_c_socket;
std::array <std::array<class ZoneManager*, ZONE_Y>, ZONE_X> zone;
concurrency::concurrent_priority_queue <EVENT> timer_queue;
SOCKET DB_socket;
WSA_OVER_EX DB_wsa_recv_over;
int DB_prev_size = 0;
std::map<std::pair<short, short>, short> World_Map;
std::set<int> login_player;
std::shared_mutex login_lock;
//std::shared_lock<std::shared_mutex> lock(player->_s_lock);
//std::unique_lock<std::shared_mutex> lock(player->_s_lock);


WSA_OVER_EX::WSA_OVER_EX()
{
	ZeroMemory(&_wsaover, sizeof(_wsaover));
	_wsabuf.len = BUF_SIZE;
	_wsabuf.buf = _buf;
	_iocpop = OP_RECV;
	_e_type = EV_ATTACK;
}

WSA_OVER_EX::WSA_OVER_EX(IOCPOP iocpop, unsigned short byte, void* buf)
{
	ZeroMemory(&_wsaover, sizeof(_wsaover));
	_iocpop = iocpop;
	_wsabuf.buf = reinterpret_cast<char*>(buf);
	_wsabuf.len = byte;
}

void WSA_OVER_EX::processpacket(int o_id, void* pk)
{
	char* packet_data = static_cast<char*>(pk);
	unsigned char packet_type = packet_data[2];



	switch (packet_type)
	{
	case CS_LOGIN:
	{
		
		Player* player = reinterpret_cast<Player*>(objects[o_id]);
		CS_LOGIN_PACKET* packet = reinterpret_cast<CS_LOGIN_PACKET*>(pk);
		string str = packet->name;
		str.erase(0, 1); // p 제거
		int id = std::stoi(str); // 문자열을 int형으로 변환합니다.
		player->_db_id = id;
		std::shared_lock<std::shared_mutex> loacl_lock(login_lock);
		auto f = login_player.find(id);
		auto e = login_player.end();
		loacl_lock.unlock();


		if(f == e)
		{
			std::unique_lock<std::shared_mutex> loacl_lock2(login_lock);
			login_player.insert(id);
			loacl_lock2.unlock();
			strcpy_s(player->_name, packet->name);
			DB_player_login(player->_db_id, player->_name, o_id);
		}
		else
		{
			SC_LOGIN_FAIL_PACKET s_packet;
			s_packet.size = sizeof(SC_LOGIN_FAIL_PACKET);
			s_packet.type = SC_LOGIN_FAIL;
			player->send_packet(&s_packet);
			login_fail_disconnect(o_id);
		}

		break;
	}
	case CS_MOVE:
	{
		Player* player = reinterpret_cast<Player*>(objects[o_id]);
		CS_MOVE_PACKET* packet = reinterpret_cast<CS_MOVE_PACKET*>(pk);
		if (player->_p_last_move_time < (chrono::system_clock::now() - chrono::milliseconds(100)))
		{
			player->_p_last_move_time = chrono::system_clock::now();
			short x = player->_x;
			short y = player->_y;
			int b_my_zoneY, b_my_zoneX;
			b_my_zoneY = player->_y / ZONE_SEC;
			b_my_zoneX = player->_x / ZONE_SEC;
			switch (packet->direction) {
			case 0: if (y > 0) y--; break;
			case 1: if (y < W_HEIGHT - 1) y++; break;
			case 2: if (x > 0) x--; break;
			case 3: if (x < W_WIDTH - 1) x++; break;
			}
			if (World_Map.find(make_pair(x, y)) == World_Map.end())
			{

				player->_x = x;
				player->_y = y;
				int my_zoneY, my_zoneX;
				my_zoneY = player->_y / ZONE_SEC;
				my_zoneX = player->_x / ZONE_SEC;

				if (my_zoneX != b_my_zoneX || my_zoneY != b_my_zoneY)
				{
					zone[b_my_zoneY][b_my_zoneX]->REMOVE(player->_id);
					zone[my_zoneY][my_zoneX]->ADD(player->_id);
				}

				unordered_set <int> old_vl;
				{
					std::shared_lock<std::shared_mutex> lock(player->_vl);
					old_vl = player->_view_list;
				}

				unordered_set <int> new_vl;

				set<int> z_list;
				zone_check(player->_x, player->_y, z_list);
				for (auto& p_id : z_list) {
					{
						std::shared_lock<std::shared_mutex> lock(objects[p_id]->_s_lock);
						if (objects[p_id]->_state != ST_INGAME)
						{
							lock.unlock();
							continue;
						}
					}
					if (objects[p_id]->_id == o_id) continue;
					if (can_see(objects[p_id]->_id, o_id)) {
						new_vl.insert(objects[p_id]->_id);
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
					else
					{
						NPC* nl = reinterpret_cast<NPC*>(objects[o]);

						bool before_wake = nl->_n_wake;
						if (old_vl.count(o) == 0) {
							nl->add_objects(o_id);
							player->send_add_object_packet(o);
						}
						else {
							player->send_move_packet(o);

						}
						if (before_wake == 0)
						{
							if (CAS(reinterpret_cast<volatile int*>(&nl->_n_wake), before_wake, 1))
								wake_up_npc(o);
						}
					}
				}
				player->send_move_packet(o_id);

				for (auto& p_id : old_vl) {
					if (objects[p_id]->_state != ST_INGAME) continue;
					if (objects[p_id]->_id == o_id) continue;
					if (new_vl.count(p_id) == 0)
					{
						player->send_remove_object_packet(p_id);
						if (!is_NPC(p_id))
						{
							Player* pl = reinterpret_cast<Player*>(objects[p_id]);
							pl->send_remove_object_packet(o_id);
						}
						else {
							NPC* npc = reinterpret_cast<NPC*>(objects[p_id]);
						}
					}
				}
				player->_movecount++;
				if (player->_movecount > 20)
				{
					player->_movecount = 0;
					player->send_location_DB();
				}
			}
		}

		break;
	}
	case CS_ATTACK:
	{
		CS_ATTACK_PACKET* packet = reinterpret_cast<CS_ATTACK_PACKET*>(pk);
		Player* player = reinterpret_cast<Player*>(objects[o_id]);
		{
			if(player->_last_attack_time < chrono::system_clock::now() - 1s)
			{
				SC_ATTACK_PACKET packet;
				packet.size = sizeof(SC_ATTACK_PACKET);
				packet.type = SC_ATTACK;
				player->send_packet(&packet);

				player->_last_attack_time = chrono::system_clock::now();
				shared_lock<shared_mutex> lock(player->_vl);
				for (auto& v_id : player->_view_list)
				{
					if (v_id < MAX_USER)
						continue;
					{
						shared_lock<shared_mutex> slock(objects[v_id]->_s_lock);
						if (objects[v_id]->_state != ST_INGAME)
							continue;
					}

					WSA_OVER_EX* w_ex = new WSA_OVER_EX;
					w_ex->_causeId = o_id;
					w_ex->_iocpop = OP_NPC_DEFENCE;
					PostQueuedCompletionStatus(h_iocp, 1, v_id, &w_ex->_wsaover);
				}
			}
			
		}
		break;
	}
	case CS_CHAT:
	{
		CS_CHAT_PACKET* packet = reinterpret_cast<CS_CHAT_PACKET*>(pk);
		set<int> z_list;
		zone_check(objects[o_id]->_x, objects[o_id]->_y, z_list);

		for (auto& p_id : z_list) {
			if (p_id >= MAX_USER)
				break;
			if (can_see(objects[p_id]->_id, o_id))
			{
				Player* player = reinterpret_cast<Player*>(objects[p_id]);
				player->send_chat_packet(o_id, packet->mess);
			}
		}
		int length = std::strlen(packet->mess);
		string msg(packet->mess, length);
		msg += '\0';
		SD_CHAT_PACKET d_packet;
		std::time_t currentTime = std::time(nullptr);
		std::tm localTime;
		localtime_s(&localTime, &currentTime);
		Player* player = reinterpret_cast<Player*>(objects[o_id]);

		strcpy_s(d_packet.mess, msg.c_str());
		d_packet.type = SD_CHAT;
		d_packet.size = sizeof(SD_CHAT_PACKET) - (CHAT_SIZE - msg.size());
		d_packet.id = player->_db_id;

		std::strftime(d_packet.time, sizeof(d_packet.time), "%Y-%m-%d %H:%M:%S", &localTime);
		DB_send_packet(&d_packet);
		break;

	}
	case DS_PLAYER_LOGIN:
	{
		DS_PLAYER_LOGIN_PACKET* packet = reinterpret_cast<DS_PLAYER_LOGIN_PACKET*>(pk);

		Player* player = reinterpret_cast<Player*>(objects[packet->s_id]);
		cout << "LOGIN : " << packet->id << endl;
		player->_x = packet->x;
		player->_y = packet->y;
		player->_max_hp = packet->max_hp;
		player->_hp = packet->hp;
		player->_level = packet->level;
		player->_exp = packet->exp;
		while (1)
		{
			if (World_Map.find(make_pair(player->_x, player->_y)) == World_Map.end())
				break;
			
			if (player->_x < 1100)
			{
				player->_x += 1;
				continue;
			}
			if(player->_x > 1000)
			{
				player->_x += 2;
			}

			
		}



		int my_zoneY, my_zoneX;
		my_zoneY = player->_y / ZONE_SEC;
		my_zoneX = player->_x / ZONE_SEC;
		zone[my_zoneY][my_zoneX]->ADD(player->_id);

		player->send_login_info_packet();
		{
			std::unique_lock<std::shared_mutex> lock(player->_s_lock);
			player->_state = ST_INGAME;
		}
		set<int> z_list;
		zone_check(player->_x, player->_y, z_list);
		for (auto& p_id : z_list) {
			{
				std::shared_lock<std::shared_mutex> lock(objects[p_id]->_s_lock);
				if (objects[p_id]->_state != ST_INGAME)
				{
					continue;
				}
			}

			if (objects[p_id]->_id == player->_id) continue;
			if (can_see(player->_id, objects[p_id]->_id) == false) continue;

			if (!is_NPC(p_id))
			{
				Player* pl = reinterpret_cast<Player*>(objects[p_id]);
				pl->send_add_object_packet(player->_id);
			}
			else
			{
				NPC* nl = reinterpret_cast<NPC*>(objects[p_id]);
				nl->add_objects(player->_id);
				bool before_wake = nl->_n_wake;
				if (before_wake == 0)
				{
					if (CAS(reinterpret_cast<volatile int*>(&nl->_n_wake), before_wake, 1))
						wake_up_npc(nl->_id);
				}
			}
			player->send_add_object_packet(objects[p_id]->_id);
		}
		player->_dmg = 10;
		for (int i = player->_level - 1; i > 0; --i)
		{
			player->_dmg += i;
		}
		if (player->_hp != player->_max_hp)
		{
			EVENT ev{ player->_id, EV_HEAL, chrono::system_clock::now() + 5s };
			//l_q.lock();
			timer_queue.push(ev);
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


void WSA_OVER_EX::disconnect(int o_id)
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
		std::unique_lock<std::shared_mutex> loacl_lock2(login_lock);
		login_player.erase(player->_db_id);
		loacl_lock2.unlock();
	}

	{
		std::unique_lock<std::shared_mutex> lock(player->_s_lock);
		player->_state = ST_FREE;
	}
}

void WSA_OVER_EX::login_fail_disconnect(int o_id)
{
	Player* player = reinterpret_cast<Player*>(objects[o_id]);
	
	closesocket(player->_socket);
	
	{
		std::unique_lock<std::shared_mutex> lock(player->_s_lock);
		player->_state = ST_FREE;
	}
}
void WSA_OVER_EX::wake_up_npc(int n_id)
{
	NPC* npc = reinterpret_cast<NPC*>(objects[n_id]);
	if(!npc->_is_batte)
	{
		if(npc->_n_type == 1 || npc->_n_type == 3)
		{
			EVENT ev(n_id, EV_RANDOM_MOVE, chrono::system_clock::now());
			//l_q.lock();
			timer_queue.push(ev);
		}
		else if (npc->_n_type == 2 || npc->_n_type == 4)
		{
			EVENT ev(n_id, EV_WAIT, chrono::system_clock::now());
			//l_q.lock();
			timer_queue.push(ev);
		}
	}
	else{
		if (npc->_n_type == 1 || npc->_n_type == 3)
		{
			EVENT ev(n_id, EV_MOVE, chrono::system_clock::now());
			//l_q.lock();
			timer_queue.push(ev);
		}
		else if (npc->_n_type == 2 || npc->_n_type == 4)
		{
			EVENT ev(n_id, EV_RANGEATTACK, chrono::system_clock::now());
			//l_q.lock();
			timer_queue.push(ev);
		}
	}
	//l_q.unlock();
}

void WSA_OVER_EX::do_npc_ramdom_move(int n_id)
{
	NPC* npc = reinterpret_cast<NPC*>(objects[n_id]);
	if (npc->_is_batte)
	{
		do_npc_move(n_id);
		return;
	}
	if (npc->_state != ST_INGAME)	// 간혹 한번 더 이동하기 vs lock 걸기 , lock걸지말자. 
		return;
	
	short x = npc->_x;
	short y = npc->_y;

	unordered_set<int> near_list;
	unordered_set<int> old_vlist;
	set<int> p_list;
	zone_check(npc->_x, npc->_y, p_list);
	for (auto& p_id : p_list) {
		if (objects[p_id]->_state != ST_INGAME) continue;
		if (can_see(p_id, n_id) == false) continue;
		if (p_id >= MAX_USER)
			break;
		old_vlist.insert(p_id);
	}
	int b_my_zoneY, b_my_zoneX;
	b_my_zoneY = npc->_y / ZONE_SEC;
	b_my_zoneX = npc->_x / ZONE_SEC;
	switch (rand() % 4) {
	case 0: if (y > 0) y--; break;
	case 1: if (y < W_HEIGHT - 1) y++; break;
	case 2: if (x > 0) x--; break;
	case 3: if (x < W_WIDTH - 1) x++; break;
	}
	if (World_Map.find(make_pair(x, y)) == World_Map.end())
	{
		npc->_x = x;
		npc->_y = y;
		int my_zoneY, my_zoneX;
		my_zoneY = npc->_y / ZONE_SEC;
		my_zoneX = npc->_x / ZONE_SEC;
		if (my_zoneX != b_my_zoneX || my_zoneY != b_my_zoneY)
		{
			zone[b_my_zoneY][b_my_zoneX]->REMOVE(npc->_id);
			zone[my_zoneY][my_zoneX]->ADD(npc->_id);
		}

		set<int> z_list;
		zone_check(npc->_x, npc->_y, z_list);
		for (auto& p_id : z_list) {
			if (p_id >= MAX_USER)
				break;
			Player* pl = reinterpret_cast<Player*>(objects[p_id]);
			if (pl->_state != ST_INGAME) continue;
			if (can_see(pl->_id, n_id))
				near_list.insert(pl->_id);
		}

		for (auto& p_id : near_list) {
			Player* pl = reinterpret_cast<Player*>(objects[p_id]);
			if(npc->_n_type == 3)
			{
				if (is_agro(pl->_id, n_id)) {
					npc->_is_batte = true;
					do_npc_move(n_id);
					return;
				}
			}
			
			std::shared_lock<std::shared_mutex> lock(pl->_vl);
			

			if (pl->_view_list.count(n_id)) {
				lock.unlock();
				pl->send_move_packet(n_id);
			}
			else {
				lock.unlock();
				pl->send_add_object_packet(n_id);
			}
		}
		for (auto& p_id : old_vlist) {
			if (0 == near_list.count(p_id)) {
				Player* pl = reinterpret_cast<Player*>(objects[p_id]);
				pl->send_remove_object_packet(n_id);
			}
		}


		if (near_list.size() == 0)
		{
			bool before_wake = npc->_n_wake;
			if (before_wake == 1)
			{
				if (!CAS(reinterpret_cast<volatile int*>(&npc->_n_wake), before_wake, 0))
				{

				}
				else
				{
					return;
				}
			}
			else
				return;
		}
	}

	EVENT ev{ n_id, EV_RANDOM_MOVE,chrono::system_clock::now() + 1s };
	//l_q.lock();
	timer_queue.push(ev);
	//l_q.unlock();	
}

void WSA_OVER_EX::do_npc_wait(int n_id)
{
	NPC* npc = reinterpret_cast<NPC*>(objects[n_id]);
	if (npc->_is_batte)
	{
		npc->do_range_attack();
		return;
	}
	if (npc->_state != ST_INGAME)	// 간혹 한번 더 이동하기 vs lock 걸기 , lock걸지말자. 
		return;

	unordered_set<int> near_list;

	set<int> z_list;
	zone_check(npc->_x, npc->_y, z_list);
	for (auto& p_id : z_list) {
		if (p_id >= MAX_USER)
			break;
		Player* pl = reinterpret_cast<Player*>(objects[p_id]);
		if (pl->_state != ST_INGAME) continue;
		if (can_see(pl->_id, n_id))
			near_list.insert(pl->_id);
	}
	for (auto& p_id : near_list) {
		Player* pl = reinterpret_cast<Player*>(objects[p_id]);
		std::shared_lock<std::shared_mutex> lock(pl->_vl);
		if (npc->_n_type == 4)
		{
			if (is_agro(pl->_id, n_id)) {
				npc->_is_batte = true;
				npc->do_range_attack();
				return;
			}
		}
		if (pl->_view_list.count(n_id)) {
			lock.unlock();
		}
		else {
			lock.unlock();
			pl->send_add_object_packet(n_id);
		}
	}
	if (near_list.size() == 0)
	{
		bool before_wake = npc->_n_wake;
		if (before_wake == 1)
		{
			if (!CAS(reinterpret_cast<volatile int*>(&npc->_n_wake), before_wake, 0))
			{

			}
			else
			{
				return;
			}
		}
		else
			return;
	}
	

	EVENT ev{ n_id, EV_WAIT,chrono::system_clock::now() + 1s };
	timer_queue.push(ev);
}

void WSA_OVER_EX::do_npc_move(int n_id)
{
	NPC* npc = reinterpret_cast<NPC*>(objects[n_id]);

	npc->move_NPC();
}

void WSA_OVER_EX::set_accept_over()
{
	_iocpop = IOCPOP::OP_ACCEPT;
}

void zone_check(int x, int y, set<int>& z_list)
{
	int my_zoneY, my_zoneX;
	my_zoneY = y / ZONE_SEC;
	my_zoneX = x / ZONE_SEC;

	zone[my_zoneY][my_zoneX]->Zonelist(z_list);	//자기 zone에 있는 object 리스트에 추가
	
	char Zonediagonal_X = 0;
	char Zonediagonal_Y = 0;

	if (y % ZONE_SEC <= VIEW_RANGE)
	{
		if (my_zoneY)
		{
			Zonediagonal_Y = -1;
			zone[my_zoneY + Zonediagonal_Y][my_zoneX]->Zonelist(z_list);
		}
	}
	else if (ZONE_SEC - (y % ZONE_SEC) <= VIEW_RANGE)
	{
		if (my_zoneY < ZONE_Y - 1)
		{
			Zonediagonal_Y = 1;
			zone[my_zoneY + Zonediagonal_Y][my_zoneX]->Zonelist(z_list);
		}
	}


	if (x % ZONE_SEC <= VIEW_RANGE)
	{
		if (my_zoneX)
		{
			Zonediagonal_X = -1;
			zone[my_zoneY][my_zoneX + Zonediagonal_X]->Zonelist(z_list);
		}
	}
	else if (ZONE_SEC - (x % ZONE_SEC) <= VIEW_RANGE)
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

int API_get_x(lua_State* L)
{
	int user_id =
		(int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	int x = objects[user_id]->_x;
	lua_pushnumber(L, x);
	return 1;
}

int API_get_y(lua_State* L)
{
	int user_id =
		(int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	int y = objects[user_id]->_y;
	lua_pushnumber(L, y);
	return 1;
}

int API_SendMessage(lua_State* L)
{
	int my_id = (int)lua_tointeger(L, -2);
	char* mess = (char*)lua_tostring(L, -1);
	lua_pop(L, 3);
	set<int> z_list;
	// 수정사항 : 시야 내에있는 모든 플레이어에게 chat을 보내게 구현하였습니다.
	zone_check(objects[my_id]->_x, objects[my_id]->_y, z_list);
	for (auto& p_id : z_list) {
		if (p_id >= MAX_USER)
			break;
		if(can_see(p_id, my_id))
		{
			Player* player = reinterpret_cast<Player*>(objects[p_id]);
			player->send_chat_packet(my_id, mess);
		}
	}
	return 0;
}

int API_Defence(lua_State* L)
{
	int atk_id = (int)lua_tointeger(L, -2);
	int def_id = (int)lua_tointeger(L, -1);
	NPC* npc = reinterpret_cast<NPC*>(objects[def_id]);

	if (objects[def_id]->_hp == objects[def_id]->_max_hp)
	{
		npc->heal_NPC();
	}
	if (def_id >= MAX_USER)
	{
		npc->_is_batte = true;
		npc->_last_attacker = atk_id;
	}
	objects[def_id]->_hp -= objects[atk_id]->_dmg;
	cout << "P" << reinterpret_cast<Player*>(objects[atk_id])->_db_id << " Attack " << "N" << def_id << "[ " << objects[atk_id]->_dmg << " damage ]" << endl;
	if (objects[def_id]->_hp <= 0)
	{
		if (def_id >= MAX_USER)
		{
			NPC* npc = reinterpret_cast<NPC*>(objects[def_id]);
			if (atk_id < MAX_USER)
			{
				Player* player = reinterpret_cast<Player*>(objects[atk_id]);
				player->kill_NPC(npc->_id);
			}
 			npc->dead_NPC();
		}
	}
	lua_pop(L, 3);
	{
		std::shared_lock<std::shared_mutex> lock(objects[def_id]->_vl);
		for (auto& p_id : objects[def_id]->_view_list) {
			if (p_id >= MAX_USER)
				continue;
			Player* player = reinterpret_cast<Player*>(objects[p_id]);
			player->send_change_hp(def_id);
		}
	}
	return 0;
}

int API_Default_Attack(lua_State* L)
{
	int atk_id = (int)lua_tointeger(L, -2);
	int def_id = (int)lua_tointeger(L, -1);
	if (objects[atk_id]->_hp <= 0)
	{
		lua_pop(L, 3);
		return 0;
	}
	if (objects[def_id]->_hp == objects[def_id]->_max_hp)
	{
		EVENT ev{ def_id, EV_HEAL, chrono::system_clock::now() + 5s };
		//l_q.lock();
		timer_queue.push(ev);
	}
	objects[def_id]->_hp -= objects[atk_id]->_dmg;
	cout << "N" << atk_id << " is Attack "<< reinterpret_cast<Player*>(objects[def_id])->_db_id << "[ " << objects[atk_id]->_dmg << " damage ]" << endl;
	lua_pop(L, 3);
	if (objects[def_id]->_hp <= 0)
	{
		Player* player = reinterpret_cast<Player*>(objects[def_id]);
		player->dead_player();
		if (atk_id >= MAX_USER)
		{
			NPC* npc = reinterpret_cast<NPC*>(objects[atk_id]);
			npc->_is_batte = false;
		}
	}

	{
		std::shared_lock<std::shared_mutex> lock(objects[def_id]->_vl);
		for (auto& p_id : objects[def_id]->_view_list) {
			if (p_id >= MAX_USER)
				continue;
			Player* player = reinterpret_cast<Player*>(objects[p_id]);
			player->send_change_hp(def_id);
		}
	}
	return 0;
}

int API_Range_Attack(lua_State* L)
{
	int atk_id = (int)lua_tointeger(L, -3);
	int def_id = (int)lua_tointeger(L, -2);
	int attack_time = (int)lua_tointeger(L, -1);
	if (objects[atk_id]->_hp <= 0)
	{
		lua_pop(L, 3);
		return 0;
	}
	if (objects[def_id]->_hp == objects[def_id]->_max_hp)
	{
		EVENT ev{ def_id, EV_HEAL, chrono::system_clock::now() + 5s };
		//l_q.lock();
		timer_queue.push(ev);
	}
	objects[def_id]->_hp -= objects[atk_id]->_dmg;
	cout << "N" << atk_id << " is Attack " << reinterpret_cast<Player*>(objects[def_id])->_db_id << "[ " << objects[atk_id]->_dmg << " damage ]" << endl;
	lua_pop(L, 4);

	if (objects[def_id]->_hp <= 0)
	{
		Player* player = reinterpret_cast<Player*>(objects[def_id]);
		player->dead_player();
		if (atk_id >= MAX_USER)
		{
			NPC* npc = reinterpret_cast<NPC*>(objects[atk_id]);
			npc->_is_batte = false;
		}
	}
	{
		std::shared_lock<std::shared_mutex> lock(objects[def_id]->_vl);
		for (auto& p_id : objects[def_id]->_view_list) {
			if (p_id >= MAX_USER)
				continue;
			Player* player = reinterpret_cast<Player*>(objects[p_id]);
			player->send_change_hp(def_id);
		}
	}

	return 0;
}


EVENT::EVENT()
{
}

void DB_send_packet(void* pk)
{
	char* buf = reinterpret_cast<char*>(pk);
	WSA_OVER_EX* _wsa_send_over = new WSA_OVER_EX(DB_SEND, buf[0], pk);

	WSASend(DB_socket, &_wsa_send_over->_wsabuf, 1, NULL, 0, &_wsa_send_over->_wsaover, NULL);
}


void DB_player_login(int id, char name[20], int s_id) {
	SD_PLAYER_LOGIN_PACKET packet;
	packet.id = id;
	strcpy_s(packet.name, name);
	packet.size = sizeof(packet);
	packet.type = SD_PLAYER_LOGIN;
	packet.s_id = s_id;
	DB_send_packet(&packet);

}

void DB_do_recv()
{
	DWORD recv_flag = 0;
	memset(&DB_wsa_recv_over._wsaover, 0, sizeof(DB_wsa_recv_over._wsaover));
	DB_wsa_recv_over._wsabuf.len = DB_BUF_SIZE - DB_prev_size;
	DB_wsa_recv_over._wsabuf.buf = DB_wsa_recv_over._buf + DB_prev_size;
	DB_wsa_recv_over._iocpop = DB_RECV;

	WSARecv(DB_socket, &DB_wsa_recv_over._wsabuf, 1, 0, &recv_flag, &DB_wsa_recv_over._wsaover, 0);
}

int is_agro(int o1, int o2)
{
	if (abs(objects[o1]->_x - objects[o2]->_x) > 5) return false;
	if (abs(objects[o1]->_y - objects[o2]->_y) > 5) return false;
	return ((abs(objects[o1]->_x - objects[o2]->_x) + (abs(objects[o1]->_y - objects[o2]->_y))));
}