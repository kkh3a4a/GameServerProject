
constexpr int DB_PORT_NUM = 5000;
constexpr int DB_BUF_SIZE = 255;
constexpr int DB_NAME_SIZE = 20;
/////////////////////// Packet ID 50บฮลอ

constexpr char DS_PLAYER_LOGIN = 50;








constexpr char SD_PLAYER_LOGIN = 50;

//////////////////////////

struct  DS_PLAYER_LOGIN_PACKET {
	unsigned char size;
	char	type;
	int		id;
};

struct  SD_PLAYER_LOGIN_PACKET {
	unsigned char size;
	char	type;
	int		id;
};



#define DB_SERVER_ADDR "127.0.0.1"

#pragma pack (push, 1)


#pragma pack (pop)