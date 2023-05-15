#include"TimerThread.h"
#include"NetWork.h"






void TimerThread()
{
	while (1)
	{
		if (timer_queue.empty())
		{
			this_thread::sleep_for(1ms);
			continue;
		}
		EVENT _event;
		if (!timer_queue.try_pop(_event))
		{
			continue;	
		}
		retry_timer:
		if (_event._exec_time > chrono::system_clock::now())	//이미 꺼냈는데 다시넣는건 오버헤드 큰거같음
		{
			this_thread::sleep_for(_event._exec_time - chrono::system_clock::now());
			goto retry_timer;
		}
		switch (_event._e_type)
		{
		case EV_RANDOM_MOVE:
		{
			int npc_id = _event._o_id;
			WSA_OVER_EX* over = new WSA_OVER_EX(OP_NPC_RANDOMMOVE, 0,0);
			ZeroMemory(&over->_wsaover, sizeof(over->_wsaover));
			over->_iocpop = OP_NPC_RANDOMMOVE;
			over->_e_type = _event._e_type;
			PostQueuedCompletionStatus(h_iocp, 1, npc_id, &over->_wsaover);
			break;
		}
		default:
			break;
		}
		

	}

}
