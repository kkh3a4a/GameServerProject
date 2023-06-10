#pragma once
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <array>
#include<queue>
#include<mutex>
#include<iostream>
#include<set>
#include "../protocol_2023.h"
#include "../DBprotocol.h"
#include"Object.h"
#include"TimerThread.h"
#include <concurrent_priority_queue.h>
#include"include/lua.hpp"
#include<string>
#include<map>

#pragma comment(lib, "lua54.lib")
//#include"Zone.h"

using namespace std;

extern std::array<class Object*, MAXOBJECT> objects;
extern HANDLE h_iocp;
bool CAS(volatile int* addr, int expected, int update);
extern SOCKET g_s_socket, g_c_socket;
extern concurrency::concurrent_priority_queue <class EVENT> timer_queue;
extern int DB_prev_size;
/////DB/////
extern SOCKET DB_socket;
////////////
extern std::map<std::pair<short, short>, short> World_Map;
enum EVENT_TYPE { EV_RANDOM_MOVE, EV_RESPAWN, EV_ATTACK, EV_DEFENCE, EV_HEAL, EV_MOVE};

constexpr int VIEW_RANGE = 6;
extern  std::array <std::array<class ZoneManager*, ZONE_Y>, ZONE_X> zone;


enum IOCPOP
{
	OP_RECV,
	OP_SEND,
	OP_ACCEPT,
	OP_NPC_RANDOMMOVE,
	OP_NPC_MOVE,
	OP_NPC_RESPAWN,
	OP_NPC_HEAL,
	OP_NPC_ATTACK,
	OP_NPC_DEFENCE,
	OP_AI_HELLO,
	OP_AI_BYE,
	
	DB_RECV,
	DB_SEND,

	
};
class WSA_OVER_EX {
public:
	WSAOVERLAPPED	_wsaover;
	IOCPOP			_iocpop;
	WSABUF			_wsabuf;
	EVENT_TYPE		_e_type;
	int				_causeId;
	char			_buf[BUF_SIZE]{};

public:
	WSA_OVER_EX();
	WSA_OVER_EX(IOCPOP iocpop, unsigned short byte, void* buf);

	void processpacket(int client_id, void* packet);
	void disconnect(int o_id);

	void wake_up_npc(int n_id);
	void do_npc_ramdom_move(int o_id);
	void set_accept_over();
	void do_npc_move(int n_id);
	
};
extern WSA_OVER_EX DB_wsa_recv_over;



int get_new_player_id();

bool can_see(int p1, int p2);

bool is_NPC(int _id);
/// <summary> LUA Script
int API_get_x(lua_State* L);

int API_get_y(lua_State* L);

int API_SendMessage(lua_State* L);

int API_Defence(lua_State* L);

int API_Attack(lua_State* L);
///////////////////////////////////////////////////////
void zone_check(int x, int y, set<int>&);
class EVENT
{
public:
	int _o_id;
	EVENT_TYPE _e_type;
	chrono::system_clock::time_point _exec_time;;

	EVENT();
	EVENT(int id, EVENT_TYPE e_type, chrono::system_clock::time_point timer) : _o_id(id), _e_type(e_type), _exec_time(timer) {}
	constexpr bool operator <(const EVENT& _Left) const
	{
		return (_exec_time > _Left._exec_time);
	}

};

/////////////////////////DB
void DB_send_packet(void*);
void DB_player_login(int id, char name[20], int s_id);
void DB_do_recv();

