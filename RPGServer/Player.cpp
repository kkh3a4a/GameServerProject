#include "Player.h"
#include"Zone.h"
#include"DefaultNPC.h"


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
	int length = std::strlen(mess);
	string msg(mess, length + 1);
	
	
	SC_CHAT_PACKET packet;
	packet.id = p_id;
	packet.type = SC_CHAT;
	strcpy_s(packet.mess, msg.c_str());
	packet.size = sizeof(SC_CHAT_PACKET) - (CHAT_SIZE - msg.size());

	send_packet(&packet);
}

void Player::send_change_hp(int o_id)
{
	{
		SC_HP_CHANGE_PACKET packet;
		packet.hp = objects[o_id]->_hp;
		packet.id = objects[o_id]->_id;
		packet.max_hp = objects[o_id]->_max_hp;
		packet.size = sizeof(packet);
		packet.type = SC_HP_CHANGE;
		send_packet(&packet);
	}
	{
		SD_PLAYER_CHANGE_STAT_PACKET packet;
		packet.size = sizeof(packet);
		packet.type = SD_PLAYER_CHANGE_STAT;
		packet.id = _db_id;
		packet.level = _level;
		packet.max_hp = _max_hp;
		packet.exp = _exp;
		packet.hp = _hp;

		DB_send_packet(&packet);
	}

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
	cout << "P" << _db_id << " kill " << n_id << " [get N" << objects[n_id]->_level * objects[n_id]->_level * 20 << " exp]\n";
	if (_exp >= _level * 100)
	{
		cout << "P" << _id << " : Level Up [" << _id << "]\n";
		_exp -= _level * 100;
		_max_hp = (int)((float)_max_hp * (1.1));
		_hp = _max_hp;
		_dmg += _level;
		_level++;
	}

	SD_PLAYER_CHANGE_STAT_PACKET packet;
	packet.size = sizeof(packet);
	packet.type = SD_PLAYER_CHANGE_STAT;
	packet.id = _db_id;
	packet.level = _level;
	packet.max_hp = _max_hp;
	packet.exp = _exp;
	packet.hp = _hp;

	DB_send_packet(&packet);

}

void Player::dead_player()
{
	
	{
		SD_PLAYER_CHANGE_STAT_PACKET packet;
		_exp = _exp / 2;
		packet.size = sizeof(packet);
		packet.type = SD_PLAYER_CHANGE_STAT;
		packet.id = _db_id;
		packet.level = _level;
		packet.max_hp = _max_hp;
		packet.exp = _exp;
		packet.hp = _max_hp;
		DB_send_packet(&packet);
	}
	EVENT ev{ _id, EV_RESPAWN, chrono::system_clock::now() + 5s };
	//l_q.lock();
	timer_queue.push(ev);

	
	//cout << _id << " dead\n";

	_last_dead_time = chrono::system_clock::now();

}

void Player::respawn_player()
{
	{
		//cout << _id << " respawn\n";
		_hp = _max_hp;
		{
			std::unique_lock<std::shared_mutex> lock(_s_lock);
			_state = ST_INGAME;
		}

		Player* player = reinterpret_cast<Player*>(objects[_id]);
		short x = player->_x;
		short y = player->_y;
		int b_my_zoneY, b_my_zoneX;
		b_my_zoneY = player->_y / ZONE_SEC;
		b_my_zoneX = player->_x / ZONE_SEC;

		retry:
		short r_x = rand() % W_WIDTH;
		short r_y = rand() % W_HEIGHT;
		if (World_Map.find(make_pair(r_x, r_y)) != World_Map.end())
			goto retry;

		player->_x = r_x;
		player->_y = r_y;
		int my_zoneY, my_zoneX;
		my_zoneY = player->_y / ZONE_SEC;
		my_zoneX = player->_x / ZONE_SEC;

		if (my_zoneX != b_my_zoneX || my_zoneY != b_my_zoneY)
		{
			zone[b_my_zoneY][b_my_zoneX]->REMOVE(player->_id);
			zone[my_zoneY][my_zoneX]->ADD(player->_id);
		}

		unordered_set <int> old_vl;
		{
			std::shared_lock<std::shared_mutex> lock(player->_vl);
			old_vl = player->_view_list;
		}

		unordered_set <int> new_vl;

		set<int> z_list;
		zone_check(player->_x, player->_y, z_list);
		for (auto& p_id : z_list) {
			{
				std::shared_lock<std::shared_mutex> lock(objects[p_id]->_s_lock);
				if (objects[p_id]->_state != ST_INGAME)
				{
					lock.unlock();
					continue;
				}
			}
			if (objects[p_id]->_id == _id) continue;
			if (can_see(objects[p_id]->_id, _id)) {
				new_vl.insert(objects[p_id]->_id);
			}
		}


		for (auto& o : new_vl) {
			if (!is_NPC(o))
			{
				Player* pl = reinterpret_cast<Player*>(objects[o]);

				if (old_vl.count(o) == 0) {
					pl->send_add_object_packet(_id);
					player->send_add_object_packet(o);
				}
				else {
					pl->send_move_packet(_id);
					player->send_move_packet(o);
				}
			}
			else
			{
				NPC* nl = reinterpret_cast<NPC*>(objects[o]);

				bool before_wake = nl->_n_wake;
				if (old_vl.count(o) == 0) {
					nl->add_objects(_id);
					player->send_add_object_packet(o);
				}
				else {
					player->send_move_packet(o);

				}
				if (before_wake == 0)
				{
					if (CAS(reinterpret_cast<volatile int*>(&nl->_n_wake), before_wake, 1))
					{
						WSA_OVER_EX was;
						was.wake_up_npc(o);
					}
				}
			}
		}
		player->send_move_packet(_id);

		for (auto& p_id : old_vl) {
			if (objects[p_id]->_state != ST_INGAME) continue;
			if (objects[p_id]->_id == _id) continue;
			if (new_vl.count(p_id) == 0)
			{
				player->send_remove_object_packet(p_id);
				if (!is_NPC(p_id))
				{
					Player* pl = reinterpret_cast<Player*>(objects[p_id]);
					pl->send_remove_object_packet(_id);
				}
				else {
					NPC* npc = reinterpret_cast<NPC*>(objects[p_id]);
				}
			}
		}
		player->_movecount++;
		player->send_location_DB();
	}
}
