#pragma once
#include <WS2tcpip.h>
#include <MSWSock.h>
#include<iostream>
#include<Windows.h>
#include<string>
#include "../DBprotocol.h"
#include <concurrent_priority_queue.h>
#include<thread>
#define UNICODE  
#include <sqlext.h>  
#include<map>

//#include"Zone.h"

using namespace std;
extern HANDLE h_iocp;
bool CAS(volatile int* addr, int expected, int update);
extern SOCKET DB_socket, G_socket;
extern SQLHENV henv[DB_THREAD_NUM];
extern SQLHDBC hdbc[DB_THREAD_NUM];
extern SQLHSTMT hstmt[DB_THREAD_NUM];
extern map<int, SOCKET> G_server;
extern int num_threads;
extern int _prev_size[8];
extern SQLCHAR szName[DB_NAME_SIZE];
extern SQLINTEGER szId, szExp;
extern SQLLEN cbName, cbID, cbExp;

enum IOCPOP
{
	OP_RECV,
	OP_SEND,
	OP_ACCEPT,

};
class WSA_OVER_EX {
public:
	WSAOVERLAPPED	_wsaover;
	IOCPOP			_iocpop;
	WSABUF			_wsabuf;
	int				_causeId;
	char			_buf[DB_BUF_SIZE]{};

public:
	WSA_OVER_EX();
	WSA_OVER_EX(IOCPOP iocpop, unsigned char byte, void* buf);
	void processpacket(int o_id, void* pk, int w_id);
	void disconnect(int o_id);
	
};
extern WSA_OVER_EX _wsa_recv_over;

void DB_connect(int w_id);
void do_recv(int key);
void send_packet(void*, int key);
void error_display(const char* msg, int err_no);
void player_login(int s_id, int id, char name[20], int key, int w_id);
void show_DB_error(int w_id);
void player_change_state(int w_id, int id, int exp, int level, int hp, int max_hp);
void player_change_location(int w_id, int id, int x, int y);
void player_chat_log(int w_id, int id, char* time, char* mess);
