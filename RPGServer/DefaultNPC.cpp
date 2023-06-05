#include "DefaultNPC.h"
#include"Zone.h"
#include"Player.h"
void NPC::add_objects(int o_id)
{
	{
		std::unique_lock<std::shared_mutex> lock(_vl);
		if (_view_list.count(o_id) != 0) {
			lock.unlock();
			return;
		}
		_view_list.insert(o_id);
	}
}

void NPC::dead_NPC()
{

	{
		std::unique_lock<std::shared_mutex> lock(_s_lock);
		_state = ST_FREE;
	}
	_n_wake = false;
	EVENT ev{ _id, EV_RESPAWN, chrono::system_clock::now() + 5s };
	//l_q.lock();
	timer_queue.push(ev);

	unordered_set<int> near_list;
	set<int> z_list;
	zone_check(_x, _y, z_list);
	for (auto& p_id : z_list) {
		if (p_id >= MAX_USER)
			break;
		Player* pl = reinterpret_cast<Player*>(objects[p_id]);
		if (pl->_state != ST_INGAME) continue;
		if (p_id >= MAX_USER)
			break;
		if (can_see(pl->_id, _id))
			near_list.insert(pl->_id);
	}
	for (auto& p_id : near_list) {
		Player* pl = reinterpret_cast<Player*>(objects[p_id]);
		std::shared_lock<std::shared_mutex> lock(pl->_vl);
		if (pl->_view_list.count(_id)) {
			lock.unlock();
			pl->send_remove_object_packet(_id);
		}
	}

}

void NPC::respawn_NPC()
{
	unordered_set<int> near_list;

	set<int> z_list;
	zone_check(_x, _y, z_list);
	for (auto& p_id : z_list) {
		if (p_id >= MAX_USER)
			break;
		Player* pl = reinterpret_cast<Player*>(objects[p_id]);
		if (pl->_state != ST_INGAME) continue;
		if (p_id >= MAX_USER)
			break;
		if (can_see(pl->_id, _id))			
			near_list.insert(pl->_id);
	}
	for (auto& p_id : near_list) {
		Player* pl = reinterpret_cast<Player*>(objects[p_id]);
		pl->send_add_object_packet(_id);
	}

	if (near_list.size() == 0)
	{
		bool before_wake = _n_wake;
		if (before_wake == 1)
		{
			if (!CAS(reinterpret_cast<volatile int*>(&_n_wake), before_wake, 0))
			{

			}
			else
			{
				return;
			}
		}
		else
			return;
	}
	else
	{
		CAS(reinterpret_cast<volatile int*>(&_n_wake), false, 1);
	}



	EVENT ev{ _id, EV_RANDOM_MOVE, chrono::system_clock::now() + 1s };
	timer_queue.push(ev);
}
