#include "NetWork.h"


HANDLE h_iocp;
bool IsNight;

SOCKET DB_socket, SV_socket;
SQLHENV henv;
SQLHDBC hdbc;
SQLHSTMT hstmt;
SQLRETURN retcode;
WSA_OVER_EX _wsa_recv_over;
int _prev_size{};

//std::shared_lock<std::shared_mutex> lock(player->_s_lock);
//std::unique_lock<std::shared_mutex> lock(player->_s_lock);


WSA_OVER_EX::WSA_OVER_EX()
{
	ZeroMemory(&_wsaover, sizeof(_wsaover));
	_wsabuf.len = DB_BUF_SIZE;
	_wsabuf.buf = _buf;
	_iocpop = OP_RECV;
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
	case SD_PLAYER_LOGIN:
	{
		SD_PLAYER_LOGIN_PACKET* packet = reinterpret_cast<SD_PLAYER_LOGIN_PACKET*>(pk);
		cout << "LOGIN ID : " << packet->id << endl;
		SD_PLAYER_LOGIN_PACKET s_p;
		s_p.id = packet->id + 1;
		s_p.size = sizeof(s_p);
		s_p.type = DS_PLAYER_LOGIN;
		send_packet(&s_p);
		break;
	}
	default:
	{
		DebugBreak();
		break;
	}
	}
}


void WSA_OVER_EX::disconnect(int o_id)	
{
	
}
bool CAS(volatile int* addr, int expected, int update)
{
    return std::atomic_compare_exchange_strong(reinterpret_cast <volatile std::atomic_int*>(addr), &expected, update);
}


void do_recv()
{
	DWORD recv_flag = 0;
	memset(&_wsa_recv_over._wsaover, 0, sizeof(_wsa_recv_over._wsaover));
	_wsa_recv_over._wsabuf.len = DB_BUF_SIZE - _prev_size;
	_wsa_recv_over._wsabuf.buf = _wsa_recv_over._buf + _prev_size;
	_wsa_recv_over._iocpop = OP_RECV;
	int ret = WSARecv(SV_socket, &_wsa_recv_over._wsabuf, 1, 0, &recv_flag, &_wsa_recv_over._wsaover, 0);
	
}

void error_display(const char* msg, int err_no)
{
	WCHAR* lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	std::cout << msg;
	std::wcout << L"¿¡·¯ " << lpMsgBuf << std::endl;
	while (true);
	LocalFree(lpMsgBuf);
}

void send_packet(void* pk)
{
	char* buf = reinterpret_cast<char*>(pk);
	WSA_OVER_EX* _wsa_send_over = new WSA_OVER_EX(OP_SEND, buf[0], pk);

	int ret = WSASend(SV_socket, &_wsa_send_over->_wsabuf, 1, NULL, 0, &_wsa_send_over->_wsaover, NULL);
	if (ret != 0)
	{
		int errorcode = WSAGetLastError();
		error_display("do recv : ", errorcode);
	}
}
