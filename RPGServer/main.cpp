#include <iostream>
#include "NetWork.h"
#include"WorkThread.h"
#include"Player.h"
#include"Zone.h"
#include"DefaultNPC.h"
#include <concurrent_unordered_set.h>
#include<fstream>
#include<random>
#include <sstream>
#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")
using namespace std;

#define UNICODE  

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
		
		relocation:
		npc->_spawn_x = npc->_x = rand() % W_WIDTH;
		npc->_spawn_y = npc->_y = rand() % W_HEIGHT;
		if (World_Map.find(std::make_pair(npc->_x, npc->_y)) != World_Map.end())
			goto relocation;
		/*npc->_x = 49 + i;
		npc->_y = 49 + i;*/
		npc->_state = ST_INGAME;
		npc->_id = npc_id;
		npc->_n_wake = 0;
		npc->_n_type = 1;
		//npc->_last_hello_time = chrono::system_clock::now();
		
		int my_zoneY, my_zoneX;

		my_zoneY = npc->_y / ZONE_SEC;
		my_zoneX = npc->_x / ZONE_SEC;
		zone[my_zoneY][my_zoneX]->ADD(npc->_id);

		auto L = npc->_L = luaL_newstate();
		luaL_openlibs(L);
		int ret = luaL_loadfile(L, "npc.lua");
		lua_pcall(L, 0, 0, 0);

		lua_getglobal(L, "set_uid");
		lua_pushnumber(L, npc_id);
		lua_pcall(L, 1, 0, 0);
		lua_pop(L, 1);// eliminate set_uid from stack after call

		int t = rand() % 4 + 1;
		if(t == 1)
			sprintf_s(npc->_name, "Q%d", npc_id);
		if (t == 2)
			sprintf_s(npc->_name, "W%d", npc_id);
		if (t == 3)
			sprintf_s(npc->_name, "E%d", npc_id);
		if (t == 4)
		sprintf_s(npc->_name, "R%d", npc_id);
			npc->_n_type = t;
		npc->_level = npc->_n_type;
		npc->_max_hp = npc->_hp = 100 + (npc->_level * npc->_level * 10);
		npc->agro = t / 2;
		lua_getglobal(L, "set_agro");
		lua_pushnumber(L, npc->agro);
		lua_pcall(L, 1, 0, 0);
		lua_pop(L, 1);// eliminate set_uid from stack after call

		lua_getglobal(L, "set_type");
		lua_pushnumber(L, t);
		lua_pcall(L, 1, 0, 0);
		lua_pop(L, 1);// eliminate set_uid from stack after call

		lua_register(L, "API_SendMessage", API_SendMessage);
		lua_register(L, "API_get_x", API_get_x);
		lua_register(L, "API_get_y", API_get_y);
		lua_register(L, "API_Defence", API_Defence);
		lua_register(L, "API_Default_Attack", API_Default_Attack);
		lua_register(L, "API_Range_Attack", API_Range_Attack);
	}
	cout << "NPC_initialize success" << endl;
}

void connect_DB() {
	DB_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN DBserver_addr;
	ZeroMemory(&DBserver_addr, sizeof(DBserver_addr));
	DBserver_addr.sin_family = AF_INET;
	DBserver_addr.sin_port = htons(DB_PORT_NUM);
	inet_pton(AF_INET, DB_SERVER_ADDR, &DBserver_addr.sin_addr);
	int ret = connect(DB_socket, reinterpret_cast<sockaddr*>(&DBserver_addr), sizeof(DBserver_addr));

	if (ret != 0)
	{
		int errorcode = WSAGetLastError();
		cout << "DBconnect Errorcode : " << errorcode << endl;;
	}
	cout << "DBconnect" << endl;
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(DB_socket), h_iocp, 1234567, 0);
	DB_do_recv();
	
}

void createMap() {
	std::ofstream file("../World_Map.txt", std::ios::app); // 파일 열기 (이어쓰기 모드)

	if (file.is_open()) {
		std::random_device rd;
		std::mt19937 rng(rd());
		std::uniform_int_distribution<int> dist(1, 1999);

		std::map<std::pair<int, int>,int> uniquePairs;

		while (uniquePairs.size() < 60000) {
			if(rand() % 8 < 4)
			{
				int num1 = dist(rng);
				int num2 = dist(rng);
				int type = 1;
				uniquePairs[make_pair(num1, num2)] = type;
			}
			else if(rand() % 8 == 4)
			{
				int num1 = dist(rng);
				int num2 = dist(rng);
				int type = 2;
				for(int i = 0; i< 3;++i)
					uniquePairs[make_pair(num1 + i, num2)] = type;				
			}
			else if (rand() % 8 == 5)
			{
				int num1 = dist(rng);
				int num2 = dist(rng);
				int type = 3;
				for (int i = 0; i < 3; ++i)
					uniquePairs[make_pair(num1, num2 + i)] = type;
			}
			else if (rand() % 8 == 6)
			{
				int num1 = dist(rng);
				int num2 = dist(rng);
				int type = 4;
				for (int i = 0; i < 5; ++i)
					uniquePairs[make_pair(num1 + i, num2)] = type;
			}
			else if (rand() % 8 == 7)
			{
				int num1 = dist(rng);
				int num2 = dist(rng);
				int type = 4;
				for (int i = 0; i < 5; ++i)
					uniquePairs[make_pair(num1, num2 + i)] = type;
			}
		}

		for (const auto& maps : uniquePairs) {
			file << maps.first.first << " " << maps.first.second << " " << maps.second <<std::endl;
		}

		std::cout << "중복이 없는 랜덤한 숫자 pair를 파일에 저장하고 파일을 닫았습니다." << std::endl;
		file.close(); // 파일 닫기
	}
	else {
		std::cout << "파일을 열지 못했습니다." << std::endl;
	}
}
void ReadMap() {
	
	std::ifstream file("../World_Map.txt");
	std::string line;
	while (std::getline(file, line)) {
		std::istringstream iss(line);
		short x, y, type;
		if (iss >> x >> y >> type) {
			World_Map[std::make_pair(x, y)] = type;
		}
	}
	file.close();
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
	connect_DB();
	//createMap();
	ReadMap();
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

