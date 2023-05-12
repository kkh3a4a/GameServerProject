#include <iostream>
#include "NetWork.h"
#include"WorkThread.h"
#include"Player.h"
#include <concurrent_unordered_set.h>

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")
using namespace std;




WSA_OVER_EX g_a_over;
void initObject()
{
	for (int i = 0; i < MAX_USER; ++i)
	{
		objects[i] = new Player(i , ST_FREE);
		Player* player = reinterpret_cast<Player*>(objects[i]);

	}
	cout << "init Object" << endl;
}



int main() {
	initObject();
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
	//
	//	initialize_npc();
	//
	//	thread timer_thread{ do_timer };
	vector <thread> worker_threads;
	int num_threads = std::thread::hardware_concurrency();
	for (int i = 0; i < num_threads; ++i)
		worker_threads.emplace_back(worker_thread, g_a_over);
	for (auto& th : worker_threads)
		th.join();
	//
	//	timer_thread.join();
	closesocket(g_s_socket);
	WSACleanup();
}










//
//bool CAS(volatile bool* addr, bool expected, bool update)
//{
//	return atomic_compare_exchange_strong(reinterpret_cast <volatile atomic_bool*>(addr), &expected, update);
//}
//
//void do_npc_ai(int npc_id);
//mutex l_q;
//
//enum COMP_TYPE { OP_ACCEPT, OP_RECV, OP_SEND, OP_NPC_AI };
//enum EVENT_TYPE { EV_RANDOM_MOVE, EV_ATTACK, EV_HEAL };
//class OVER_EXP {
//public:
//	WSAOVERLAPPED _over;
//	WSABUF _wsabuf;
//	char _send_buf[BUF_SIZE];
//	COMP_TYPE _comp_type;
//	EVENT_TYPE _e_type;
//	OVER_EXP()
//	{
//		_wsabuf.len = BUF_SIZE;
//		_wsabuf.buf = _send_buf;
//		_comp_type = OP_RECV;
//		ZeroMemory(&_over, sizeof(_over));
//	}
//	OVER_EXP(char* packet)
//	{
//		_wsabuf.len = packet[0];
//		_wsabuf.buf = _send_buf;
//		ZeroMemory(&_over, sizeof(_over));
//		_comp_type = OP_SEND;
//		memcpy(_send_buf, packet, packet[0]);
//	}
//};
//
//enum S_STATE { ST_FREE, ST_ALLOC, ST_INGAME, ST_UNUSED };		//ST_UNUSED : 처음 생성할때만 사용되며 이후 최대 플레이어 접속 수 만큼만 check할수 있도록 판단하게 해줌
//class SESSION {
//	OVER_EXP _recv_over;
//
//public:
//	mutex _s_lock;
//	S_STATE _state;
//	int _id;
//	SOCKET _socket;
//	short	x, y;
//	char	_name[NAME_SIZE];
//	int		_prev_remain;
//	int		_last_move_time;
//	volatile bool _n_wake;	// 주변에 플레이어가 있다면 한번만 wake를 true 바꿔주어 wake작동하게 한다.
//
//	unordered_set <int> _view_list;
//	shared_mutex _vl;
//public:
//	SESSION()
//	{
//		_id = -1;
//		_socket = 0;
//		x = y = 0;
//		_name[0] = 0;
//		_state = ST_UNUSED;
//		_prev_remain = 0;
//	}
//
//	~SESSION() {}
//
//	void do_recv()
//	{
//		DWORD recv_flag = 0;
//		memset(&_recv_over._over, 0, sizeof(_recv_over._over));
//		_recv_over._wsabuf.len = BUF_SIZE - _prev_remain;
//		_recv_over._wsabuf.buf = _recv_over._send_buf + _prev_remain;
//		WSARecv(_socket, &_recv_over._wsabuf, 1, 0, &recv_flag,
//			&_recv_over._over, 0);
//	}
//
//	void do_send(void* packet)
//	{
//		OVER_EXP* sdata = new OVER_EXP{ reinterpret_cast<char*>(packet) };
//		WSASend(_socket, &sdata->_wsabuf, 1, 0, 0, &sdata->_over, 0);
//	}
//	void send_login_info_packet()
//	{
//		SC_LOGIN_INFO_PACKET p;
//		p.id = _id;
//		p.size = sizeof(SC_LOGIN_INFO_PACKET);
//		p.type = SC_LOGIN_INFO;
//		p.x = x;
//		p.y = y;
//		do_send(&p);
//	}
//	void send_move_packet(int c_id);
//	void send_add_player_packet(int c_id);
//	void send_remove_player_packet(int c_id)
//	{
//		if (_view_list.count(c_id) != 0)
//		{
//			{
//				std::unique_lock<std::shared_mutex> lock(_vl);
//				if (_view_list.count(c_id) == 0) {
//					lock.unlock();
//					return;
//				}
//				_view_list.erase(c_id);
//			}
//			SC_REMOVE_PLAYER_PACKET p;
//			p.id = c_id;
//			p.size = sizeof(p);
//			p.type = SC_REMOVE_PLAYER;
//			do_send(&p);
//		}
//
//	}
//};
//
//array<SESSION, MAX_USER + MAX_NPC> clients;
//
//SOCKET g_s_socket, g_c_socket;
//OVER_EXP g_a_over;
//HANDLE g_h_iocp;
//
//bool is_NPC(int _id) {
//	if (_id < MAX_USER)
//		return false;
//	return true;
//}
//class EVENT
//{
//public:
//	int _object_id;
//	EVENT_TYPE _type;
//	chrono::system_clock::time_point _exec_time;;
//
//
//	EVENT(int id, EVENT_TYPE t, chrono::system_clock::time_point tm) : _object_id(id), _type(t), _exec_time(tm) {}
//	constexpr bool operator <(const EVENT& _Left) const
//	{
//		return (_exec_time > _Left._exec_time);
//	}
//};
//
//priority_queue<EVENT> g_timer_queue;
//
//void SESSION::send_move_packet(int c_id)
//{
//	{
//		std::shared_lock<std::shared_mutex> lock(_vl);
//		if (_view_list.count(c_id) != 0) {
//			lock.unlock();
//			SC_MOVE_PLAYER_PACKET p;
//			p.id = c_id;
//			p.size = sizeof(SC_MOVE_PLAYER_PACKET);
//			p.type = SC_MOVE_PLAYER;
//			p.x = clients[c_id].x;
//			p.y = clients[c_id].y;
//			p.move_time = clients[c_id]._last_move_time;
//			do_send(&p);
//		}
//		else {
//			lock.unlock();
//			send_add_player_packet(c_id);
//		}
//	}
//}
//
//void SESSION::send_add_player_packet(int c_id)
//{
//	{
//		std::unique_lock<std::shared_mutex> lock(_vl);
//		if (_view_list.count(c_id) != 0) {
//			lock.unlock();
//			send_move_packet(c_id);
//			return;
//		}
//		_view_list.insert(c_id);
//	}
//
//	SC_ADD_PLAYER_PACKET add_packet;
//	add_packet.id = c_id;
//	strcpy_s(add_packet.name, clients[c_id]._name);
//	add_packet.size = sizeof(add_packet);
//	add_packet.type = SC_ADD_PLAYER;
//	add_packet.x = clients[c_id].x;
//	add_packet.y = clients[c_id].y;
//	do_send(&add_packet);
//}
//
//
//int get_new_client_id()
//{
//	for (int i = 0; i < MAX_USER; ++i) {
//		lock_guard <mutex> ll{ clients[i]._s_lock };
//		if (clients[i]._state == ST_FREE || clients[i]._state == ST_UNUSED)
//			return i;
//	}
//	return -1;
//}
//
//bool can_see(int p1, int p2)
//{
//	// return VIEW_RANGE <= SQRT((p1.x - p2.x) ^ 2 + (p1.y - p2.y) ^ 2);
//	if (abs(clients[p1].x - clients[p2].x) > VIEW_RANGE) return false;
//	if (abs(clients[p1].y - clients[p2].y) > VIEW_RANGE) return false;
//	return true;
//}
//void wake_up_npc(int id)
//{
//	EVENT ev(id, EV_RANDOM_MOVE, chrono::system_clock::now());
//	l_q.lock();
//	g_timer_queue.push(ev);
//	l_q.unlock();
//}
//void process_packet(int c_id, char* packet)
//{
//	switch (packet[1]) {
//	case CS_LOGIN: {
//		CS_LOGIN_PACKET* p = reinterpret_cast<CS_LOGIN_PACKET*>(packet);
//		strcpy_s(clients[c_id]._name, p->name);
//		clients[c_id].x = rand() % W_WIDTH;
//		clients[c_id].y = rand() % W_HEIGHT;
//		clients[c_id].send_login_info_packet();
//		{
//			lock_guard<mutex> ll{ clients[c_id]._s_lock };
//			clients[c_id]._state = ST_INGAME;
//		}
//		for (auto& pl : clients) {
//			{
//				lock_guard<mutex> ll(pl._s_lock);
//				if (pl._state == ST_INGAME)
//				{
//				}
//				else if (pl._state == ST_FREE || pl._state == ST_ALLOC)
//				{
//					continue;
//				}
//				else if (pl._state == ST_UNUSED)
//				{
//					continue;
//				}
//			}
//
//			if (pl._id == c_id) continue;
//			if (can_see(c_id, pl._id) == false) continue;
//
//			pl.send_add_player_packet(c_id);
//			clients[c_id].send_add_player_packet(pl._id);
//			if (pl._id >= MAX_USER)
//			{
//				bool before_wake = clients[pl._id]._n_wake;
//				if (before_wake == false)
//				{
//					if (CAS(&clients[pl._id]._n_wake, before_wake, true))
//						wake_up_npc(pl._id);
//				}
//			}
//		}
//		break;
//	}
//	case CS_MOVE: {
//		CS_MOVE_PACKET* p = reinterpret_cast<CS_MOVE_PACKET*>(packet);
//		clients[c_id]._last_move_time = p->move_time;
//		short x = clients[c_id].x;
//		short y = clients[c_id].y;
//		switch (p->direction) {
//		case 0: if (y > 0) y--; break;
//		case 1: if (y < W_HEIGHT - 1) y++; break;
//		case 2: if (x > 0) x--; break;
//		case 3: if (x < W_WIDTH - 1) x++; break;
//		}
//		clients[c_id].x = x;
//		clients[c_id].y = y;
//		unordered_set <int> old_vl;
//		{
//			std::shared_lock<std::shared_mutex> lock(clients[c_id]._vl);
//			old_vl = clients[c_id]._view_list;
//		}
//
//		unordered_set <int> new_vl;
//		for (auto& cl : clients) {
//			if (cl._state == ST_INGAME)
//			{
//			}
//			else if (cl._state == ST_FREE || cl._state == ST_ALLOC)
//			{
//				continue;
//			}
//			else if (cl._state == ST_UNUSED)
//			{
//				continue;
//			}
//			if (cl._id == c_id) continue;
//			if (can_see(cl._id, c_id)) {
//				new_vl.insert(cl._id);
//			}
//		}
//
//		for (auto& o : new_vl) {
//			if (!is_NPC(o))
//			{
//				if (old_vl.count(o) == 0) {
//					clients[o].send_add_player_packet(c_id);
//					clients[c_id].send_add_player_packet(o);
//				}
//				else {
//					clients[o].send_move_packet(c_id);
//					clients[c_id].send_move_packet(o);
//				}
//			}
//			else
//			{
//				bool before_wake = clients[o]._n_wake;
//				if (before_wake == false)
//				{
//					if (CAS(&clients[o]._n_wake, before_wake, true))
//						wake_up_npc(o);
//				}
//			}
//
//		}
//		clients[c_id].send_move_packet(c_id);
//
//		for (auto op : old_vl)
//		{
//			if (clients[op]._state != ST_INGAME) continue;
//			if (clients[op]._id == c_id) continue;
//			if (new_vl.count(op) == 0)
//			{
//				clients[c_id].send_remove_player_packet(op);
//				if (!is_NPC(op))
//				{
//					clients[op].send_remove_player_packet(c_id);
//				}
//			}
//		}
//	}
//	}
//}
//
//void disconnect(int c_id)
//{
//	for (auto& pl : clients) {
//		{
//			lock_guard<mutex> ll(pl._s_lock);
//			if (pl._state == ST_INGAME)
//			{
//			}
//			else if (pl._state == ST_FREE || pl._state == ST_ALLOC)
//			{
//				continue;
//			}
//			else if (pl._state == ST_UNUSED)
//			{
//				continue;
//			}
//		}
//		if (pl._id == c_id) continue;
//		pl.send_remove_player_packet(c_id);
//	}
//	closesocket(clients[c_id]._socket);
//
//	lock_guard<mutex> ll(clients[c_id]._s_lock);
//	clients[c_id]._state = ST_FREE;
//}
//
//void worker_thread()
//{
//	while (true) {
//		DWORD num_bytes;
//		ULONG_PTR key;
//		WSAOVERLAPPED* over = nullptr;
//		BOOL ret = GetQueuedCompletionStatus(g_h_iocp, &num_bytes, &key, &over, INFINITE);
//		OVER_EXP* ex_over = reinterpret_cast<OVER_EXP*>(over);
//		if (FALSE == ret) {
//			if (ex_over->_comp_type == OP_ACCEPT) cout << "Accept Error";
//			else {
//				cout << "GQCS Error on client[" << key << "]\n";
//				disconnect(static_cast<int>(key));
//				if (ex_over->_comp_type == OP_SEND) delete ex_over;
//				continue;
//			}
//		}
//
//		if ((0 == num_bytes) && ((ex_over->_comp_type == OP_RECV) || (ex_over->_comp_type == OP_SEND))) {
//			disconnect(static_cast<int>(key));
//			if (ex_over->_comp_type == OP_SEND) delete ex_over;
//			continue;
//		}
//
//		switch (ex_over->_comp_type) {
//		case OP_ACCEPT: {
//			int client_id = get_new_client_id();
//			if (client_id != -1) {
//				{
//					lock_guard<mutex> ll(clients[client_id]._s_lock);
//					clients[client_id]._state = ST_ALLOC;
//				}
//				clients[client_id].x = 0;
//				clients[client_id].y = 0;
//				clients[client_id]._id = client_id;
//				clients[client_id]._name[0] = 0;
//				clients[client_id]._prev_remain = 0;
//				clients[client_id]._socket = g_c_socket;
//				clients[client_id]._view_list.clear();
//				CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_c_socket),
//					g_h_iocp, client_id, 0);
//				clients[client_id].do_recv();
//				g_c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
//			}
//			else {
//				cout << "Max user exceeded.\n";
//			}
//			ZeroMemory(&g_a_over._over, sizeof(g_a_over._over));
//			int addr_size = sizeof(SOCKADDR_IN);
//			AcceptEx(g_s_socket, g_c_socket, g_a_over._send_buf, 0, addr_size + 16, addr_size + 16, 0, &g_a_over._over);
//			break;
//		}
//		case OP_RECV: {
//			int remain_data = num_bytes + clients[key]._prev_remain;
//			char* p = ex_over->_send_buf;
//			while (remain_data > 0) {
//				int packet_size = p[0];
//				if (packet_size <= remain_data) {
//					process_packet(static_cast<int>(key), p);
//					p = p + packet_size;
//					remain_data = remain_data - packet_size;
//				}
//				else break;
//			}
//			clients[key]._prev_remain = remain_data;
//			if (remain_data > 0) {
//				memcpy(ex_over->_send_buf, p, remain_data);
//			}
//			clients[key].do_recv();
//			break;
//		}
//		case OP_SEND:
//		{
//			delete ex_over;
//			break;
//		}
//		case OP_NPC_AI:
//		{
//
//			do_npc_ai(static_cast<int>(key));
//
//			break;
//		}
//		}
//	}
//}
//
//
//void do_random_move(int n_id)
//{
//	short x = clients[n_id].x;
//	short y = clients[n_id].y;
//
//	unordered_set<int> near_list;
//	unordered_set<int> old_vlist;
//
//	for (int i = 0; i < MAX_USER; ++i) {
//		if (clients[i]._state == ST_ALLOC || clients[i]._state == ST_FREE) continue;
//		else if (clients[i]._state == ST_UNUSED)
//			break;
//		if (can_see(i, n_id) == false) continue;
//		old_vlist.insert(i);
//	}
//
//	switch (rand() % 4) {
//	case 0: if (y > 0) y--; break;
//	case 1: if (y < W_HEIGHT - 1) y++; break;
//	case 2: if (x > 0) x--; break;
//	case 3: if (x < W_WIDTH - 1) x++; break;
//	}
//	clients[n_id].x = x;
//	clients[n_id].y = y;
//
//	for (int i = 0; i < MAX_USER; ++i) {
//		auto& cl = clients[i];
//		if (cl._state != ST_INGAME) continue;
//		else if (cl._state == ST_UNUSED)
//			break;
//		if (can_see(cl._id, n_id))
//			near_list.insert(cl._id);
//	}
//	for (auto& pl : near_list) {
//		auto& cpl = clients[pl];
//		cpl._vl.lock();
//		if (cpl._view_list.count(n_id)) {
//			cpl._vl.unlock();
//			cpl.send_move_packet(n_id);
//		}
//		else {
//			cpl._vl.unlock();
//			cpl.send_add_player_packet(n_id);
//		}
//	}
//
//	for (auto& pl : old_vlist) {
//		if (0 == near_list.count(pl)) {
//			clients[pl].send_remove_player_packet(n_id);
//		}
//	}
//
//}
//void initialize_npc()
//{
//	for (int i = 0; i < MAX_NPC; ++i)
//	{
//		int npc_id = i + MAX_USER;
//		clients[npc_id].x = rand() % W_WIDTH;
//		clients[npc_id].y = rand() % W_HEIGHT;
//		clients[npc_id]._state = ST_INGAME;
//		clients[npc_id]._id = npc_id;
//		clients[npc_id]._n_wake = false;
//		sprintf_s(clients[npc_id]._name, "N%d", npc_id);
//	}
//	cout << "NPC_initialize success" << endl;
//}
//void do_npc_ai(int npc_id)
//{
//	bool skip_ai = true;
//	for (int j = 0; j < MAX_USER; ++j)
//	{
//		if (clients[j]._state != ST_INGAME) continue;
//		else if (clients[j]._state == ST_UNUSED)
//			break;
//		if (can_see(npc_id, j) == true)
//		{
//			skip_ai = false;
//			break;
//		}
//	}
//	while (skip_ai == true) {
//
//		bool before_wake = clients[npc_id]._n_wake;
//		if (before_wake == true)
//		{
//			if (!CAS(&clients[npc_id]._n_wake, before_wake, false))
//			{
//				continue;
//			}
//			else
//			{
//				return;
//			}
//		}
//		else
//			return;
//	}
//
//	do_random_move(npc_id);
//	EVENT ev{ npc_id, EV_RANDOM_MOVE,chrono::system_clock::now() + 1s };
//	l_q.lock();
//	g_timer_queue.push(ev);
//	l_q.unlock();
//}
//
//
//void do_timer()
//{
//	while (1)
//	{
//		if (g_timer_queue.empty())
//		{
//			this_thread::sleep_for(1ms);
//			continue;
//		}
//
//		l_q.lock();
//		auto ev = g_timer_queue.top();
//		if (ev._exec_time > chrono::system_clock::now())
//		{
//			l_q.unlock();
//			this_thread::sleep_for(1ms);
//			continue;
//		}
//		g_timer_queue.pop();
//		l_q.unlock();
//		int npc_id = ev._object_id;
//		OVER_EXP* over = new OVER_EXP;
//		over->_comp_type = OP_NPC_AI;
//		over->_e_type = ev._type;
//		PostQueuedCompletionStatus(g_h_iocp, 1, npc_id, &over->_over);
//
//	}
//}
//
//int main()
//{
//
//	WSADATA WSAData;
//	WSAStartup(MAKEWORD(2, 2), &WSAData);
//	g_s_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
//	SOCKADDR_IN server_addr;
//	memset(&server_addr, 0, sizeof(server_addr));
//	server_addr.sin_family = AF_INET;
//	server_addr.sin_port = htons(PORT_NUM);
//	server_addr.sin_addr.S_un.S_addr = INADDR_ANY;
//	bind(g_s_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
//	listen(g_s_socket, SOMAXCONN);
//	SOCKADDR_IN cl_addr;
//	int addr_size = sizeof(cl_addr);
//	g_h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
//	CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_s_socket), g_h_iocp, 9999, 0);
//	g_c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
//	g_a_over._comp_type = OP_ACCEPT;
//	AcceptEx(g_s_socket, g_c_socket, g_a_over._send_buf, 0, addr_size + 16, addr_size + 16, 0, &g_a_over._over);
//
//	initialize_npc();
//
//	thread timer_thread{ do_timer };
//	vector <thread> worker_threads;
//	int num_threads = std::thread::hardware_concurrency();
//	for (int i = 0; i < num_threads; ++i)
//		worker_threads.emplace_back(worker_thread);
//	for (auto& th : worker_threads)
//		th.join();
//
//	timer_thread.join();
//	closesocket(g_s_socket);
//	WSACleanup();
//}
