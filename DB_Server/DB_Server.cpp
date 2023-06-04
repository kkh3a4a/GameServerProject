#include "NetWork.h"
#include"WorkThread.h"

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")



using namespace std;


SQLCHAR szName[DB_NAME_SIZE];
SQLINTEGER szId, szExp;
SQLLEN cbName = 0, cbID = 0, cbExp = 0;
WSA_OVER_EX g_a_over;
void show_DB_error(SQLHSTMT hstmt);
void DB_connect();

void DB_connect()
{
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
						retcode = SQLBindCol(hstmt, 2, SQL_C_CHAR, szName, DB_NAME_SIZE, &cbName);
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


int main()
{
	std::wcout.imbue(std::locale("korean"));
	DB_connect();

	WSADATA WSAData;
	int ret = WSAStartup(MAKEWORD(2, 2), &WSAData);
	if (ret != 0)
	{
		int errorcode = WSAGetLastError();
		error_display("WSAStartup : ", errorcode);
	}
	DB_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(DB_PORT_NUM);
	server_addr.sin_addr.S_un.S_addr = INADDR_ANY;
	ret = bind(DB_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
	if (ret != 0)
	{
		int errorcode = WSAGetLastError();
		error_display("WSAStartup : ", errorcode);
	}
	ret = listen(DB_socket, SOMAXCONN);
	if (ret != 0)
	{
		int errorcode = WSAGetLastError();
		error_display("WSAStartup : ", errorcode);
	}
	SOCKADDR_IN cl_addr;
	int addr_size = sizeof(cl_addr);
	h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(DB_socket), h_iocp, 999999, 0);
	G_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	g_a_over._iocpop = OP_ACCEPT;
	ret = AcceptEx(DB_socket, G_socket, g_a_over._buf, 0, addr_size + 16, addr_size + 16, 0, &g_a_over._wsaover);
	if (ret != 0)
	{
		int errorcode = WSAGetLastError();
		error_display("WSAStartup : ", errorcode);
	}
	vector <thread> worker_threads;
	int num_threads = std::thread::hardware_concurrency();
	for (int i = 0; i < num_threads; ++i)
		worker_threads.emplace_back(worker_thread, g_a_over);

	for (auto& th : worker_threads)
		th.join();
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