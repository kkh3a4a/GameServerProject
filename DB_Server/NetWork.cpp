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

SQLCHAR szName[DB_NAME_SIZE];
SQLINTEGER szId, szExp;
SQLLEN cbName = 0, cbID = 0, cbExp = 0;
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
	case SD_PLAYER_CHANGE_STAT:
	{
		SD_PLAYER_CHANGE_STAT_PACKET* packet = reinterpret_cast<SD_PLAYER_CHANGE_STAT_PACKET*>(pk);

		player_change_state(w_id, packet->id, packet->exp, packet->level, packet->hp, packet->max_hp);
		break;
	}
	case SD_PLAYER_LOCATION:
	{
		SD_PLAYER_LOCATION_PACKET* packet = reinterpret_cast<SD_PLAYER_LOCATION_PACKET*>(pk);

		player_change_location(w_id, packet->id, packet->x, packet->y);


		break;
	}
	case SD_CHAT:
	{
		SD_CHAT_PACKET* packet = reinterpret_cast<SD_CHAT_PACKET*>(pk);
		player_chat_log(w_id, packet->id, packet->time, packet->mess);
		break;
	}
	default:
	{
		cout << "�߸��� packet" << endl;
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
	std::wcout << L"���� " << lpMsgBuf << std::endl;
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

	//id �ִ��� Ȯ��
	{
		SQLWCHAR query[100];
		SQLINTEGER inputId = id;
		swprintf(query, 100, L"SELECT user_id FROM user_info WHERE user_id = %d", inputId);
		retcode = SQLExecDirect(hstmt[w_id], query, SQL_NTS);
	}
	if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
		show_DB_error(w_id);
	}
	SQLLEN rowCount = 0;
	while (SQLFetch(hstmt[w_id]) == SQL_SUCCESS) {
		rowCount++; // ������ �� �ึ�� rowCount ����
	}
	if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
		show_DB_error(w_id);
	}
	SQLFreeHandle(SQL_HANDLE_STMT, hstmt[w_id]);
	///////////////////////////////////////////////////

	//id ������ �߰� ����
	if (rowCount <= 0)
	{
		cout << id << " ����" << endl;
		retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc[w_id], &hstmt[w_id]);
		if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
			show_DB_error(w_id);
		}
		SQLWCHAR query[100];
		wchar_t w_name[20];
		swprintf(w_name, 20, L"%hs", name);
		swprintf(query, 100, L"EXEC Add_User %d, '%hs'", id, name);
		retcode = SQLExecDirect(hstmt[w_id], query, SQL_NTS);
		if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
			show_DB_error(w_id);
		}
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt[w_id]);
	}


	//////////////////////////////////////

	retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc[w_id], &hstmt[w_id]);
	if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
	{
		show_DB_error(w_id);
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
			show_DB_error(w_id);
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
		retcode = SQLFetch(hstmt[w_id]);  // ������ �ؼ�
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
			cout << "login ���� ����" << endl;
		}
		else
		{
			show_DB_error(w_id);
		}

		SQLFreeHandle(SQL_HANDLE_STMT, hstmt[w_id]);
	}


	
}



void show_DB_error(int w_id) {
	std::wcout.imbue(std::locale("korean"));
	SQLWCHAR sqlState[6];
	SQLINTEGER nativeErr;
	SQLWCHAR errMsg[SQL_MAX_MESSAGE_LENGTH / sizeof(SQLWCHAR)];
	SQLSMALLINT msgLen;

	SQLGetDiagRec(SQL_HANDLE_STMT, hstmt[w_id], 1, sqlState, &nativeErr, (errMsg), SQL_MAX_MESSAGE_LENGTH / sizeof(SQLWCHAR), &msgLen);
	wcout << (L"SQL error : %ls\n", errMsg);
	wcout << endl;

	SQLDisconnect(hdbc);
	SQLFreeHandle(SQL_HANDLE_STMT, hstmt[w_id]);
	SQLFreeHandle(SQL_HANDLE_DBC, hdbc[w_id]);

	DB_connect(w_id);
}

void player_change_location(int w_id, int id, int x, int y)
{
	auto retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc[w_id], &hstmt[w_id]);

	{
		SQLWCHAR query[100];
		swprintf(query, 100, L"EXEC Location_User %d, %d, %d", id, x, y);
		retcode = SQLExecDirect(hstmt[w_id], query, SQL_NTS);
		if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
		{
			show_DB_error(w_id);
		}
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt[w_id]);
	}
}

void player_chat_log(int w_id, int id, char* time, char* mess)
{
	int length = std::strlen(mess);
	string msg(mess, length);
	msg += '\0';
	auto retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc[w_id], &hstmt[w_id]);

	{
		SQLWCHAR query[100];
		swprintf(query, 100, L"EXEC Chating_Logs %d, '%hs', '%hs'", id, time, msg.c_str());
		int retcode = SQLExecDirect(hstmt[w_id], query, SQL_NTS);
		if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
		{
			show_DB_error(w_id);
		}
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt[w_id]);
	}
}

void player_change_state(int w_id, int id, int exp, int level, int hp, int max_hp)
{
	auto retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc[w_id], &hstmt[w_id]);
	
	{
		SQLWCHAR query[100];
    		swprintf(query, 100, L"EXEC STAT_User %d, %d, %d, %d, %d", id, exp, level, hp, max_hp);
		int retcode = SQLExecDirect(hstmt[w_id], query, SQL_NTS);
		if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
		{
			show_DB_error(w_id);
		}
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt[w_id]);
	}
}

void DB_connect(int w_id)
{

	auto retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv[w_id]);
	bool connect_check = true;
	// Set the ODBC version environment attribute  
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv[w_id], SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);

		// Allocate connection handle  
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv[w_id], &hdbc[w_id]);

			// Set login timeout to 5 seconds  
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retry:
				SQLSetConnectAttr(hdbc[w_id], SQL_LOGIN_TIMEOUT, (SQLPOINTER)10, 0);

				// Connect to data source  
			   //SQLConnect(hdbc, (SQLWCHAR*)L"DB_Master", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);
				retcode = SQLConnect(hdbc[w_id], (SQLWCHAR*)L"DB_GameServerProject", SQL_NTS, (SQLWCHAR*)L"2019180046", SQL_NTS, (SQLWCHAR*)L"2019180046", SQL_NTS);
				//retcode = SQLConnect(hdbc[w_id], (SQLWCHAR*)L"2023TT", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);
				// Allocate statement handle  
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc[w_id], &hstmt[w_id]);

					SQLWCHAR query[100];
					SQLINTEGER inputId = 999999; // ����ڰ� �Է��� ID ��
					// �������� SQL ���� ����
					swprintf(query, 100, L"SELECT user_id, user_name FROM user_info WHERE user_id = %d", inputId);
					// SQL ���� ����
					retcode = SQLExecDirect(hstmt[w_id], query, SQL_NTS);
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						// Bind columns 1, 2
						retcode = SQLBindCol(hstmt[w_id], 1, SQL_INTEGER, &szId, sizeof(SQLINTEGER), &cbID);
						retcode = SQLBindCol(hstmt[w_id], 2, SQL_C_CHAR, szName, DB_NAME_SIZE, &cbName);

						// Fetch and print each row of data. On an error, display a message and exit. 
						retcode = SQLFetch(hstmt[w_id]);  // ������ �ؼ�
						if (retcode == SQL_ERROR)
							std::cout << "Fetch error" << endl;
						if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
						{
							wcout << w_id << L" : " << reinterpret_cast<char*>(szName) << endl;
						}
					}
					else
					{
						connect_check = false;
					}

					// Process data  
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						SQLFreeHandle(SQL_HANDLE_STMT, hstmt[w_id]);
					}

				}
				else
				{
					goto retry;
				}

			}
		}
	}

	if (!connect_check)
		show_DB_error(w_id);
}