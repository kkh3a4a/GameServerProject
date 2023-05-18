#pragma once
#include "NetWork.h"

class NPC : public Object
{
public:
	volatile bool _n_wake;	// 주변에 플레이어가 있다면 한번만 wake를 true 바꿔주어 wake작동하게 한다.
	
	void add_objects(int o_id);

	lua_State* _L;
	std::mutex _lua_lock;
	chrono::system_clock::time_point _last_hello_time;
};
