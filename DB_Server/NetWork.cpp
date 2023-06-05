#include "NetWork.h"
#define _CRT_SECURE_NO_WARNINGS

HANDLE h_iocp;
bool IsNight;

SOCKET DB_socket, G_socket;
SQLHENV henv[DB_THREAD_NUM];
SQLHDBC hdbc[DB_THREAD_NUM];
SQLHSTMT hstmt[DB_THREAD_NUM];
WSA_OVER_EX _wsa_recv_over;
int _prev_size{};
map<int, SOCKET> G_server;
int num_threads;
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

void WSA_OVER_EX::processpacket(int o_id, void* pk, int w_id)
{
	char* packet_data = static_cast<char*>(pk);
	unsigned char packet_type = packet_data[2];

	switch (packet_type)
	{
	case SD_PLAYER_LOGIN:
	{
		SD_PLAYER_LOGIN_PACKET* packet = reinterpret_cast<SD_PLAYER_LOGIN_PACKET*>(pk);
		//cout << "LOGIN ID : " << packet->id << endl;

		player_login(packet->s_id, packet->id, packet->name, o_id, w_id);
		
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


void do_recv(int key)
{
	DWORD recv_flag = 0;
	memset(&_wsa_recv_over._wsaover, 0, sizeof(_wsa_recv_over._wsaover));
	_wsa_recv_over._wsabuf.len = DB_BUF_SIZE - _prev_size;
	_wsa_recv_over._wsabuf.buf = _wsa_recv_over._buf + _prev_size;
	_wsa_recv_over._iocpop = OP_RECV;
	int ret = WSARecv(G_server[key], &_wsa_recv_over._wsabuf, 1, 0, &recv_flag, &_wsa_recv_over._wsaover, 0);
	
	
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
	std::wcout << L"에러 " << lpMsgBuf << std::endl;
	while (true);
	LocalFree(lpMsgBuf);
}

void send_packet(void* pk, int key)
{
	char* buf = reinterpret_cast<char*>(pk);
	WSA_OVER_EX* _wsa_send_over = new WSA_OVER_EX(OP_SEND, buf[0], pk);

	int ret = WSASend(G_server[key], &_wsa_send_over->_wsabuf, 1, NULL, 0, &_wsa_send_over->_wsaover, NULL);
	
}

void player_login(int s_id, int id, char name[20], int key, int w_id)
{
	SQLINTEGER exp = 0;
	SQLINTEGER hp = 0;
	SQLINTEGER max_hp = 0;
	SQLINTEGER level = 0;
	SQLINTEGER x = 0;
	SQLINTEGER y = 0;
	SQLLEN cbChar = 0, cbInt = 0;
	auto retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc[w_id], &hstmt[w_id]);

	//id 있는지 확인
	{
		SQLWCHAR query[100];
		SQLINTEGER inputId = id;
		swprintf(query, 100, L"SELECT user_id FROM user_info WHERE user_id = %d", inputId);
		retcode = SQLExecDirect(hstmt[w_id], query, SQL_NTS);
	}
	if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
		show_DB_error(hstmt[w_id]);
	}
	SQLLEN rowCount = 0;
	while (SQLFetch(hstmt[w_id]) == SQL_SUCCESS) {
		rowCount++; // 가져온 각 행마다 rowCount 증가
	}
	if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
		show_DB_error(hstmt[w_id]);
	}
	SQLFreeHandle(SQL_HANDLE_STMT, hstmt[w_id]);
	///////////////////////////////////////////////////

	//id 없으면 추가 해줌
	if (rowCount <= 0)
	{
		cout << id << " 생성" << endl;
		retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc[w_id], &hstmt[w_id]);
		if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
			show_DB_error(hstmt[w_id]);
		}
		SQLWCHAR query[100];
		wchar_t w_name[20];
		swprintf(w_name, 20, L"%hs", name);
		swprintf(query, 100, L"EXEC Add_User %d, '%hs'", id, name);
		retcode = SQLExecDirect(hstmt[w_id], query, SQL_NTS);
		if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
			show_DB_error(hstmt[w_id]);
		}
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt[w_id]);
	}


	//////////////////////////////////////

	retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc[w_id], &hstmt[w_id]);
	if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
	{
		show_DB_error(hstmt[w_id]);
	}
	{
		SQLWCHAR query[200];
		//swprintf(query, 100, L"SELECT user_hp, user_max_hp, user_exp, user_level, user_x, user_y FROM user_info WHERE user_id = %d", id);
		SQLINTEGER inputId = id;
		swprintf(query, 200, L"SELECT user_hp, user_max_hp, user_exp, user_level, user_x, user_y FROM user_info WHERE user_id = %d", inputId);
		//swprintf(query, 100, L"SELECT user_hp, user_max_hp, user_exp, user_level, user_x, user_y FROM user_info WHERE user_id = %d", inputId);
		retcode = SQLExecDirect(hstmt[w_id], query, SQL_NTS);
		if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
		{
			show_DB_error(hstmt[w_id]);
		}
	}
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		// Bind columns 1, 2, and 3  
		retcode = SQLBindCol(hstmt[w_id], 1, SQL_INTEGER, &hp, 4, &cbInt);
		retcode = SQLBindCol(hstmt[w_id], 2, SQL_INTEGER, &max_hp, 4, &cbInt);
		retcode = SQLBindCol(hstmt[w_id], 3, SQL_INTEGER, &exp, 4, &cbInt);
		retcode = SQLBindCol(hstmt[w_id], 4, SQL_INTEGER, &level, 4, &cbInt);
		retcode = SQLBindCol(hstmt[w_id], 5, SQL_INTEGER, &x, 4, &cbInt);
		retcode = SQLBindCol(hstmt[w_id], 6, SQL_INTEGER, &y, 4, &cbInt);
		// Fetch and print each row of data. On an error, display a message and exit. 
		retcode = SQLFetch(hstmt[w_id]);  // 데이터 해석
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
		{
			DS_PLAYER_LOGIN_PACKET s_p;
			s_p.id = id;
			s_p.s_id = s_id;
			s_p.exp = exp;
			s_p.hp = hp;
			s_p.max_hp = max_hp;
			s_p.level = level;
			s_p.x = x;
			s_p.y = y;

			s_p.type = DS_PLAYER_LOGIN;
			s_p.size = sizeof(DS_PLAYER_LOGIN_PACKET);

			cout << "login : " << s_p.id << endl;
			send_packet(&s_p, key);
		}
		else if (retcode == SQL_NO_DATA)
		{
			cout << "login 정보 없음" << endl;
		}
		else
		{
			show_DB_error(hstmt[w_id]);
		}

		SQLFreeHandle(SQL_HANDLE_STMT, hstmt[w_id]);
	}


	
}



void show_DB_error(SQLHSTMT hstmt) {
	std::wcout.imbue(std::locale("korean"));
	SQLWCHAR sqlState[6];
	SQLINTEGER nativeErr;
	SQLWCHAR errMsg[SQL_MAX_MESSAGE_LENGTH / sizeof(SQLWCHAR)];
	SQLSMALLINT msgLen;

	SQLGetDiagRec(SQL_HANDLE_STMT, hstmt, 1, sqlState, &nativeErr, (errMsg), SQL_MAX_MESSAGE_LENGTH / sizeof(SQLWCHAR), &msgLen);
	wcout << (L"SQL error : %ls\n", errMsg);
	wcout << endl;
}
