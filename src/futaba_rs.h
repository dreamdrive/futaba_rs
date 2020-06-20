// エラーコード
#define TEMPERATURE_ERROR 0x80		// 温度エラー(トルクオフ)
#define TEMPERATURE_ALARM 0x20		// 温度アラート
#define FLASH_WRITE_ERROR 0x08		// フラッシュ書き込みエラー
#define PROCESSING_ERROR  0x02		// 受信パケット処理エラー
#define TIMEOUT_ERROR     0x0100	// タイムアウトエラー
#define CHECKSUM_ERROR    0x0200	// チェックサムエラー
#define NO_ERROR          0x0000	// エラーなし

// サーボのメモリマップの送受信に使用する構造体の定義
struct frs_t {
	int 			id;				// サーボID

	short			angle;			// 現在の角度 (サーボから受信した値)
	short			time;			// 移動時間 (サーボから受信した値)
	short			speed;			// 角速度 (サーボから受信した値)
	short			load;			// 負荷 (サーボから受信した値)
	short			temperature;	// 温度 (サーボから受信した値)
	char			torque;			// トルクON/OFF (サーボから受信した値)

	short			g_angle;		// 目標角度 (サーボに送信する目標値)
	unsigned short	g_time;			// 目標への移動時間 (サーボに送信する目標値)

	unsigned short	error;			// エラー
};

class futaba_rs
{
	public:

	    char flag_opened;											//comポートが開かれているかどうか

		// 初期化と終了
		bool init(const char *comport_in,int baudrate);				// サーボのシリアルの初期化
		bool sv_close(void);										// サーボのシリアルのクローズ

		// メモリマップ書き込み
		ssize_t sv_move(int id, short sPos, unsigned short sTime);	// ショートパケットでサーボを１個を動作させる
		ssize_t sv_move_long(frs_t sendDATA[],unsigned char svcnt);	// ロングパケットで全サーボを動作させる
		ssize_t sv_torque(int id, int torque);						// トルクON/OFFの指示

		// メモリマップ読み出し
		int sv_read_torque(int id);									// トルクON/OFFの読み取り
		frs_t sv_read(int id);										// サーボの情報をメモリマップから読み出す
		frs_t sv_read2(int id);										// サーボの情報をメモリマップから読み出す(sv_readよりちょっと高速)

	private:

		double  packet_start_time_;
  		double  packet_timeout_;
  		double  tx_time_per_byte_;
		int servo_fd_;

		// シリアルポート制御
		int flushPort();
		int readPort(u_int8_t *packet, int length);
		int writePort(u_int8_t *packet, int length);
		void setPacketTimeout(u_int16_t packet_length);
		bool isPacketTimeout();
		double getCurrentTime();
		double getTimeSinceStart();
};