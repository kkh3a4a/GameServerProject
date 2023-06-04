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
		case OP_AI_HELLO:
		{
			NPC* npc = reinterpret_cast<NPC*>(objects[key]);
			npc->_lua_lock.lock();
			lua_State* L = npc->_L;
			if(L != nullptr)
			{
				lua_getglobal(L, "event_player_move");
				lua_pushnumber(L, ex_over->_causeId);
				int status = lua_pcall(L, 1, 0, 0);
				

			}
			npc->_lua_lock.unlock();

			delete ex_over;
			
			break;
		}
		case OP_AI_BYE:
		{
			delete ex_over;
			break;
		}
		case OP_AI_DEFENCE:
		{
			NPC* npc = reinterpret_cast<NPC*>(objects[key]);
			npc->_lua_lock.lock();
			lua_State* L = npc->_L;
			if (L != nullptr)
			{
				lua_getglobal(L, "event_player_Attack");
				lua_pushnumber(L, ex_over->_causeId);
				int status = lua_pcall(L, 1, 0, 0);


			}
			npc->_lua_lock.unlock();
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
