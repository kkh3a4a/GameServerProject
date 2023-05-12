constexpr int PORT_NUM = 4000;
constexpr int BUF_SIZE = 200;
constexpr int NAME_SIZE = 20;

constexpr int MAX_USER = 10000;
constexpr int MAX_NPC = 200000;
constexpr int MAXOBJECT = MAX_USER + MAX_NPC;

constexpr int W_WIDTH = 2000;
constexpr int W_HEIGHT = 2000;
constexpr int ZONE_SEC = 40;
// Packet ID
constexpr char CS_LOGIN = 0;
constexpr char CS_MOVE = 1;

constexpr char SC_LOGIN_INFO = 2;
constexpr char SC_ADD_OBJECT = 3;
constexpr char SC_REMOVE_OBJECT = 4;
constexpr char SC_MOVE_OBJECT = 5;

#pragma pack (push, 1)
struct CS_LOGIN_PACKET {
	unsigned char size;
	char	type;
	char	name[NAME_SIZE];
};

struct CS_MOVE_PACKET {
	unsigned char size;
	char	type;
	char	direction;  // 0 : UP, 1 : DOWN, 2 : LEFT, 3 : RIGHT
	unsigned	move_time;
};

struct SC_LOGIN_INFO_PACKET {
	unsigned char size;
	char	type;
	int	id;
	short	x, y;
};

struct SC_ADD_OBJECT_PACKET {
	unsigned char size;
	char	type;
	int	id;
	short	x, y;
	char	name[NAME_SIZE];
};

struct SC_REMOVE_OBJECT_PACKET {
	unsigned char size;
	char	type;
	int	id;
};

struct SC_MOVE_OBJECT_PACKET {
	unsigned char size;
	char	type;
	int	id;
	short	x, y;
	unsigned int move_time;
};

#pragma pack (pop)