#include "Player.h"





Player::Player(int id, S_STATE state)
{
	_x = 0; _y = 0; _z = 0;
	_state = state;
	_id = id;
	_name[NAME_SIZE - 1] = {};
	_socket = { 0 };
	_state = state;
}

void Player::send_packet(void* packet)
{
	char* buf = reinterpret_cast<char*>(packet);

	WSA_OVER_EX* _wsa_send_over = new WSA_OVER_EX(OP_SEND, buf[0], packet);

	if (_state == S_STATE::ST_INGAME)
		WSASend(_socket, &_wsa_send_over->_wsabuf, 1, NULL, 0, &_wsa_send_over->_wsaover, NULL);
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
