//#ifndef SVMOTOR_H
//#define SVMOTOR_H
#include "serial.h"

struct sv_r {
	int 			id;
	int				enable;
	short			angle;
	short			time;
	short			speed;
	short			load;
	short			temperature;
	int				torque;
	int				old_torque;
	int				error;
	short			g_angle;
	short			old_g_angle;
	unsigned short	g_time;
};

class SVMotor : public serial
{
private:

public:

	void sv_move(int id, short sPos, unsigned short sTime);
	void sv_move_long(sv_r sendDATA[100]);

	void sv_torque(int id, int torque);		// トルクのオンオフ
	int sv_read_torque(int id);

	sv_r sv_read(int id);
	sv_r sv_read2(int id);
	void sv_readFF(sv_r *rDATA);

	unsigned char sv_read_Responsetime(int id);
	void sv_write_Responsetime(int id, unsigned char r_time);
	
	void sv_flash_write(int id);
};