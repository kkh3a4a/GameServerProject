#pragma once
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <array>
#include<queue>
#include<mutex>
#include<iostream>
#include<set>
#include "../protocol.h"
#include"Object.h"
//#include"Zone.h"

using namespace std;

extern std::array<class Object*, MAXOBJECT> objects;
extern HANDLE h_iocp;
bool CAS(volatile int* addr, int expected, int update);
extern SOCKET g_s_socket, g_c_socket;
enum S_STATE { ST_FREE, ST_ALLOC, ST_INGAME };
constexpr int VIEW_RANGE = 6;
extern  std::array <std::array<class ZoneManager*, ZONE_Y>, ZONE_X> zone;

enum IOCPOP
{
	OP_RECV,
	OP_SEND,
	OP_ACCEPT,
	OP_NPC_AI
};
class WSA_OVER_EX {
public:
	WSAOVERLAPPED	_wsaover;
	IOCPOP			_iocpop;
	WSABUF			_wsabuf;
	char	_buf[BUF_SIZE];

public:
	WSA_OVER_EX();
	WSA_OVER_EX(IOCPOP iocpop, unsigned char byte, void* buf);

	void processpacket(int client_id, char* packet);
	void disconnect(int o_id);
	void do_npc_ai(int o_id);
	void set_accept_over();
};



int get_new_player_id();


bool can_see(int p1, int p2);

bool is_NPC(int _id);