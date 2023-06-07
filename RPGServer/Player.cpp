#include "Player.h"




Player::Player(int id, S_STATE state)
{
	_x = 0; _y = 0; _z = 0;
	_id = id;
	_name[NAME_SIZE - 1] = {};
	_socket = { 0 };
	_state = state;
	_db_id = 0;
	_movecount = 0;
}

void Player::send_packet(void* packet)
{
	char* buf = reinterpret_cast<char*>(packet);
	unsigned short* buf_size = reinterpret_cast<unsigned short*>(packet);
	WSA_OVER_EX * _wsa_send_over = new WSA_OVER_EX(OP_SEND, buf_size[0], packet);
	
	int ret = WSASend(_socket, &_wsa_send_over->_wsabuf, 1, NULL, 0, &_wsa_send_over->_wsaover, NULL);
}

void Player::do_recv()
{
	DWORD recv_flag = 0;
	memset(&_wsa_recv_over._wsaover, 0, sizeof(_wsa_recv_over._wsaover));
	_wsa_recv_over._wsabuf.len = BUF_SIZE - _prev_size;
	_wsa_recv_over._wsabuf.buf = _wsa_recv_over._buf + _prev_size;
	_wsa_recv_over._iocpop = OP_RECV;
	WSARecv(_socket, &_wsa_recv_over._wsabuf, 1, 0, &recv_flag,
		&_wsa_recv_over._wsaover, 0);
}

void Player::send_login_info_packet()
{
	SC_LOGIN_INFO_PACKET packet;
	packet.id = _id;
	packet.size = sizeof(SC_LOGIN_INFO_PACKET);
	packet.type = SC_LOGIN_INFO;
	packet.x = _x;
	packet.y = _y;
	packet.max_hp = _max_hp;
	packet.hp = _hp;
	send_packet(&packet);
}

void Player::send_add_object_packet(int o_id)
{
	{
		std::unique_lock<std::shared_mutex> lock(_vl);
		if (_view_list.count(o_id) != 0) {
			lock.unlock();
			send_move_packet(o_id);
			return;
		}
		_view_list.insert(o_id);
	}	
	SC_ADD_OBJECT_PACKET packet;
	packet.id = o_id;
	strcpy_s(packet.name, objects[o_id]->_name);
	packet.size = sizeof(packet);
	packet.type = SC_ADD_OBJECT;
	packet.x = objects[o_id]->_x;
	packet.y = objects[o_id]->_y;
	send_packet(&packet);

	if(o_id < MAXMOVEOBJECT)
	{
		send_change_hp(o_id);
	}

}

void Player::send_move_packet(int o_id)
{
	{
		std::shared_lock<std::shared_mutex> lock(_vl);
		if (_view_list.count(o_id) != 0) {
			lock.unlock();
			SC_MOVE_OBJECT_PACKET p;
			p.id = o_id;
			p.size = sizeof(SC_MOVE_OBJECT_PACKET);
			p.type = SC_MOVE_OBJECT;
			p.x = objects[o_id]->_x;
			p.y = objects[o_id]->_y;
			p.move_time = objects[o_id]->_last_move_time;
			send_packet(&p);
		}
		else {
			lock.unlock();
			send_add_object_packet(o_id);
		}
	}
}

void Player::send_remove_object_packet(int o_id)
{

	{
		std::unique_lock<std::shared_mutex> lock(_vl);
		if (_view_list.count(o_id) == 0) {
			lock.unlock();
			return;
		}
		_view_list.erase(o_id);
	}
	SC_REMOVE_OBJECT_PACKET p;
	p.id = o_id;
	p.size = sizeof(p);
	p.type = SC_REMOVE_OBJECT;
	send_packet(&p);
}

void Player::send_chat_packet(int p_id, const char* mess)
{
	SC_CHAT_PACKET packet;
	packet.id = p_id;
	packet.size = sizeof(packet);
	packet.type = SC_CHAT;
	strcpy_s(packet.mess, mess);
	send_packet(&packet);
}

void Player::send_change_hp(int o_id)
{
	SC_HP_CHANGE_PACKET packet;
	packet.hp = objects[o_id]->_hp;
	packet.id = objects[o_id]->_id;
	packet.max_hp = objects[o_id]->_max_hp;
	packet.size = sizeof(packet);
	packet.type = SC_HP_CHANGE;

	send_packet(&packet);
}

void Player::send_location_DB()
{
	SD_PLAYER_LOCATION_PACKET packet;
	packet.id = _db_id;
	packet.x = _x;
	packet.y = _y;
	packet.size = sizeof(packet);
	packet.type = SD_PLAYER_LOCATION;
	DB_send_packet(&packet);
}

void Player::kill_NPC(int n_id)
{
	
  	_exp += (objects[n_id]->_level * objects[n_id]->_level * 20);
	if (_exp >= _level * 100)
	{
		cout << _id << " : Level Up [" << _id << "]" << endl;
		_exp -= _level * 100;
		_max_hp = (int)((float)_max_hp * (1.1));
		_hp = _max_hp;
		_dmg += _level;
		_level++;
	}

	SD_PLAYER_CHANGE_EXP_PACKET packet;
	packet.size = sizeof(packet);
	packet.type = SD_PLAYER_CHANGE_EXP;
	packet.id = _db_id;
	packet.level = _level;
	packet.max_hp = _max_hp;
	packet.exp = _exp;

	DB_send_packet(&packet);

}
