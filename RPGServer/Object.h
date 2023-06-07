#pragma once
#include<unordered_set>
#include<shared_mutex>
enum S_STATE { ST_FREE, ST_ALLOC, ST_INGAME};

class Object
{
public:
	Object();

	int		_x, _y, _z;		//location
	int		_id;	
	int		_dmg;
	int		_last_move_time;	
	char	_name[20]{};
	int		_hp;
	int		_max_hp;
	int		_level;
	S_STATE	_state = ST_FREE;
	std::shared_mutex _s_lock;

	std::unordered_set <int> _view_list;
	std::shared_mutex _vl;

};