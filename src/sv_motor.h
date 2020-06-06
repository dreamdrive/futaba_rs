
struct sv_r {
	int 			id;				// サーボID
	int				enable;			// 

	short			angle;			// 現在の角度 (サーボから受信した値)
	short			time;			// 移動時間 (サーボから受信した値)
	short			speed;			// 角速度 (サーボから受信した値)
	short			load;			// 負荷 (サーボから受信した値)
	short			temperature;	// 温度 (サーボから受信した値)
	int				torque;			// トルクON/OFF (サーボから受信した値)

	short			g_angle;		// 目標角度 (サーボに送信する目標値)
	unsigned short	g_time;			// 目標への移動時間 (サーボに送信する目標値)

	int				error;			// エラー
};

class SVMotor
{
	private:
		double  packet_start_time_;
  		double  packet_timeout_;
  		double  tx_time_per_byte;
		int servo_fd;

	public:
	    char flag_opened;			//comポートが開かれているかどうか
    	char comport[16];			//comポート名
    	int baudrate;				//ボーレートをここに出力

		bool init(const char *comport_in,int baudrate);
		bool sv_close(void);

		ssize_t sv_move(int id, short sPos, unsigned short sTime);	// 単発サーボを回転させるコマンド
		ssize_t sv_move_long(sv_r sendDATA[],unsigned char svcnt);

		ssize_t sv_torque(int id, int torque);		// トルクのオンオフ
		int sv_read_torque(int id);

		// sv_readとsv_read2はinput/outputは同じ、パケットが短いのでread2が高速
		sv_r sv_read(int id);
		sv_r sv_read2(int id);

		int flushPort();
		int readPort(u_int8_t *packet, int length);
		int writePort(u_int8_t *packet, int length);
		void setPacketTimeout(u_int16_t packet_length);
		bool isPacketTimeout();
		double getCurrentTime();
		double getTimeSinceStart();
};