#pragma once
#include <WS2tcpip.h>
#include <MSWSock.h>
#include<iostream>
#include<Windows.h>
#include<string>
#include "../protocol.h"
#include <concurrent_priority_queue.h>
#include<thread>
#define UNICODE  
#include <sqlext.h>  
//#include"Zone.h"

using namespace std;
extern HANDLE h_iocp;
bool CAS(volatile int* addr, int expected, int update);
extern SOCKET g_s_socket, g_c_socket;
extern SQLHENV henv;
extern SQLHDBC hdbc;
extern SQLHSTMT hstmt;
extern SQLRETURN retcode;

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
	char			_buf[BUF_SIZE]{};

public:
	WSA_OVER_EX();
	WSA_OVER_EX(IOCPOP iocpop, unsigned char byte, void* buf);
	void processpacket(int o_id, char* pk);
	void disconnect(int o_id);
};

void error_display(const char* msg, int err_no);