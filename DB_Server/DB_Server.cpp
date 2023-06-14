#include "NetWork.h"
#include"WorkThread.h"

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")



using namespace std;


SQLCHAR szName[DB_NAME_SIZE];
SQLINTEGER szId, szExp;
SQLLEN cbName = 0, cbID = 0, cbExp = 0;
WSA_OVER_EX g_a_over;

void DB_connect(int w_id);

void DB_connect(int w_id)
{

	auto retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv[w_id]);

	// Set the ODBC version environment attribute  
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv[w_id], SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);

		// Allocate connection handle  
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv[w_id], &hdbc[w_id]);

			// Set login timeout to 5 seconds  
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(hdbc[w_id], SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

				// Connect to data source  
			   //SQLConnect(hdbc, (SQLWCHAR*)L"DB_Master", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);
				retcode = SQLConnect(hdbc[w_id], (SQLWCHAR*)L"DB_GameServerProject", SQL_NTS, (SQLWCHAR*)L"2019180046", SQL_NTS, (SQLWCHAR*)L"2019180046", SQL_NTS);
				//retcode = SQLConnect(hdbc, (SQLWCHAR*)L"2023TT", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);
				// Allocate statement handle  
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc[w_id], &hstmt[w_id]);

					SQLWCHAR query[100];
					SQLINTEGER inputId = 999999; // 사용자가 입력한 ID 값
					// 동적으로 SQL 문장 생성
					swprintf(query, 100, L"SELECT user_id, user_name FROM user_info WHERE user_id = %d", inputId);
					// SQL 문장 실행
					retcode = SQLExecDirect(hstmt[w_id], query, SQL_NTS);
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						// Bind columns 1, 2
						retcode = SQLBindCol(hstmt[w_id], 1, SQL_INTEGER, &szId, sizeof(SQLINTEGER), &cbID);
						retcode = SQLBindCol(hstmt[w_id], 2, SQL_C_CHAR, szName, DB_NAME_SIZE, &cbName);

						// Fetch and print each row of data. On an error, display a message and exit. 
						retcode = SQLFetch(hstmt[w_id]);  // 데이터 해석
						if (retcode == SQL_ERROR)
							std::cout << "Fetch error" << endl;
						if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
						{
							wcout << w_id << L" : " << reinterpret_cast<char*>(szName) << endl;
						}
					}
					else
					{
						show_DB_error(hstmt[w_id]);
					}

					// Process data  
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						SQLFreeHandle(SQL_HANDLE_STMT, hstmt[w_id]);
					}

				}

			}
		}
	}
}

void player_login()
{
}


int main()
{
	std::wcout.imbue(std::locale("korean"));
	

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

	std::cout << "test DB" << endl;
	for (int i = 0; i < DB_THREAD_NUM; ++i)
	{
		worker_threads.emplace_back(worker_thread, g_a_over, i);
		DB_connect(i);
	}

	cout << "connect DB" << endl;
	for (auto& th : worker_threads)
		th.join();
}



