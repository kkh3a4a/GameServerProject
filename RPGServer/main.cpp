#include <iostream>
#include "NetWork.h"
#include"WorkThread.h"
#include"Player.h"
#include"Zone.h"
#include"DefaultNPC.h"
#include <concurrent_unordered_set.h>

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")
using namespace std;




WSA_OVER_EX g_a_over;
void initObject()
{
	for (int i = 0; i < MAX_USER; ++i)
	{
		objects[i] = new Player(i , ST_FREE);
	}
	for (int i = MAX_USER; i < MAX_USER + MAX_NPC; ++i)
	{
		objects[i] = new NPC;
	}
	cout << "init Object" << endl;
}

void initZone()
{
	for (int x = 0; x < ZONE_Y; ++x)
	{
		for(int y=0; y<ZONE_X;++y)
		zone[y][x] = new ZoneManager;
	}
}

void initialize_npc()
{
	for (int i = 0; i < MAX_NPC; ++i)
	{
		int npc_id = i + MAX_USER;
		NPC* npc = reinterpret_cast<NPC*>(objects[npc_id]);

		npc->_x = rand() % W_WIDTH;
		npc->_y = rand() % W_HEIGHT;
		npc->_x = 49 + i;
		npc->_y = 49 + i;
		npc->_state = ST_INGAME;
		npc->_id = npc_id;
		npc->_n_wake = 0;
		sprintf_s(npc->_name, "N%d", npc_id);
		int my_zoneY, my_zoneX;

		my_zoneY = npc->_y / ZONE_SEC;
		my_zoneX = npc->_x / ZONE_SEC;
		zone[my_zoneY][my_zoneX]->ADD(npc->_id);
	}
	cout << "NPC_initialize success" << endl;
}

int main() {
	initObject();
	initZone();
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);
	g_s_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT_NUM);
	server_addr.sin_addr.S_un.S_addr = INADDR_ANY;
	bind(g_s_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
	listen(g_s_socket, SOMAXCONN);
	SOCKADDR_IN cl_addr;
	int addr_size = sizeof(cl_addr);
	h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_s_socket), h_iocp, 999999, 0);
	g_c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	g_a_over._iocpop = OP_ACCEPT;
	AcceptEx(g_s_socket, g_c_socket, g_a_over._buf, 0, addr_size + 16, addr_size + 16, 0, &g_a_over._wsaover);
	
	initialize_npc();
	thread timer_thread{ TimerThread };
	vector <thread> worker_threads;
	int num_threads = std::thread::hardware_concurrency();
	for (int i = 0; i < num_threads; ++i)
		worker_threads.emplace_back(worker_thread, g_a_over);
	for (auto& th : worker_threads)
		th.join();
	
	timer_thread.join();
	closesocket(g_s_socket);
	WSACleanup();
}

