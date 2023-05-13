#pragma once
#include<unordered_set>
#include<shared_mutex>
class Object
{
public:
	Object();

	int		_x, _y, _z;		//location
	int		_id;	
	int		_last_move_time;
	char	_name[20]{};
	std::unordered_set <int> _view_list;
	std::shared_mutex _vl;

};