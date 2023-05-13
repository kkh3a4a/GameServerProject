#pragma once
#include "NetWork.h"

class Player : public Object
{
public:
	
	WSA_OVER_EX _wsa_recv_over;
	S_STATE	_state = ST_FREE;
	SOCKET	_socket;
	int		_prev_size{};
	char	_packet_buf[BUF_SIZE]{};
	
	std::shared_mutex _s_lock;
	Player(int id, S_STATE _state);
	~Player();

	void send_packet(void* packet);
	void do_recv();
	void send_login_info_packet();
	void send_add_object_packet(int o_id);
	void send_move_packet(int o_id);
	void send_remove_object_packet(int o_id);
};
