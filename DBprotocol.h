
constexpr int DB_PORT_NUM = 5000;
constexpr int DB_BUF_SIZE = 512;
constexpr int DB_NAME_SIZE = 20;
/////////////////////// Packet ID 50บฮลอ

constexpr char DS_PLAYER_LOGIN = 50;
constexpr char DS_PLAYER_LOCATION = 51;
constexpr char DS_PLAYER_CHANGE_STAT = 60;
constexpr char DS_CHAT = 52;
constexpr int DB_THREAD_NUM = 8;





constexpr char SD_PLAYER_LOGIN = 50;
constexpr char SD_PLAYER_LOCATION = 51;
constexpr char SD_CHAT = 52;
constexpr char SD_PLAYER_CHANGE_STAT = 60;

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

struct DS_PLAYER_CHANGE_STAT_PACKET
{
	unsigned short size;
	char	type;

};



struct  SD_PLAYER_LOGIN_PACKET {
	unsigned short size;
	char	type;
	int		id;
	int		s_id;
	char	name[20];
};

struct	SD_PLAYER_CHANGE_STAT_PACKET {
	unsigned short size;
	char	type;

	int		id;
	int		exp;
	int		level;
	int		hp;
	int		max_hp;

};

struct	SD_PLAYER_LOCATION_PACKET{
	unsigned short size;
	char	type;

	int		id;
	int		x;
	int		y;
};
struct SD_CHAT_PACKET {
	unsigned short size;
	char	type;
	int		id;
	char	time[20];
	char	mess[200];
};


#define DB_SERVER_ADDR "127.0.0.1"

#pragma pack (push, 1)


#pragma pack (pop)