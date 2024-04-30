#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <WinSock2.h>
#include <winsock.h>
#include <Windows.h>
#include <iostream>
#include <thread>
#include <vector>
#include <unordered_set>
#include <mutex>
#include <atomic>
#include <chrono>
#include <queue>
#include <array>
#include <memory>

using namespace std;
using namespace chrono;

extern HWND		hWnd;

const static int MAX_TEST = 10000;

//const static int MAX_CLIENTS = 1;
const static int MAX_CLIENTS = MAX_TEST*2;
const static int INVALID_ID = -1;
const static int MAX_PACKET_SIZE = 512;
const static int MAX_BUFF_SIZE = 512;

#pragma comment (lib, "ws2_32.lib")

#include "../protocol_2023.h"

HANDLE g_hiocp;

enum IOCPOP { OP_SEND, OP_RECV, OP_DO_MOVE };

high_resolution_clock::time_point last_connect_time;

struct WSA_OVER_EX {
	WSAOVERLAPPED _wsaover;
	IOCPOP _iocpop;
	WSABUF _wsabuf;	
	int _e_type;
	char _buf[MAX_BUFF_SIZE];
};


struct CLIENT {
	int id;
	int x;
	int y;
	atomic_bool connected;

	SOCKET client_socket;
	WSA_OVER_EX _wsaover;
	int _prev_size;
	high_resolution_clock::time_point last_move_time;
};

array<int, MAX_CLIENTS> client_map;
array<CLIENT, MAX_CLIENTS> g_clients;
atomic_int num_connections;
atomic_int client_to_close;
atomic_int active_clients;

int			global_delay;				// ms단위, 1000이 넘으면 클라이언트 증가 종료

vector <thread*> worker_threads;
thread test_thread;

float point_cloud[MAX_TEST * 2];

// 나중에 NPC까지 추가 확장 용
struct ALIEN {
	int id;
	int x, y;
	int visible_count;
};
void do_recv(int c_id);
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
	std::wcout << L"에러" << lpMsgBuf << std::endl;

	MessageBox(hWnd, lpMsgBuf, L"ERROR", 0);
	LocalFree(lpMsgBuf);
	// while (true);
}

void DisconnectClient(int ci)
{
	bool status = true;
	if (true == atomic_compare_exchange_strong(&g_clients[ci].connected, &status, false)) {
		closesocket(g_clients[ci].client_socket);
		active_clients--;
	}
}

void SendPacket(int cl, void* packet)
{
	int psize = reinterpret_cast<unsigned char*>(packet)[0];
	int ptype = reinterpret_cast<unsigned char*>(packet)[2];
	WSA_OVER_EX* over = new WSA_OVER_EX;
	over->_iocpop = OP_SEND;
	memcpy(over->_buf, packet, psize);
	ZeroMemory(&over->_wsaover, sizeof(over->_wsaover));
	over->_wsabuf.buf = reinterpret_cast<CHAR*>(over->_buf);
	over->_wsabuf.len = psize;
	int ret = WSASend(g_clients[cl].client_socket, &over->_wsabuf, 1, NULL, 0,
		&over->_wsaover, NULL);
	if (0 != ret) {
		int err_no = WSAGetLastError();
		if (WSA_IO_PENDING != err_no)
			error_display("Error in SendPacket:", err_no);
	}
}

void processpacket(int ci, char packet[])
{
	char* packet_data = static_cast<char*> (packet);

	unsigned char p_type = packet[2];
	switch (p_type) {
	case SC_MOVE_OBJECT: {
		SC_MOVE_OBJECT_PACKET* move_packet = reinterpret_cast<SC_MOVE_OBJECT_PACKET*>(packet);
		if (move_packet->id < MAX_CLIENTS) {
			int my_id = client_map[move_packet->id];
			if (-1 != my_id) {
				g_clients[my_id].x = move_packet->x;
				g_clients[my_id].y = move_packet->y;
			}
			if (ci == my_id) {
				if (0 != move_packet->move_time) {
					unsigned long long m_t = static_cast<unsigned long long>(duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count());
					auto d_ms = m_t - move_packet->move_time;

					if (global_delay < d_ms) global_delay++;
					else if (global_delay > d_ms) global_delay--;
				}
			}
		}
		break;
	}
					 
	case SC_ADD_OBJECT: break;
	case SC_REMOVE_OBJECT: break;
	case SC_LOGIN_OK: break;
	case SC_LOGIN_FAIL: break;
	case SC_HP_CHANGE: break;
	case SC_STAT_CHANGE: break;
	case SC_LOGIN_INFO:
	{
		g_clients[ci].connected = true;
		active_clients++;
		SC_LOGIN_INFO_PACKET* login_packet = reinterpret_cast<SC_LOGIN_INFO_PACKET*>(packet);
		int my_id = ci;
		client_map[login_packet->id] = my_id;
		g_clients[my_id].id = login_packet->id;
		g_clients[my_id].x = login_packet->x;
		g_clients[my_id].y = login_packet->y;

		break;
	}
	case SC_CHAT: break;
	case SC_ATTACK_RANGE: break;
	default:
		cout << "error" << endl;
		break;
		MessageBox(hWnd, L"Unknown Packet Type", L"ERROR", 0);
		while (true);

	}
}

void Worker_Thread()
{
	while (true) {
		DWORD io_size;
		unsigned long long ci;
		WSA_OVER_EX* over;
		BOOL ret = GetQueuedCompletionStatus(g_hiocp, &io_size, &ci,
			reinterpret_cast<LPWSAOVERLAPPED*>(&over), INFINITE);
		// std::cout << "GQCS :";
		int client_id = static_cast<int>(ci);
		if (FALSE == ret) {
			int err_no = WSAGetLastError();
			if (64 == err_no) DisconnectClient(client_id);
			else {
				// error_display("GQCS : ", WSAGetLastError());
				DisconnectClient(client_id);
			}
			if (OP_SEND == over->_iocpop) delete over;
		}
		if (0 == io_size) {
			DisconnectClient(client_id);
			continue;
		}
		if (OP_RECV == over->_iocpop) {
			//std::cout << "RECV from Client :" << ci;
			//std::cout << "  IO_SIZE : " << io_size << std::endl;
			int remain_data = io_size + g_clients[ci]._prev_size;
			char buf[MAX_BUFF_SIZE * 2];
			memcpy(buf, over->_buf, remain_data);
			short* p = reinterpret_cast<short*>(buf);
			while (remain_data > 0) {
				int packet_size = p[0];
				if (packet_size <= remain_data) {
					processpacket(static_cast<int>(ci), buf);
					char* m_buf = buf + packet_size;
					remain_data = remain_data - packet_size;
					memcpy(buf, m_buf, remain_data);
					p = reinterpret_cast<short*>(buf);
				}
				else {
					break;
				}
			}
			g_clients[ci]._prev_size = remain_data;
			if (remain_data > 0) {
				memcpy(over->_buf, p, remain_data);
			}

			do_recv(ci);
			
			if (SOCKET_ERROR == ret) {
				int err_no = WSAGetLastError();
				if (err_no != WSA_IO_PENDING)
				{
					//error_display("RECV ERROR", err_no);
					DisconnectClient(client_id);
				}
			}
		}
		else if (OP_SEND == over->_iocpop) {
			if (io_size != over->_wsabuf.len) {
				// std::cout << "Send Incomplete Error!\n";
				DisconnectClient(client_id);
			}
			delete over;
		}
		else if (OP_DO_MOVE == over->_iocpop) {
			// Not Implemented Yet
			delete over;
		}
		else {
			std::cout << "Unknown GQCS event!\n";
			while (true);
		}
	}
}

constexpr unsigned long long DELAY_LIMIT = 100;
constexpr unsigned long long DELAY_LIMIT2 = 150;
constexpr unsigned long long ACCEPT_DELY = 50;

void Adjust_Number_Of_Client()
{
	static int delay_multiplier = 1;
	static int max_limit = MAXINT;
	static bool increasing = true;

	if (active_clients >= MAX_TEST) return;
	if (num_connections >= MAX_CLIENTS) return;

	auto duration = high_resolution_clock::now() - last_connect_time;
	if (ACCEPT_DELY * delay_multiplier > duration_cast<milliseconds>(duration).count()) return;

	int t_delay = global_delay;
	if (DELAY_LIMIT2 < t_delay) {
		if (true == increasing) {
			max_limit = active_clients;
			increasing = false;
		}
		if (100 > active_clients) return;
		if (ACCEPT_DELY * 10 > duration_cast<milliseconds>(duration).count()) return;
		last_connect_time = high_resolution_clock::now();
		DisconnectClient(client_to_close);
		client_to_close++;
		return;
	}
	else
		if (DELAY_LIMIT < t_delay) {
			delay_multiplier = 10;
			return;
		}
	if (max_limit - (max_limit / 20) < active_clients) return;

	increasing = true;
	last_connect_time = high_resolution_clock::now();
	g_clients[num_connections].client_socket = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	SOCKADDR_IN ServerAddr;
	ZeroMemory(&ServerAddr, sizeof(SOCKADDR_IN));
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(PORT_NUM);
	ServerAddr.sin_addr.s_addr = inet_addr("127.0.0.1");


	int Result = WSAConnect(g_clients[num_connections].client_socket, (sockaddr*)&ServerAddr, sizeof(ServerAddr), NULL, NULL, NULL, NULL);
	if (0 != Result) {
		error_display("WSAConnect : ", GetLastError());
	}

	g_clients[num_connections]._prev_size = 0;
	ZeroMemory(&g_clients[num_connections]._wsaover, sizeof(g_clients[num_connections]._wsaover));
	g_clients[num_connections]._wsaover._iocpop = OP_RECV;
	g_clients[num_connections]._wsaover._wsabuf.buf = g_clients[num_connections]._wsaover._buf;
	g_clients[num_connections]._wsaover._wsabuf.len = sizeof(g_clients[num_connections]._wsaover._buf);

	DWORD recv_flag = 0;
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_clients[num_connections].client_socket), g_hiocp, num_connections, 0);

	CS_LOGIN_PACKET l_packet;

	int temp = num_connections + 10000;
	sprintf_s(l_packet.name, "D%d", temp);
	l_packet.size = sizeof(l_packet);
	l_packet.type = CS_LOGIN;
	SendPacket(num_connections, &l_packet);

	do_recv(num_connections);
	num_connections++;
fail_to_connect:
	return;
}

void Test_Thread()
{
	while (true) {
		//Sleep(max(20, global_delay));
		Adjust_Number_Of_Client();

		for (int i = 0; i < num_connections; ++i) {
			if (false == g_clients[i].connected) continue;
			if (g_clients[i].last_move_time + 1s > high_resolution_clock::now()) continue;
			g_clients[i].last_move_time = high_resolution_clock::now();
			CS_MOVE_PACKET my_packet;
			my_packet.size = sizeof(my_packet);
			my_packet.type = CS_MOVE;
			switch (rand() % 4) {
			case 0: my_packet.direction = 0; break;
			case 1: my_packet.direction = 1; break;
			case 2: my_packet.direction = 2; break;
			case 3: my_packet.direction = 3; break;
			}
			my_packet.move_time = static_cast<unsigned long long>(duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count());
			//if (i == 0)
				//cout << my_packet.move_time << endl;
			SendPacket(i, &my_packet);
		}
	}
}

void InitializeNetwork()
{
	for (auto& cl : g_clients) {
		cl.connected = false;
		cl.id = INVALID_ID;
	}

	for (auto& cl : client_map) cl = -1;
	num_connections = 0;
	last_connect_time = high_resolution_clock::now();

	WSADATA	wsadata;
	WSAStartup(MAKEWORD(2, 2), &wsadata);

	g_hiocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, NULL, 0);

	for (int i = 0; i < 6; ++i)
		worker_threads.push_back(new std::thread{ Worker_Thread });

	test_thread = thread{ Test_Thread };
}

void ShutdownNetwork()
{
	test_thread.join();
	for (auto pth : worker_threads) {
		pth->join();
		delete pth;
	}
}

void Do_Network()
{
	return;
}

void GetPointCloud(int* size, float** points)
{
	int index = 0;
	for (int i = 0; i < num_connections; ++i)
		if (true == g_clients[i].connected) {
			point_cloud[index * 2] = static_cast<float>(g_clients[i].x);
			point_cloud[index * 2 + 1] = static_cast<float>(g_clients[i].y);
			index++;
		}

	*size = index;
	*points = point_cloud;
}

void do_recv(int ci)
{
	DWORD recv_flag = 0;
	memset(&g_clients[ci]._wsaover._wsaover, 0, sizeof(g_clients[ci]._wsaover._wsaover));
	g_clients[ci]._wsaover._wsabuf.len = BUF_SIZE - g_clients[ci]._prev_size;
	g_clients[ci]._wsaover._wsabuf.buf = g_clients[ci]._wsaover._buf+ g_clients[ci]._prev_size;
	g_clients[ci]._wsaover._iocpop = OP_RECV;

	int ret = WSARecv(g_clients[ci].client_socket,
		&g_clients[ci]._wsaover._wsabuf, 1,
		NULL, &recv_flag, &g_clients[ci]._wsaover._wsaover, NULL);
}
