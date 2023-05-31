#include "WorkThread.h"

using namespace std;


void worker_thread(WSA_OVER_EX g_a_over)
{
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
			CreateIoCompletionPort(reinterpret_cast<HANDLE>(SV_socket), h_iocp, key, 0);
			key++;
			do_recv();
			break;
		}

		case OP_RECV: {
			int remain_data = num_bytes + _prev_size;
			char* p = ex_over->_buf;
			while (remain_data > 0) {
				int packet_size = p[0];
				if (packet_size <= remain_data) {
					ex_over->processpacket(static_cast<int>(key), p);
					p = p + packet_size;
					remain_data = remain_data - packet_size;
				}
				else break;
			}

			_prev_size = remain_data;
			if (remain_data > 0) {
				memcpy(ex_over->_buf, p, remain_data);
			}
			do_recv();
			break;
		}
		case OP_SEND:
		{
			delete ex_over;
			break;
		}
		}
	}
}
