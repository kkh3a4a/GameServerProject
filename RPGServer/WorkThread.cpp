#include"WorkThread.h"
#include"Player.h"
#include"Zone.h"
#include"DefaultNPC.h"
using namespace std;


void worker_thread(WSA_OVER_EX g_a_over)
{
	while (true) {
		DWORD num_bytes;
		ULONG_PTR key;
		WSAOVERLAPPED* over = nullptr;
		
		BOOL ret = GetQueuedCompletionStatus(h_iocp, &num_bytes, &key, &over, INFINITE);
    		WSA_OVER_EX* ex_over = reinterpret_cast<WSA_OVER_EX*>(over);
		if (FALSE == ret) {
			if (ex_over->_iocpop == OP_ACCEPT) cout << "Accept Error";
			else {
				cout << "GQCS Error on client[" << key << "]\n";
				if(key != 1234567)
					ex_over->disconnect(static_cast<int>(key));
				if (ex_over->_iocpop == OP_SEND) delete ex_over;
				continue;
			}
		}

		if ((0 == num_bytes) && ((ex_over->_iocpop == OP_RECV) || (ex_over->_iocpop == OP_SEND))) {
			ex_over->disconnect(static_cast<int>(key));
			if (ex_over->_iocpop == OP_SEND) delete ex_over;
			continue;
		}

		switch (ex_over->_iocpop) {
		case OP_ACCEPT: {
			int p_id = get_new_player_id();
			if (p_id != -1)
			{
				Player* player = reinterpret_cast<Player*>(objects[p_id]);
				{
					std::unique_lock<std::shared_mutex> lock(player->_s_lock);
					player->_state = ST_ALLOC;
				}
				player->_x = 0;
				player->_y = 0;
				player->_id = p_id;
				player->_name[0] = 0;
				player->_prev_size = 0;
				player->_socket = g_c_socket;
				player->_view_list.clear();
				CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_c_socket),
					h_iocp, p_id, 0);
				player->do_recv();
				g_c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			}
			else
			{
				cout << " Accept Error : MAXPLAYER connect" << endl;
			}
			ZeroMemory(&g_a_over._wsaover, sizeof(g_a_over._wsaover));
			int addr_size = sizeof(SOCKADDR_IN);
			AcceptEx(g_s_socket, g_c_socket, g_a_over._buf, 0, addr_size + 16, addr_size + 16, 0, &g_a_over._wsaover);
			break;
		}
		case OP_RECV: {

			Player* player = reinterpret_cast<Player*>(objects[key]);
			int remain_data = num_bytes + player->_prev_size;
			char* buf = ex_over->_buf;
			short* p = reinterpret_cast<short*>(ex_over->_buf);
			while (remain_data > 0) {
				int packet_size = p[0];
				if (packet_size <= remain_data) {
					ex_over->processpacket(static_cast<int>(key), p);
					buf = buf + packet_size;
					p = reinterpret_cast<short*>(buf);
					remain_data = remain_data - packet_size;
				}
				else break;
			}

			player->_prev_size = remain_data;
			if (remain_data > 0) {
				memcpy(ex_over->_buf, p, remain_data);
			}
			player->do_recv();

			break;
		}
		case OP_SEND:
		{
			delete ex_over;
			break;
		}
		case OP_NPC_RANDOMMOVE:
		{
			ex_over->do_npc_ramdom_move(static_cast<int>(key));

			delete ex_over;
			break;
		}
		case OP_NPC_RESPAWN:
		{

			if(key >= MAX_USER)
			{
				NPC* npc = reinterpret_cast<NPC*>(objects[key]);
				{
					npc->_n_wake = false;
					//cout << "respawn " << npc->_id << endl;
					npc->_hp = npc->_max_hp;
					std::unique_lock<std::shared_mutex> lock(npc->_s_lock);
					npc->_state = ST_INGAME;

				}
				npc->respawn_NPC();
			}
			else if (key < MAX_USER)
			{
				Player* player = reinterpret_cast<Player*>(objects[key]);

				player->respawn_player();
			}
			break;
		}
		case OP_NPC_DEFENCE:
		{
			NPC* npc = reinterpret_cast<NPC*>(objects[key]);
			npc->_lua_lock.lock();
			lua_State* L = npc->_L;
			if (L != nullptr)
			{
				lua_getglobal(L, "event_object_Defence");
				lua_pushnumber(L, ex_over->_causeId);
				int status = lua_pcall(L, 1, 0, 0);
			}
			npc->_lua_lock.unlock();
			delete ex_over;
			break;
		}
		case OP_NPC_HEAL:
		{
			
			if (objects[key]->_hp <= 0)
			{
				break;
			}
			objects[key]->_hp += objects[key]->_max_hp / 10;
			if (objects[key]->_hp >= objects[key]->_max_hp)
			{
				objects[key]->_hp = objects[key]->_max_hp;
				for (auto& p_id : objects[key]->_view_list) {
					if (p_id >= MAX_USER)
						continue;
					Player* player = reinterpret_cast<Player*>(objects[p_id]);
					player->send_change_hp(objects[key]->_id);
				}
				break;
			}
			for (auto& p_id : objects[key]->_view_list) {
				if (p_id >= MAX_USER)
					continue;
				Player* player = reinterpret_cast<Player*>(objects[p_id]);
				player->send_change_hp(objects[key]->_id);
			}
			EVENT ev{ objects[key]->_id, EV_HEAL, chrono::system_clock::now() + 5s };
			//l_q.lock();
			timer_queue.push(ev);
			break;
		}
		case OP_NPC_ATTACK:
		{
			NPC* npc = reinterpret_cast<NPC*>(objects[key]);
			{
				npc->_lua_lock.lock();
				lua_State* L = npc->_L;
				for(auto p_id : npc->_view_list)
				{
					if (p_id >= MAX_USER) 
						break;
					if (L != nullptr)
					{
						lua_getglobal(L, "event_object_Attack");
						lua_pushnumber(L, p_id);
						int status = lua_pcall(L, 1, 0, 0);
					}
				}
				npc->_lua_lock.unlock();
			}
			EVENT ev{ objects[key]->_id, EV_MOVE, chrono::system_clock::now() + 1s };
			//l_q.lock();
			timer_queue.push(ev);
			delete ex_over;
			break;
		}
		case OP_NPC_RANGEATTACK:
		{
			NPC* npc = reinterpret_cast<NPC*>(objects[key]);
			{
				npc->_lua_lock.lock();
				lua_State* L = npc->_L;
				for (auto p_id : npc->_view_list)
				{
					if (p_id >= MAX_USER)
						break;
					if (L != nullptr)
					{
						
						pair<short, short> attack_range;
						while (npc->attack_range.try_pop(attack_range))
						{
							lua_getglobal(L, "event_range_Attack");
							lua_pushnumber(L, p_id);
							lua_pushnumber(L, attack_range.first);
							lua_pushnumber(L, attack_range.second);
							int status = lua_pcall(L, 3, 0, 0);
						}
					}
				}
				npc->_lua_lock.unlock();
			}
			ex_over->do_npc_wait(key);
			delete ex_over;
			break;
		}
		case OP_NPC_MOVE:
		{
			ex_over->do_npc_move(static_cast<int>(key));

			delete ex_over;
			break;
		}
		case OP_NPC_WAIT:
		{
			ex_over->do_npc_wait(static_cast<int>(key));

			delete ex_over;
			break;
		}
		case DB_RECV:
		{
			int remain_data = num_bytes + DB_prev_size;
			short* p = reinterpret_cast<short*>(ex_over->_buf);
			while (remain_data > 0) {
				int packet_size = p[0];
				if (packet_size <= remain_data) {
					ex_over->processpacket(static_cast<int>(key), p);
					p = p + packet_size;
					remain_data = remain_data - packet_size;
				}
				else break;
			}

			DB_prev_size = remain_data;
			if (remain_data > 0) {
				memcpy(ex_over->_buf, p, remain_data);
			}
			DB_do_recv();
			break;
		}
		case DB_SEND:
		{
			delete ex_over;
			break;
		}
		}
	}
}
