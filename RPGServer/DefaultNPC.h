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
	short _n_type = 1;						// 1, 2 peace 3, 4 agro
											// 1, 3 �ι� 2, 4 ����
											// 2 : ���浥��, 4: ��ũ���浥��
	bool agro = 0;
	concurrency::concurrent_queue<pair<short, short>> attack_range;
	concurrency::concurrent_queue<pair<short, short>> move_queue;
	lua_State* _L;
	std::mutex _lua_lock;
	volatile int _last_attack_check = 1;
	void dead_NPC();
	void respawn_NPC();
	void heal_NPC();
	void move_NPC();
	void send_attack_range(int attack_time);
	void do_range_attack();
};
