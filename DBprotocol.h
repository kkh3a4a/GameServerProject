
constexpr int DB_PORT_NUM = 5000;
constexpr int DB_BUF_SIZE = 512;
constexpr int DB_NAME_SIZE = 20;
/////////////////////// Packet ID 50บฮลอ

constexpr char DS_PLAYER_LOGIN = 50;
constexpr char DS_PLAYER_LOCATION = 51;
constexpr char DS_PLAYER_CHANGE_EXP = 60;

constexpr int DB_THREAD_NUM = 8;





constexpr char SD_PLAYER_LOGIN = 50;
constexpr char SD_PLAYER_LOCATION = 51;
constexpr char SD_PLAYER_CHANGE_EXP = 60;
//////////////////////////

struct  DS_PLAYER_LOGIN_PACKET {
	unsigned short size;
	char	type;

	int		s_id;
	int		id;
	int		hp;
	int		max_hp;
	int		exp;
	int		level;
	int		x, y;
};

struct DS_PLAYER_CHANGE_EXP_PACKET
{
	unsigned short size;
	char	type;

	int		s_id;
	int		id;
	int		hp;
	int		max_hp;
	int		exp;
	int		level;
	int		x, y;
};



struct  SD_PLAYER_LOGIN_PACKET {
	unsigned short size;
	char	type;
	int		id;
	int		s_id;
	char	name[20];
};

struct	SD_PLAYER_CHANGE_EXP_PACKET {
	unsigned short size;
	char	type;

	int		s_id;
	int		id;
	int		exp;
	int		level;
	int		max_hp;

};

struct	SD_PLAYER_LOCATION_PACKET{
	unsigned short size;
	char	type;

	int		id;
	int		x;
	int		y;
};



#define DB_SERVER_ADDR "127.0.0.1"

#pragma pack (push, 1)


#pragma pack (pop)