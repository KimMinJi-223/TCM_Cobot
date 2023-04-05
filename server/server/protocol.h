#pragma once

constexpr int PORT_NUM = 7000;
constexpr int BUF_SIZE = 2048;

constexpr int MAX_USER = 10;
constexpr int MAX_LOGIN_LEN = 15;

enum class packet_type { cs_login, sc_login, cs_enter, sc_add_player, cs_move, sc_move, sc_next_light, cs_logout };
enum class direction { up, down, left, right };
enum class synch_type { button1, button2, button3 };

struct vector_d3 {
	double x;
	double y;
	double z;

public:
	vector_d3() : x(-1.f), y(-1.f), z(-1.f)
	{}

	vector_d3(double x, double y, double z)
	{
		this->x = x;
		this->y = y;
		this->z = z;
	}
};

struct cs_login_packet {
	char	size;
	char	type;
	wchar_t id[MAX_LOGIN_LEN];
	wchar_t passward[MAX_LOGIN_LEN];
};

struct sc_login_packet {
	char	size;
	char	type;
	int		id;
	double	x;
	double	y;
	double	z;
	double	yaw;
	double	tm_x;
	double	tm_y;
	double	tm_z;
	double	tm_yaw;
};

struct cs_enter_packet {
	char size;
	char type;
};

struct cs_move_packet {
	char		size;
	char		type;
	float		time;
	direction	direction;
	double		yaw;
	vector_d3	location;
	vector_d3	current;
};

//struct cs_left_move_packet {
//	char size;
//	char type;
//	double current_left_x;
//	double current_left_y;
//	double current_left_z;
//	double location_x;
//	double location_y;
//	double location_z;
//	float time_left;
//};

struct sc_move_packet {
	char		size;
	char		type;
	int			client_id;
	float		time;
	direction	direction;
	double		yaw;
	vector_d3	location;
	vector_d3	current;
};

//struct sc_move_packet {
//	char	size;
//	char	type;
//	int		client_id;
//	double	x;
//	double	y;
//	double	z;
//	double	yaw;
//	unsigned int move_time;
//};

struct sc_synch_packet {
	char	size;
	char	type;
	char	synch_type;
};

struct cs_logout_packet {
	char size;
	char type;
};