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
	_x = _spawn_x;
	_y = _spawn_y;
	_last_attacker = 0;
	_is_batte = false;
	_view_list.clear();
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
	{
		std::unique_lock<std::shared_mutex> lock(_vl);
		_view_list = near_list;
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


	if (_n_type == 1 || _n_type == 3)
	{
		EVENT ev(_id, EV_RANDOM_MOVE, chrono::system_clock::now());
		//l_q.lock();
		timer_queue.push(ev);
	}
	else if (_n_type == 2 || _n_type == 4)
	{
		EVENT ev(_id, EV_WAIT, chrono::system_clock::now());
		//l_q.lock();
		timer_queue.push(ev);
	}
}

void NPC::heal_NPC()
{
	EVENT ev{ _id, EV_HEAL, chrono::system_clock::now() + 20s };
	//l_q.lock();
	timer_queue.push(ev);

}

void NPC::move_NPC()
{
	if (_state != ST_INGAME)	// 간혹 한번 더 이동하기 vs lock 걸기 , lock걸지말자. 
		return;
	Player* pl = reinterpret_cast<Player*>(objects[_last_attacker]);
	if(pl->_state == ST_INGAME)
	{
		if (pl->_last_dead_time > chrono::system_clock::now() - 3s || pl->_state != ST_INGAME)
		{
			EVENT ev{ _id, EV_RANDOM_MOVE,chrono::system_clock::now() + 1s };
			//l_q.lock();
			timer_queue.push(ev);
			_is_batte = false;
			return;
		}
		if (abs(pl->_x - _x) > VIEW_RANGE || abs(pl->_y - _y) > VIEW_RANGE) {
			EVENT ev{ _id, EV_RANDOM_MOVE,chrono::system_clock::now() + 1s };
			//l_q.lock();
			timer_queue.push(ev);
			_is_batte = false;
			return;
		}
		unordered_set<int> near_list;
		unordered_set<int> old_vlist;
		set<int> p_list;

		zone_check(_x, _y, p_list);
		for (auto& p_id : p_list) {
			if (objects[p_id]->_state != ST_INGAME) continue;
			if (can_see(p_id, _id) == false) continue;
			if (p_id >= MAX_USER)
				break;
			old_vlist.insert(p_id);
		}
		int b_my_zoneY, b_my_zoneX;
		b_my_zoneY = _y / ZONE_SEC;
		b_my_zoneX = _x / ZONE_SEC;
		if (abs(objects[_last_attacker]->_x - _x) <= 1 && abs(objects[_last_attacker]->_y - _y) <= 1)
		{
			int attack_time = 1000;
			_lua_lock.lock();
			lua_getglobal(_L, "event_NPC_Attack_msg");
			lua_pcall(_L, 0, 0, 0);
			_lua_lock.unlock();
			EVENT ev{ _id, EV_ATTACK, chrono::system_clock::now() + chrono::milliseconds(attack_time) };
			timer_queue.push(ev);
			send_attack_range(attack_time);

			return;
		}
		int x = _x;
		int y = _y;
		if (move_queue.empty())
		{
			if (objects[_last_attacker]->_x - _x < -1)
			{
				x--;
			}
			else if (objects[_last_attacker]->_x - _x > 1)
			{
				x++;
			}
			if (objects[_last_attacker]->_y - _y < -1)
			{
				y--;
			}
			else if (objects[_last_attacker]->_y - _y > 1)
			{
				y++;
			}
			if (World_Map.find(make_pair(x, y)) == World_Map.end())
			{


				_x = x;
				_y = y;

				int my_zoneY, my_zoneX;
				my_zoneY = _y / ZONE_SEC;
				my_zoneX = _x / ZONE_SEC;
				if (my_zoneX != b_my_zoneX || my_zoneY != b_my_zoneY)
				{
					zone[b_my_zoneY][b_my_zoneX]->REMOVE(_id);
					zone[my_zoneY][my_zoneX]->ADD(_id);
				}

				set<int> z_list;
				zone_check(_x, _y, z_list);
				for (auto& p_id : z_list) {
					if (p_id >= MAX_USER)
						break;
					Player* pl = reinterpret_cast<Player*>(objects[p_id]);
					if (pl->_state != ST_INGAME) continue;
					if (can_see(pl->_id, _id))
						near_list.insert(pl->_id);
				}
				for (auto& p_id : near_list) {
					Player* pl = reinterpret_cast<Player*>(objects[p_id]);
					std::shared_lock<std::shared_mutex> lock(pl->_vl);
					if (pl->_view_list.count(_id)) {
						lock.unlock();
						pl->send_move_packet(_id);
					}
					else {
						lock.unlock();
						pl->send_add_object_packet(_id);
					}
				}



				for (auto& p_id : old_vlist) {
					if (0 == near_list.count(p_id)) {
						Player* pl = reinterpret_cast<Player*>(objects[p_id]);
						pl->send_remove_object_packet(_id);
					}
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
				{
					std::unique_lock<std::shared_mutex> lock(_vl);
					_view_list = near_list;
				}
				if (abs(objects[_last_attacker]->_x - _x) <= 1 && abs(objects[_last_attacker]->_y - _y) <= 1)
				{
					int attack_time = 1000;
					_lua_lock.lock();
					lua_getglobal(_L, "event_NPC_Attack_msg");
					lua_pcall(_L, 0, 0, 0);
					_lua_lock.unlock();
					EVENT ev{ _id, EV_ATTACK, chrono::system_clock::now() + chrono::milliseconds(attack_time) };
					timer_queue.push(ev);
					send_attack_range(attack_time);
					return;
				}

			}
			else
			{
				if (_x == objects[_last_attacker]->_x)
				{
					int i = 1;
					while (1)
					{
						if (World_Map.find(make_pair(_x + i, y)) == World_Map.end())
						{
							if (World_Map.find(make_pair(_x - i, y)) == World_Map.end())
							{
								if (objects[_last_attacker]->_x < _x)
								{
									i *= -1;
								}
								else if (objects[_last_attacker]->_x > _x)
								{

								}
							}
							break;
						}
						else if (World_Map.find(make_pair(_x - i, y)) == World_Map.end())
						{
							i *= -1;
							break;
						}
						i++;
					}
					if (i > 0)
					{
						for (int j = 1; j <= i; ++j)
						{
							move_queue.push(make_pair(_x + j, _y));
						}
						move_queue.push(make_pair(_x + i, y));
					}
					else
					{
						for (int j = -1; j >= i; --j)
						{
							move_queue.push(make_pair(_x + j, _y));
						}
						move_queue.push(make_pair(_x + i, y));
					}

				}
				else if (_y == objects[_last_attacker]->_y)
				{
					int i = 1;
					while (1)
					{
						if (World_Map.find(make_pair(x, _y + i)) == World_Map.end())
						{
							if (World_Map.find(make_pair(x, _y - i)) == World_Map.end())
							{
								if (objects[_last_attacker]->_y < _y)
								{
									i *= -1;
								}
								else if (objects[_last_attacker]->_y > _y)
								{

								}
							}
							break;
						}
						else if (World_Map.find(make_pair(x, _y - i)) == World_Map.end())
						{
							i *= -1;
							break;
						}
						i++;
					}
					if (i > 0)
					{
						for (int j = 1; j <= i; ++j)
						{
							move_queue.push(make_pair(_x, _y + j));
						}
						move_queue.push(make_pair(x, _y + i));
					}
					else
					{
						for (int j = -1; j >= i; --j)
						{
							move_queue.push(make_pair(_x, _y + j));
						}
						move_queue.push(make_pair(x, _y + i));
					}
				}
				else
				{
					if (World_Map.find(make_pair(x, _y)) == World_Map.end())
					{
						if (objects[_last_attacker]->_x - _x == 1)
							move_queue.push(make_pair(x + 1, _y));
						else if (objects[_last_attacker]->_x - _x == -1)
							move_queue.push(make_pair(x - 1, _y));
						else
							move_queue.push(make_pair(x, _y));
					}
					else if (World_Map.find(make_pair(_x, y)) == World_Map.end())
					{
						if (objects[_last_attacker]->_y - _y == 1)
							move_queue.push(make_pair(_x, y + 1));
						else if (objects[_last_attacker]->_y - _y == -1)
							move_queue.push(make_pair(_x, y - 1));
						else
							move_queue.push(make_pair(_x, y));
					}
				}

				EVENT ev{ _id, EV_MOVE,chrono::system_clock::now() };
				//l_q.lock();
				timer_queue.push(ev);
				return;
			}
		}
		else {
			pair<short, short> point;
			if (move_queue.try_pop(point))
			{
				_x = point.first;
				_y = point.second;
				int my_zoneY, my_zoneX;
				my_zoneY = _y / ZONE_SEC;
				my_zoneX = _x / ZONE_SEC;
				if (my_zoneX != b_my_zoneX || my_zoneY != b_my_zoneY)
				{
					zone[b_my_zoneY][b_my_zoneX]->REMOVE(_id);
					zone[my_zoneY][my_zoneX]->ADD(_id);
				}

				set<int> z_list;
				zone_check(_x, _y, z_list);
				for (auto& p_id : z_list) {
					if (p_id >= MAX_USER)
						break;
					Player* pl = reinterpret_cast<Player*>(objects[p_id]);
					if (pl->_state != ST_INGAME) continue;
					if (can_see(pl->_id, _id))
						near_list.insert(pl->_id);
				}
				for (auto& p_id : near_list) {
					Player* pl = reinterpret_cast<Player*>(objects[p_id]);
					std::shared_lock<std::shared_mutex> lock(pl->_vl);
					if (pl->_view_list.count(_id)) {
						lock.unlock();
						pl->send_move_packet(_id);
					}
					else {
						lock.unlock();
						pl->send_add_object_packet(_id);
					}
				}



				for (auto& p_id : old_vlist) {
					if (0 == near_list.count(p_id)) {
						Player* pl = reinterpret_cast<Player*>(objects[p_id]);
						pl->send_remove_object_packet(_id);
					}
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
				if (abs(objects[_last_attacker]->_x - _x) <= 1 && abs(objects[_last_attacker]->_y - _y) <= 1)
				{
					int attack_time = 1000;
					move_queue.clear();

					_lua_lock.lock();
					lua_getglobal(_L, "event_NPC_Attack_msg");
					lua_pcall(_L, 0, 0, 0);
					_lua_lock.unlock();

					EVENT ev{ _id, EV_ATTACK, chrono::system_clock::now() + chrono::milliseconds(attack_time) };
					timer_queue.push(ev);
					send_attack_range(attack_time);
					return;
				}
			}
		}
	}
	

	EVENT ev{ _id, EV_MOVE,chrono::system_clock::now() + 1s };
	//l_q.lock();
	timer_queue.push(ev);
}

void NPC::send_attack_range(int attack_time)
{
	SC_ATTACK_RANGE_PACKET packet;
	packet.id = _id;
	packet.type = SC_ATTACK_RANGE;
	packet.attack_time = attack_time;
	string s;

	if((_n_type % 2) == 1)
	{
		for (int i = -1; i <= 1; ++i)
		{
			for (int j = -1; j <= 1; ++j)
			{
				char buffer[100];

				sprintf_s(buffer, "%d %d\n", _x + j, _y + i);
				s += buffer;
			}
		}
	}
	else if ((_n_type % 2) == 0)
	{
		char buffer[100];
		attack_range.clear();
		sprintf_s(buffer, "%d %d\n", objects[_last_attacker]->_x, objects[_last_attacker]->_y);
		s += buffer;
		attack_range.push(make_pair(objects[_last_attacker]->_x, objects[_last_attacker]->_y));
	}
	strcpy_s(packet.range, sizeof(char) * (s.size() + 1), s.c_str());
	packet.size = sizeof(SC_ATTACK_RANGE_PACKET) - (200 - s.size() - 1);
	for (auto p_id : _view_list)
	{
		if (p_id >= MAX_USER)
			break;
		if (objects[p_id]->_state != ST_INGAME) continue;
		Player* pl = reinterpret_cast<Player*>(objects[p_id]);
		pl->send_packet(&packet);
	}
	
}

void NPC::do_range_attack()
{
	if (_state != ST_INGAME)	// 간혹 한번 더 이동하기 vs lock 걸기 , lock걸지말자. 
		return;
	Player* pl = reinterpret_cast<Player*>(objects[_last_attacker]);
	if(pl->_state == ST_INGAME)
	{
		if (abs(pl->_x - _x) > VIEW_RANGE || abs(pl->_y - _y) > VIEW_RANGE) {
			EVENT ev{ _id, EV_WAIT, chrono::system_clock::now() + 1s };
			//l_q.lock();
			timer_queue.push(ev);
			_is_batte = false;
			return;
		}
		if (pl->_last_dead_time > chrono::system_clock::now() - 3s || pl->_state != ST_INGAME)
		{
			EVENT ev{ _id, EV_WAIT, chrono::system_clock::now() + 1s };
			//l_q.lock();
			timer_queue.push(ev);
			_is_batte = false;
			return;
		}
		if (abs(objects[_last_attacker]->_x - _x) <= 5 && abs(objects[_last_attacker]->_y - _y) <= 5)
		{
			int attack_time = 1000;
			_lua_lock.lock();
			lua_getglobal(_L, "event_NPC_Attack_msg");
			lua_pcall(_L, 0, 0, 0);
			_lua_lock.unlock();
			EVENT ev{ _id, EV_RANGEATTACK, chrono::system_clock::now() + chrono::milliseconds(attack_time) };
			timer_queue.push(ev);
			send_attack_range(attack_time);
			return;
		}
	}

	EVENT ev{ _id, EV_WAIT, chrono::system_clock::now() + 1s };
	timer_queue.push(ev);
	_is_batte = false;
	return;
}


