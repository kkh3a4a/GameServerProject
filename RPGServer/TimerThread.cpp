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
		retry_pop:
		EVENT _event;
		if (!timer_queue.try_pop(_event))
		{
			continue;	
		}
		retry_timer:
		if (_event._exec_time > chrono::system_clock::now())	//이미 꺼냈는데 다시넣는건 오버헤드 큰거같음
		{
			if ((_event._exec_time - chrono::system_clock::now()).count() > 100)	//0.01초보다 크면 다시 넣자....
			{
   				timer_queue.push(_event);
				goto retry_pop;
			}
			this_thread::sleep_for(1ms);
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
		case EV_RESPAWN:
		{
			int npc_id = _event._o_id;
			WSA_OVER_EX* over = new WSA_OVER_EX(OP_NPC_RESPAWN, 0, 0);
			ZeroMemory(&over->_wsaover, sizeof(over->_wsaover));
			over->_iocpop = OP_NPC_RESPAWN;
			over->_e_type = _event._e_type;
			PostQueuedCompletionStatus(h_iocp, 1, npc_id, &over->_wsaover);
			break;
		}
		case EV_MOVE:
		{
			int npc_id = _event._o_id;
			WSA_OVER_EX* over = new WSA_OVER_EX(OP_NPC_MOVE, 0, 0);
			ZeroMemory(&over->_wsaover, sizeof(over->_wsaover));
			over->_iocpop = OP_NPC_MOVE;
			over->_e_type = _event._e_type;
			PostQueuedCompletionStatus(h_iocp, 1, npc_id, &over->_wsaover);
			break;
		}
		case EV_HEAL:
		{
			int _id = _event._o_id;
			if (objects[_id]->_state != ST_INGAME)
			{
				break;
			}
			WSA_OVER_EX* over = new WSA_OVER_EX(OP_NPC_HEAL, 0, 0);
			ZeroMemory(&over->_wsaover, sizeof(over->_wsaover));
			over->_iocpop = OP_NPC_HEAL;
			over->_e_type = _event._e_type;
			PostQueuedCompletionStatus(h_iocp, 1, _id, &over->_wsaover);
			break;
		}
		case EV_DEFENCE:
		{
			int npc_id = _event._o_id;
			WSA_OVER_EX* over = new WSA_OVER_EX(OP_NPC_DEFENCE, 0, 0);
			ZeroMemory(&over->_wsaover, sizeof(over->_wsaover));
			over->_iocpop = OP_NPC_DEFENCE;
			over->_e_type = _event._e_type;
			PostQueuedCompletionStatus(h_iocp, 1, npc_id, &over->_wsaover);
			break;
		}
		case EV_ATTACK:
		{
			int npc_id = _event._o_id;
			WSA_OVER_EX* over = new WSA_OVER_EX(OP_NPC_ATTACK, 0, 0);
			ZeroMemory(&over->_wsaover, sizeof(over->_wsaover));
			over->_iocpop = OP_NPC_ATTACK;
			over->_e_type = _event._e_type;
			PostQueuedCompletionStatus(h_iocp, 1, npc_id, &over->_wsaover);
			break;
		}
		case EV_RANGEATTACK:
		{
			int npc_id = _event._o_id;
			WSA_OVER_EX* over = new WSA_OVER_EX(OP_NPC_RANGEATTACK, 0, 0);
			ZeroMemory(&over->_wsaover, sizeof(over->_wsaover));
			over->_iocpop = OP_NPC_RANGEATTACK;
			over->_e_type = _event._e_type;
			PostQueuedCompletionStatus(h_iocp, 1, npc_id, &over->_wsaover);
			break;
		}
		case EV_WAIT:
		{
			int npc_id = _event._o_id;
			WSA_OVER_EX* over = new WSA_OVER_EX(OP_NPC_WAIT, 0, 0);
			ZeroMemory(&over->_wsaover, sizeof(over->_wsaover));
			over->_iocpop = OP_NPC_WAIT;
			over->_e_type = _event._e_type;
			PostQueuedCompletionStatus(h_iocp, 1, npc_id, &over->_wsaover);
			break;
		}
		default:
			break;
		}
		

	}

}


