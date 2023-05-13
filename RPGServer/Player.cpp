#include "Player.h"




Player::Player(int id, S_STATE state)
{
	_x = 0; _y = 0; _z = 0;
	_id = id;
	_name[NAME_SIZE - 1] = {};
	_socket = { 0 };
	_state = state;
}

void Player::send_packet(void* packet)
{
	char* buf = reinterpret_cast<char*>(packet);

	WSA_OVER_EX* _wsa_send_over = new WSA_OVER_EX(OP_SEND, buf[0], packet);
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
}

void Player::send_move_packet(int o_id)
{
	SC_MOVE_OBJECT_PACKET p;
	p.id = o_id;
	p.size = sizeof(SC_MOVE_OBJECT_PACKET);
	p.type = SC_MOVE_OBJECT;
	p.x = objects[o_id]->_x;
	p.y = objects[o_id]->_y;
	p.move_time = objects[o_id]->_last_move_time;
	send_packet(&p);
	/*{
		std::shared_lock<std::shared_mutex> lock(_vl);
		if (_view_list.count(c_id) != 0) {
			lock.unlock();
			SC_MOVE_OBJECT_PACKET p;
			p.id = c_id;
			p.size = sizeof(SC_MOVE_OBJECT_PACKET);
			p.type = SC_MOVE_OBJECT;
			p.x = objects[o_id]->_x;
			p.y = objects[o_id]->_y;
			p.move_time = objects[o_id]->_last_move_time;
			send_packet(&p);
		}
		else {
			lock.unlock();
			send_add_player_packet(c_id);
		}
	}*/
}

void Player::send_remove_object_packet(int o_id)
{
	if (_view_list.count(o_id) != 0)
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
}
