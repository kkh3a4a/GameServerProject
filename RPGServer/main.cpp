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

		npc->_x = rand() % W_WIDTH;
		npc->_y = rand() % W_HEIGHT;
		/*npc->_x = 49 + i;
		npc->_y = 49 + i;*/
		npc->_state = ST_INGAME;
		npc->_id = npc_id;
		npc->_n_wake = 0;
		//npc->_last_hello_time = chrono::system_clock::now();
		sprintf_s(npc->_name, "N%d", npc_id);
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

		lua_register(L, "API_SendMessage", API_SendMessage);
		lua_register(L, "API_get_x", API_get_x);
		lua_register(L, "API_get_y", API_get_y);
	}
	cout << "NPC_initialize success" << endl;
}

void connect_DB() {
	SQLRETURN retcode;
	SQLCHAR szName[NAME_SIZE];
	SQLINTEGER szId, szExp;
	SQLLEN cbName = 0, cbID = 0, cbExp = 0;



	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	// Set the ODBC version environment attribute  
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);

		// Allocate connection handle  
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

			// Set login timeout to 5 seconds  
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

				// Connect to data source  
			   //SQLConnect(hdbc, (SQLWCHAR*)L"DB_Master", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);
				retcode = SQLConnect(hdbc, (SQLWCHAR*)L"DB_GameServerProject", SQL_NTS, (SQLWCHAR*)L"2019180046", SQL_NTS, (SQLWCHAR*)L"2019180046", SQL_NTS);
				// Allocate statement handle  
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

					// DB SELECT 함수 실행 
					//retcode = SQLExecDirect(hstmt, (SQLWCHAR*)L"EXEC over_exp 30000", SQL_NTS);
					//retcode = SQLExecDirect(hstmt, (SQLWCHAR*)L"SELECT COUNT(*) FROM information_schema.tables WHERE table_name = 'user_info'", SQL_NTS);
					retcode = SQLExecDirect(hstmt, (SQLWCHAR*)L"SELECT user_id, user_name, user_exp FROM user_info", SQL_NTS);
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						// Bind columns 1, 2, and 3  
						retcode = SQLBindCol(hstmt, 1, SQL_INTEGER, &szId, 12, &cbID);
						retcode = SQLBindCol(hstmt, 2, SQL_C_CHAR, szName, NAME_SIZE, &cbName);
						retcode = SQLBindCol(hstmt, 3, SQL_INTEGER, &szExp, 12, &cbExp);

						// Fetch and print each row of data. On an error, display a message and exit. 
						cout << "test DB" << endl;
						for (int i = 0; ; i++) {
							retcode = SQLFetch(hstmt);  // 데이터 해석
							if (retcode == SQL_ERROR)
								cout << "Fetch error" << endl;
							if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
							{
								wcout << i + 1 << L" : " << szId << L" " << reinterpret_cast<char*>(szName) << L" " << szExp << endl;
							}
							else
								break;
						}
					}
					else
					{
						show_DB_error(hstmt);
					}

					// Process data  
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						SQLCancel(hstmt);
						SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
					}

					//SQLDisconnect(hdbc);
				}

				//SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
			}
		}
		//SQLFreeHandle(SQL_HANDLE_ENV, henv);
	}
	/*retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
	retcode = SQLExecDirect(hstmt, (SQLWCHAR*)L"SELECT user_id, user_exp FROM user_info", SQL_NTS);*/
	
	cout << "connect DB" << endl;
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

