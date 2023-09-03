#include "NetWork.h"
#include"WorkThread.h"

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")



using namespace std;



WSA_OVER_EX g_a_over;




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



