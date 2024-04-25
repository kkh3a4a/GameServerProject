#include "WorkThread.h"
#include <mutex>
using namespace std;
mutex ml;

void worker_thread(WSA_OVER_EX g_a_over, int w_id)
{
	cout << std::this_thread::get_id() << " : " << w_id << endl;
	while (true) 
	{
		DWORD num_bytes;
		ULONG_PTR key;
		WSAOVERLAPPED* over = nullptr;

		BOOL ret = GetQueuedCompletionStatus(h_iocp, &num_bytes, &key, &over, INFINITE);
		WSA_OVER_EX* ex_over = reinterpret_cast<WSA_OVER_EX*>(over);
		if (FALSE == ret) {
			if (ex_over->_iocpop == OP_ACCEPT) cout << "Accept Error";
			else {
				cout << "server[" << key << "]\n";
				ex_over->disconnect(static_cast<int>(key));
				if (ex_over->_iocpop == OP_SEND) delete ex_over;
				continue;
			}
		}

		if ((0 == num_bytes) && ((ex_over->_iocpop == OP_RECV) || (ex_over->_iocpop == OP_SEND))) {
			ex_over->disconnect(static_cast<int>(key));
			if (ex_over->_iocpop == OP_SEND) delete ex_over;
			continue;
		}

		switch (ex_over->_iocpop) {
		case OP_ACCEPT: {
			static int key = 0;
			_prev_size[key] = 0;
			G_server[key] = G_socket;
			CreateIoCompletionPort(reinterpret_cast<HANDLE>(G_socket), h_iocp, key, 0);
			do_recv(key);
			G_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			ZeroMemory(&g_a_over._wsaover, sizeof(g_a_over._wsaover));
			int addr_size = sizeof(SOCKADDR_IN);
			AcceptEx(DB_socket, G_socket, g_a_over._buf, 0, addr_size + 16, addr_size + 16, 0, &g_a_over._wsaover);
			cout << "Server[" << key << "]" << " connect!" << endl;
			
			key++;

			
			break;
		}

		case OP_RECV: {
			int remain_data = num_bytes + _prev_size[key];
			char buf[DB_BUF_SIZE * 2];
			memcpy(buf, ex_over->_buf - _prev_size[key], _prev_size[key] + num_bytes);
			short* p = reinterpret_cast<short*>(buf);
			while (remain_data > 0) {
				int packet_size = p[0];
				if (packet_size > 500)
				{
					DebugBreak();
				}
				if (packet_size <= remain_data) {
					ex_over->processpacket(static_cast<int>(key), p, w_id);
					char* m_buf = buf + packet_size;
					remain_data = remain_data - packet_size;
					memcpy(buf, m_buf, remain_data);
					p = reinterpret_cast<short*>(buf);
				}
				else break;
			}

			_prev_size[key] = remain_data;
			if (remain_data > 0) {
				memcpy(ex_over->_buf, p, remain_data);
			}
			do_recv(key);
			break;
		}
		case OP_SEND:
		{
			delete ex_over;
			break;
		}
		}
	}

	cout << std::this_thread::get_id() << " : " << w_id << endl;
}
