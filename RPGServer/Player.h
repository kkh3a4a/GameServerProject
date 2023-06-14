#pragma once
#include "NetWork.h"

class Player : public Object
{
public:
	
	WSA_OVER_EX _wsa_recv_over;
	SOCKET	_socket;
	int		_prev_size{};
	char	_packet_buf[BUF_SIZE]{};
	int		_db_id;
	int		_exp;
	int		_movecount = 0;
	volatile int _wake = true;
	chrono::system_clock::time_point _last_attack_time = chrono::system_clock::now();
	chrono::system_clock::time_point _p_last_move_time = chrono::system_clock::now();
	chrono::system_clock::time_point _last_dead_time = chrono::system_clock::now();
	Player(int id, S_STATE _state);
	~Player();

	void send_packet(void* packet);
	void do_recv();
	void send_login_info_packet();
	void send_add_object_packet(int o_id);
	void send_move_packet(int o_id);
	void send_remove_object_packet(int o_id);
	void send_chat_packet(int p_id, const char* mess);
	void send_change_hp(int o_id);
	void send_location_DB();
	void kill_NPC(int n_id);
	void dead_player();
	void respawn_player();
};
