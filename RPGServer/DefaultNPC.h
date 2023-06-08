#pragma once
#include "NetWork.h"


class NPC : public Object
{
public:
	volatile bool _n_wake;	// �ֺ��� �÷��̾ �ִٸ� �ѹ��� wake�� true �ٲ��־� wake�۵��ϰ� �Ѵ�.
	
	void add_objects(int o_id);

	int _spawn_x = 0, _spawn_y = 0;
	int _last_attacker = 0;
	bool _is_batte = false;
	lua_State* _L;
	std::mutex _lua_lock;
	chrono::system_clock::time_point _last_attack_time = chrono::system_clock::now();
	void dead_NPC();
	void respawn_NPC();
	void heal_NPC();
	void move_NPC();
};
