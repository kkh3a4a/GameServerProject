#define SFML_STATIC 1
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <unordered_map>
#include <Windows.h>
#include <chrono>
#include <fstream>
#include <sstream>
#include <string>
#include<set>
using namespace std;

#pragma comment (lib, "opengl32.lib")
#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "ws2_32.lib")

//#include "../protocol.h"
#include "../protocol_2023.h"
sf::TcpSocket s_socket;

constexpr auto SCREEN_WIDTH = 16;
constexpr auto SCREEN_HEIGHT = 16;

constexpr auto TILE_WIDTH = 40;
constexpr auto WINDOW_WIDTH = SCREEN_WIDTH * TILE_WIDTH;   // size of window
constexpr auto WINDOW_HEIGHT = SCREEN_WIDTH * TILE_WIDTH;
bool isEnter = false;
int g_left_x;
int g_top_y;
int g_myid;
volatile bool login_success = false;
chrono::system_clock::time_point _last_attack_time = chrono::system_clock::now();
sf::RenderWindow* g_window;
sf::Font g_font;
sf::Texture* hpTexture;
sf::Texture* clearTexture;
sf::Texture* fire;
sf::Texture* stone;
sf::Texture* wood;
sf::Texture* monster1;
sf::Texture* monster2;
sf::Texture* monster3;
sf::Texture* monster4;


float borderWidth = 2.0f;
sf::Color borderColor = sf::Color::Black;
sf::RectangleShape borderShape;
sf::RectangleShape borderShape2;
sf::Sprite m_Stamina;
sf::Sprite m_fire;
sf::Sprite m_Stone;
sf::Sprite m_Wood;
sf::Sprite chat;
sf::Sprite m_attack_range;
sf::Sprite p_attack_range;
sf::Sprite m_monster1;
sf::Sprite m_monster2;
sf::Sprite m_monster3;
sf::Sprite m_monster4;


int stamina = 100;
int max_stamina = 100;
std::map<std::pair<short, short>, short> World_Map;

class OBJECT {
private:
	bool m_showing;
	sf::Sprite m_sprite;

	sf::Text m_name;
	sf::Text m_chat;
	
public:
	chrono::system_clock::time_point m_mess_end_time;
	sf::Sprite m_HP;
	map<pair<short, short>, chrono::system_clock::time_point> attack_range;
	int id;
	int m_x, m_y;
	int hp;
	int _max_hp;
	char name[NAME_SIZE];
	OBJECT(sf::Texture& t, int x, int y, int x2, int y2) {
		m_showing = false;
		m_sprite.setTexture(t);
		//m_sprite.setScale(0.5f, 0.5f);
		m_sprite.setTextureRect(sf::IntRect(x, y, x2, y2));
		set_name("NONAME");
		
		
		m_HP.setTexture(*hpTexture);
		m_HP.setColor(sf::Color::Green);
		m_mess_end_time = chrono::system_clock::now();
		hp = 0;
	}
	OBJECT() {
		m_showing = false;
	}
	void show()
	{
		m_showing = true;
	}
	void hide()
	{
		m_showing = false;
	}

	void a_move(int x, int y) {
		m_sprite.setPosition((float)x, (float)y);
	}

	void a_draw() {
		g_window->draw(m_sprite);
	}

	void move(int x, int y) {
		m_x = x;
		m_y = y;
	}
	void draw() {
		if (false == m_showing) return;
		float rx = (m_x - g_left_x) * TILE_WIDTH + 1;
		float ry = (m_y - g_top_y) * TILE_WIDTH + 1;
		m_sprite.setPosition(rx, ry);
		g_window->draw(m_sprite);
		auto size = m_name.getGlobalBounds();

		//채팅 or 이름 그리기
		if (m_mess_end_time < chrono::system_clock::now()) {
			m_name.setPosition(rx + TILE_WIDTH/2 - size.width / 2, ry - 10);
			g_window->draw(m_name);
		}
		else {
			if(id<MAX_USER)
			{
				chat.setPosition(rx + TILE_WIDTH / 2 - size.width / 2, ry - TILE_WIDTH);
				int text = (int)((m_chat.getString().getSize() - m_name.getString().getSize() - 1));
				if (text < 7)
					text = 7;
				chat.setTextureRect(sf::IntRect(0, 0, (float)text * (TILE_WIDTH / 3), int((float)50 * ((float)TILE_WIDTH / 40))));
				g_window->draw(chat);

				m_chat.setPosition(rx + TILE_WIDTH / 2 - size.width / 2, ry - TILE_WIDTH);
				g_window->draw(m_chat);
			}
			else
			{
				chat.setPosition(rx + TILE_WIDTH / 2 - size.width / 2, ry - 10);
				int text = (int)((m_chat.getString().getSize()));
				chat.setTextureRect(sf::IntRect(0, 0, (float)text * (TILE_WIDTH / 3), int((float)30 * ((float)TILE_WIDTH / 40))));
				g_window->draw(chat);

				m_chat.setPosition(rx + TILE_WIDTH / 2 - size.width / 2, ry - 10);
				g_window->draw(m_chat);
			}
			
			
		}

		if(hp > 0)
		{
			if (id != g_myid)
			{
				m_HP.setTextureRect(sf::IntRect(1, 1, (int)(120 * ((float)hp / _max_hp) * ((float)TILE_WIDTH / 64)), int((float)10 * ((float)TILE_WIDTH / 64))));
				m_HP.setPosition(rx + TILE_WIDTH / 2 - size.width / 2, ry + int(TILE_WIDTH));
				g_window->draw(m_HP);
			}
			else
			{
				sf::Vector2f size((int)(120 * ((float)TILE_WIDTH / 24)), int((float)10 * ((float)TILE_WIDTH / 30)));
				borderShape.setSize(size + sf::Vector2f(2 * borderWidth, 2 * borderWidth));
				borderShape.setPosition(m_HP.getPosition() - sf::Vector2f(borderWidth, borderWidth));
				borderShape.setFillColor(sf::Color::Transparent);
				borderShape.setOutlineThickness(borderWidth);
				borderShape.setOutlineColor(borderColor);
				g_window->draw(borderShape);

				sf::Vector2f size2((int)(120 * ((float)TILE_WIDTH / 24)), int((float)10 * ((float)TILE_WIDTH / 30)));
				borderShape2.setSize(size + sf::Vector2f(2 * borderWidth, 2 * borderWidth));
				borderShape2.setPosition(m_Stamina.getPosition() - sf::Vector2f(borderWidth, borderWidth));
				borderShape2.setFillColor(sf::Color::Transparent);
				borderShape2.setOutlineThickness(borderWidth);
				borderShape2.setOutlineColor(borderColor);
				g_window->draw(borderShape2);

				m_Stamina.setTextureRect(sf::IntRect(1, 1, (int)(120 * ((float)stamina / max_stamina) * ((float)TILE_WIDTH / 24)), int((float)10 * ((float)TILE_WIDTH / 30))));
				m_Stamina.setPosition(WINDOW_WIDTH / 2  + TILE_WIDTH , int(WINDOW_HEIGHT) - TILE_WIDTH * 2);
				g_window->draw(m_Stamina);

				m_HP.setTextureRect(sf::IntRect(1, 1, (int)(120 * ((float)hp / _max_hp) * ((float)TILE_WIDTH / 24)), int((float)10 * ((float)TILE_WIDTH / 30))));
				m_HP.setPosition(WINDOW_WIDTH / 2 - TILE_WIDTH * 6, int(WINDOW_HEIGHT) - TILE_WIDTH * 2);
				g_window->draw(m_HP);
			}

			
		}
		//
	}
	void attackrangedraw() {
		chrono::system_clock::time_point time = chrono::system_clock::now();
		
		
			for (auto& p : attack_range)
			{
				int a = 150 - (p.second - time).count() / 100000;
				m_attack_range.setColor(sf::Color(255, 0, 0, a ));
				if (p.second > time) {
					float rx = (p.first.first - g_left_x) * TILE_WIDTH + 1;
					float ry = (p.first.second - g_top_y) * TILE_WIDTH + 1;
					m_attack_range.setPosition(rx, ry);
					g_window->draw(m_attack_range);
				}
				else
				{
					attack_range.erase(make_pair(p.first.first, p.first.second));
				}
			}
		
	}
	void set_name(const char str[]) {
		m_name.setFont(g_font);
		m_name.setString(str);
		if (id < MAX_USER) m_name.setFillColor(sf::Color(255, 255, 255));
		else m_name.setFillColor(sf::Color(255, 255, 0));
		m_name.setCharacterSize(TILE_WIDTH / 2);
		m_name.setStyle(sf::Text::Bold);
	}

	void set_chat(const char str[]) {
		int length = std::strlen(str);
		string msg(str, length);
		if(id < MAX_USER)
		{
			string temp_name = m_name.getString();
			/*temp_name.resize(temp_name.size() + 1);
			temp_name[temp_name.size()] = '\n';*/
			temp_name += ":";
			temp_name += "\n";
			msg.insert(0, temp_name);
		}

		m_chat.setFont(g_font);
		m_chat.setString(msg);
		m_chat.setFillColor(sf::Color(255, 255, 255));
		m_chat.setCharacterSize(TILE_WIDTH / 2);
		m_chat.setStyle(sf::Text::Bold);
		
		m_mess_end_time = chrono::system_clock::now() + chrono::seconds(3);
	}
	void set_chat2(string str) {
		string msg = str;
		string temp_name = m_name.getString();
		/*temp_name.resize(temp_name.size() + 1);
		temp_name[temp_name.size()] = '\n';*/
		temp_name += ":";
		temp_name += "\n";
		msg.insert(0, temp_name);

		m_chat.setFont(g_font);
		m_chat.setString(msg);
		m_chat.setFillColor(sf::Color(255, 255, 255));
		m_chat.setCharacterSize(TILE_WIDTH / 2);
		m_chat.setStyle(sf::Text::Bold);
	}

	void set_Sprite_scale(float x, float y)
	{
		m_sprite.setScale(x, y);
	}
};

OBJECT avatar;
unordered_map <int, OBJECT> players;

OBJECT white_tile;
OBJECT black_tile;

sf::Texture* board;
sf::Texture* pieces;

void client_initialize()
{
	board = new sf::Texture;
	pieces = new sf::Texture;
	hpTexture = new sf::Texture;
	clearTexture = new sf::Texture;
	fire = new sf::Texture;
	stone = new sf::Texture;
	wood = new sf::Texture;
	monster1 = new sf::Texture;
	monster2 = new sf::Texture;
	monster3 = new sf::Texture;
	monster4 = new sf::Texture;

	board->loadFromFile("chessmap.bmp");
	pieces->loadFromFile("chess2.png");
	hpTexture->loadFromFile("hp_Image.png");
	fire->loadFromFile("fire.png");
	stone->loadFromFile("stone.png");
	wood->loadFromFile("wood.png");
	monster1->loadFromFile("monster1.png");
	monster2->loadFromFile("monster2.png");
	monster3->loadFromFile("monster3.png");
	monster4->loadFromFile("monster4.png");
	sf::Image image = monster3->copyToImage();

	// 이미지의 크기
	sf::Vector2u imageSize = image.getSize();

	// 흰색 배경을 제거
	sf::Color whiteColor(255, 255, 255);
	for (unsigned int x = 0; x < imageSize.x; ++x)
	{
		for (unsigned int y = 0; y < imageSize.y; ++y)
		{
			sf::Color pixelColor = image.getPixel(x, y);
			if (pixelColor == whiteColor)
			{
				// 흰색 배경을 투명으로 변경
				image.setPixel(x, y, sf::Color::Transparent);
			}
		}
	}
	image.saveToFile("modified_image.png");

	if (false == g_font.loadFromFile("cour.ttf")) {
		cout << "Font Loading Error!\n";
		exit(-1);
	}
	white_tile = OBJECT{ *board, 5, 5, TILE_WIDTH, TILE_WIDTH };
	black_tile = OBJECT{ *board, 69, 5, TILE_WIDTH, TILE_WIDTH };
	avatar = OBJECT{ *pieces, 128, 0, 64, 64 };
	avatar.set_Sprite_scale((float)TILE_WIDTH / 64, (float)TILE_WIDTH / 64);
	avatar.move(4, 4);
	avatar.m_HP.setTexture(*clearTexture);
	avatar.m_HP.setColor(sf::Color(255, 0, 0 ,200));
	chat.setTexture(*clearTexture);
	chat.setColor(sf::Color(25, 25, 25,200));
	m_Stamina.setTexture(*clearTexture);
	m_Stamina.setColor(sf::Color(0,  0, 200, 200));

	{
		m_fire.setTexture(*fire);
		sf::Vector2u originalSize = fire->getSize();
		sf::Vector2f scale(40.f / originalSize.x, 40.f / originalSize.y);
		m_fire.setScale(scale);
	}

	{
		m_Stone.setTexture(*stone);
		sf::Vector2u originalSize = stone->getSize();
		sf::Vector2f scale(40.f / originalSize.x, 40.f / originalSize.y);
		m_Stone.setScale(scale);
	}
	{
		m_Wood.setTexture(*wood);
		sf::Vector2u originalSize = wood->getSize();
		sf::Vector2f scale(40.f / originalSize.x, 40.f / originalSize.y);
		m_Wood.setScale(scale);
	}
	{
		m_attack_range.setTexture(*clearTexture);
		m_attack_range.setColor(sf::Color(255, 0, 0, 100));
		m_attack_range.setTextureRect(sf::IntRect(1, 1, TILE_WIDTH, TILE_WIDTH));
	}
	{
		p_attack_range.setTexture(*clearTexture);
		p_attack_range.setColor(sf::Color(0, 255, 0, 100));
		p_attack_range.setTextureRect(sf::IntRect(1, 1, TILE_WIDTH, TILE_WIDTH));
	}

	{
		m_monster1.setTexture(*monster1);
		sf::Vector2u originalSize = monster1->getSize();
		sf::Vector2f scale(40.f / originalSize.x, 40.f / originalSize.y);
		m_monster1.setColor(sf::Color(255, 0, 0, 255));
		m_monster1.setScale(scale);
	}
	{
		m_monster2.setTexture(*monster2);
		sf::Vector2u originalSize = monster2->getSize();
		sf::Vector2f scale(40.f / originalSize.x, 40.f / originalSize.y);
		m_monster1.setColor(sf::Color(0, 255, 0, 255));
		m_monster2.setScale(scale);
	}
	{
		m_monster3.setTexture(*monster3);
		sf::Vector2u originalSize = monster3->getSize();
		sf::Vector2f scale(40.f / originalSize.x, 40.f / originalSize.y);
		m_monster1.setColor(sf::Color(0, 0, 255, 255));
		m_monster3.setScale(scale);
	}
	{
		m_monster4.setTexture(*monster4);
		sf::Vector2u originalSize = monster4->getSize();
		sf::Vector2f scale(40.f / originalSize.x, 40.f / originalSize.y);
		m_monster1.setColor(sf::Color(255, 255, 0, 255));
		m_monster4.setScale(scale);
	}
	//m_Stone.setTextureRect(sf::IntRect(0,0, TILE_WIDTH, TILE_WIDTH));
}

void client_finish()
{
	players.clear();
	delete board;
	delete pieces;
}

void ProcessPacket(char* ptr)
{
	static bool first_time = true;
	switch (static_cast<char>(ptr[2]))
	{
	case SC_LOGIN_INFO:
	{
		SC_LOGIN_INFO_PACKET* packet = reinterpret_cast<SC_LOGIN_INFO_PACKET*>(ptr);
		g_myid = packet->id;
		avatar.id = g_myid;
		avatar.move(packet->x, packet->y);
		g_left_x = packet->x - SCREEN_WIDTH / 2;
		g_top_y = packet->y - SCREEN_HEIGHT / 2;
		avatar.hp = packet->hp;
		avatar._max_hp = packet->max_hp;
		avatar.show();
		login_success = true;
	}
	break;

	case SC_ADD_OBJECT:
	{
		SC_ADD_OBJECT_PACKET* my_packet = reinterpret_cast<SC_ADD_OBJECT_PACKET*>(ptr);
		int id = my_packet->id;

		if (id == g_myid) {
			avatar.move(my_packet->x, my_packet->y);
			g_left_x = my_packet->x - SCREEN_WIDTH / 2;
			g_top_y = my_packet->y - SCREEN_HEIGHT / 2;
			avatar.show();
		}
		else if (id < MAX_USER) {
			players[id] = OBJECT{ *pieces, 0, 0,  64, 64 };
			players[id].id = id;
			players[id].move(my_packet->x, my_packet->y);
			players[id].set_name(my_packet->name);
			players[id].set_Sprite_scale((float)TILE_WIDTH / 64, (float)TILE_WIDTH / 64);
			players[id].show();
		}
		else {
			if (my_packet->name[0] == 'Q')
				players[id] = OBJECT{ *monster1, 0, 0, 64, 64 };
			else if (my_packet->name[0] == 'W')
				players[id] = OBJECT{ *monster2, 0, 0, 64, 64 };
			else if (my_packet->name[0] == 'E')
				players[id] = OBJECT{ *monster3, 0, 0, 64, 64 }; 
			else if (my_packet->name[0] == 'R')
				players[id] = OBJECT{ *monster4, 0, 0, 64, 64 };
			players[id].id = id;
			players[id].move(my_packet->x, my_packet->y);
			players[id].set_name(my_packet->name);
			
			players[id].set_Sprite_scale((float)TILE_WIDTH / 64, (float)TILE_WIDTH / 64);
			players[id].show();
		}
		break;
	}
	case SC_MOVE_OBJECT:
	{
		SC_MOVE_OBJECT_PACKET* my_packet = reinterpret_cast<SC_MOVE_OBJECT_PACKET*>(ptr);
		int other_id = my_packet->id;
		if (other_id == g_myid) {
			avatar.move(my_packet->x, my_packet->y);
			g_left_x = my_packet->x - SCREEN_WIDTH / 2;
			g_top_y = my_packet->y - SCREEN_HEIGHT / 2;
		}
		else {
			players[other_id].move(my_packet->x, my_packet->y);
		}
		break;
	}

	case SC_REMOVE_OBJECT:
	{
		SC_REMOVE_OBJECT_PACKET* my_packet = reinterpret_cast<SC_REMOVE_OBJECT_PACKET*>(ptr);
		int other_id = my_packet->id;
		if (other_id == g_myid) {
			avatar.hide();
		}
		else {
			players.erase(other_id);
		}
		break;
	}
	case SC_CHAT:
	{
		SC_CHAT_PACKET* my_packet = reinterpret_cast<SC_CHAT_PACKET*>(ptr);
		int other_id = my_packet->id;
		if (other_id == g_myid) {
			avatar.set_chat(my_packet->mess);
		}
		else {
			players[other_id].set_chat(my_packet->mess);
		}

		break;
	}
	case SC_HP_CHANGE:
	{
		SC_HP_CHANGE_PACKET* packet = reinterpret_cast<SC_HP_CHANGE_PACKET*>(ptr);
		
		int other_id = packet->id;
		if (other_id == g_myid) {
			avatar.hp = packet->hp;
			avatar._max_hp = packet->max_hp;
			if (avatar.hp <= 0)
			{
				avatar.hide();
			}
		}
		else
		{
			players[other_id].hp = packet->hp;
			players[other_id]._max_hp = packet->max_hp;
			if (players[other_id].hp <= 0)
			{
				players[other_id].hide();
			}
		}
		break;
	}
	case SC_ATTACK_RANGE:
	{
		SC_ATTACK_RANGE_PACKET* packet = reinterpret_cast<SC_ATTACK_RANGE_PACKET*>(ptr);
		players[packet->id].attack_range.clear();
		std::string range_string(packet->range);
		std::istringstream iss(range_string);
		std::string line;
		while (std::getline(iss, line)) {
			std::istringstream line_stream(line);
			int first, second;
			line_stream >> first >> second;
			players[packet->id].attack_range[make_pair(first, second)] = (chrono::system_clock::now() + chrono::milliseconds(packet->attack_time));
		}
		break;
	}
	case SC_ATTACK:
	{
		_last_attack_time = chrono::system_clock::now();
		break;
	}
	default:
		printf("Unknown PACKET type [%d]\n", ptr[2]);
	}
}

void process_data(void* net_buf, size_t io_byte)
{
	char* ptr = static_cast<char*>(net_buf);
	unsigned short* ptr_size = reinterpret_cast<unsigned short*>(net_buf);
	static size_t in_packet_size = 0;
	static size_t saved_packet_size = 0;
	static char packet_buffer[BUF_SIZE];

	while (0 != io_byte) {
		if (0 == in_packet_size) 
		{
			in_packet_size = ptr_size[0];
		}
		if (io_byte + saved_packet_size >= in_packet_size) {
			memcpy(packet_buffer + saved_packet_size, ptr, in_packet_size - saved_packet_size);
			ProcessPacket(packet_buffer);
			
			ptr += in_packet_size - saved_packet_size;
			ptr_size = reinterpret_cast<unsigned short*>(ptr);
			io_byte -= in_packet_size - saved_packet_size;
			in_packet_size = 0;
			saved_packet_size = 0;
		}
		else {
			memcpy(packet_buffer + saved_packet_size, ptr, io_byte);
			saved_packet_size += io_byte;
			io_byte = 0;
		}
	}
}

void client_main()
{
	char net_buf[BUF_SIZE];
	size_t	received;

	auto recv_result = s_socket.receive(net_buf, BUF_SIZE, received);
	if (recv_result == sf::Socket::Error)
	{
		wcout << L"Recv 에러!";
		exit(-1);
	}
	if (recv_result == sf::Socket::Disconnected) {
		wcout << L"Disconnected\n";
		exit(-1);
	}
	if (recv_result != sf::Socket::NotReady)
		if (received > 0) process_data(net_buf, received);
	g_window->clear(sf::Color::Black);
	for (int i = 0; i < SCREEN_WIDTH; ++i)
		for (int j = 0; j < SCREEN_HEIGHT; ++j)
		{
			int tile_x = i + g_left_x;
			int tile_y = j + g_top_y;
			if ((tile_x < 0) || (tile_y < 0)) continue;
			std::pair<short, short> key(tile_x, tile_y);
			
			{
				if (0 == (tile_x / 3 + tile_y / 3) % 2) {
					white_tile.a_move(TILE_WIDTH * i, TILE_WIDTH * j);
					white_tile.a_draw();
				}
				else
				{
					black_tile.a_move(TILE_WIDTH * i, TILE_WIDTH * j);
					black_tile.a_draw();
				}
			}
			if (World_Map.find(key) != World_Map.end())
			{
				if (World_Map[key] == 1)
				{
					m_fire.setPosition((float)TILE_WIDTH * i, (float)TILE_WIDTH * j);
					g_window->draw(m_fire);
				}
				else if (World_Map[key] == 2 || World_Map[key] == 3)
				{
					m_Stone.setPosition((float)TILE_WIDTH * i, (float)TILE_WIDTH * j);
					g_window->draw(m_Stone);
				}
				else if(World_Map[key] == 4 || World_Map[key] == 5)
				{
					m_Wood.setPosition((float)TILE_WIDTH * i, (float)TILE_WIDTH * j);
					g_window->draw(m_Wood);
				}
			}
		}
	for (auto& pl : players) pl.second.attackrangedraw();
	if (_last_attack_time > chrono::system_clock::now() - chrono::milliseconds(200))
	{
		for (int i = -1; i <= 1; ++i)
		{
			for (int j = -1; j <= 1; ++j)
			{
				float rx = ((avatar.m_x - i) - g_left_x) * TILE_WIDTH + 1;
				float ry = ((avatar.m_y - j) - g_top_y) * TILE_WIDTH + 1;
				p_attack_range.setPosition(rx, ry);

				g_window->draw(p_attack_range);
			}
		}
	}
	for (auto& pl : players) pl.second.draw();
	sf::Text text;
	text.setFont(g_font);
	char buf[200];
	sprintf_s(buf, "(%d, %d)", avatar.m_x, avatar.m_y);
	text.setString(buf);
	avatar.draw();
	g_window->draw(text);

}

void send_packet(void* packet)
{
	unsigned short* p = reinterpret_cast<unsigned short*>(packet);

	size_t sent = 0;
	s_socket.send(packet, p[0], sent);
}

int main()
{
	std::ifstream file("../World_Map.txt");
	if (file.is_open()) {
		std::string line;
		while (std::getline(file, line)) {
			std::istringstream iss(line);
			short x, y, type;
			if (iss >> x >> y >> type) {
				World_Map[std::make_pair(x, y)] = type;
			}
		}
	}
	file.close();
	wcout.imbue(locale("korean"));
	sf::Socket::Status status = s_socket.connect("127.0.0.1", PORT_NUM);
	s_socket.setBlocking(false);

	if (status != sf::Socket::Done) {
		wcout << L"서버와 연결할 수 없습니다.\n";
		exit(-1);
	}

	client_initialize();
	CS_LOGIN_PACKET p;
	p.size = sizeof(p);
	p.type = CS_LOGIN;


	cout << "insert my id (only integer) : ";
	cin >> g_myid;
	string player_name{ "P" };
	player_name += to_string(g_myid);

	strcpy_s(p.name, player_name.c_str());
	send_packet(&p);
	avatar.set_name(p.name);

	sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "2D CLIENT");
	g_window = &window;
	
	std::string msg;
	window.setKeyRepeatEnabled(false);
	int KeySize = 0;
	while (window.isOpen())
	{
		sf::Event event;
		if (isEnter)
			avatar.m_mess_end_time = chrono::system_clock::now() + chrono::milliseconds(50);
		while (window.pollEvent(event))
		{
			
			if (event.type == sf::Event::Closed)
				window.close();
			
			if(!isEnter)
			{
				if (event.type == sf::Event::KeyPressed) {
					int direction = -1;
					bool attack = false;
					switch (event.key.code) {
					case sf::Keyboard::Left:
						direction = 2;
						break;
					case sf::Keyboard::Right:
						direction = 3;
						break;
					case sf::Keyboard::Up:
						direction = 0;
						break;
					case sf::Keyboard::Down:
						direction = 1;
						break;
					case  sf::Keyboard::A:
						attack = true;
						break;
					case sf::Keyboard::Escape:
						window.close();
						break;
					case sf::Keyboard::Enter:
						isEnter = true;
						break;
					}
					if (-1 != direction) {
						CS_MOVE_PACKET p;
						p.size = sizeof(p);
						p.type = CS_MOVE;
						p.direction = direction;
						send_packet(&p);
					}
					else if (attack)
					{
						if (_last_attack_time < chrono::system_clock::now() - chrono::milliseconds(800))//무한 send 방지
						{
							CS_ATTACK_PACKET p;
							p.size = sizeof(p);
							p.type = CS_ATTACK;
							send_packet(&p);
						}
					}

				}
			}
			else
			{
				KeySize++;
				if (event.type == sf::Event::TextEntered && KeySize > 1)
				{
					if (event.text.unicode == '\r')
					{
						if(msg.size() != 0)
						{
							msg += '\0';
							CS_CHAT_PACKET packet;
							strcpy_s(packet.mess, msg.c_str());
							packet.size = sizeof(CS_CHAT_PACKET) - (CHAT_SIZE - msg.size());
							packet.type = CS_CHAT;
							send_packet(&packet);
						}
						msg.clear();
						KeySize = 0;
						isEnter = false;
					}
					else if (event.text.unicode == '\b')
					{
						if (!msg.empty())
						{
							msg.pop_back();
						}
					}
					else if (event.text.unicode < 128)
					{
						msg += static_cast<char>(event.text.unicode);
					}
					avatar.set_chat2(msg);
				}
				avatar.set_chat2(msg);
				
			}

		}

		window.clear();
		client_main();
		window.display();
	}
	client_finish();

	return 0;
}