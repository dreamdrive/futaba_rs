
#include "futaba_rs_controller/futaba_rs.h"

#define LATENCY_TIMER 16

// サーボの動作
ssize_t futaba_rs::sv_move(int id, short sPos, unsigned short sTime) {
	
	unsigned char	sendbuf[28];
	unsigned char	sum;
	int				i;

	// バッファクリア
	memset(sendbuf, 0x00, sizeof(sendbuf));

	// パケット作成
	sendbuf[0] = (unsigned char)0xFA;						// ヘッダー1
	sendbuf[1] = (unsigned char)0xAF;						// ヘッダー2
	sendbuf[2] = (unsigned char)id;							// サーボID
	sendbuf[3] = (unsigned char)0x00;						// フラグ
	sendbuf[4] = (unsigned char)0x1E;						// アドレス(0x1E=30)
	sendbuf[5] = (unsigned char)0x04;						// 長さ(4byte)
	sendbuf[6] = (unsigned char)0x01;						// 個数
	sendbuf[7] = (unsigned char)(sPos & 0x00FF);			// 位置
	sendbuf[8] = (unsigned char)((sPos & 0xFF00) >> 8);		// 位置
	sendbuf[9] = (unsigned char)(sTime & 0x00FF);			// 時間
	sendbuf[10] = (unsigned char)((sTime & 0xFF00) >> 8);	// 時間
															// チェックサムの計算
	sum = sendbuf[2];
	for (i = 3; i < 11; i++) {
		sum = (unsigned char)(sum ^ sendbuf[i]);
	}
	sendbuf[11] = sum;								// チェックサム

	flushPort();				// シリアル通信入力バッファクリア（追加）

	return writePort(sendbuf, 12);	// 送信したバイト数を返す／失敗時は-1を返す
}

// サーボIDとトルクのオンオフ
ssize_t futaba_rs::sv_torque(int id, int torque) {

	unsigned char	sendbuf[28];
	unsigned char	sum;
	int				i;
	short sPos = 0;
	unsigned short sTime = 0;

	// バッファクリア
	memset(sendbuf, 0x00, sizeof(sendbuf));

	// パケット作成
	sendbuf[0] = (unsigned char)0xFA;			// ヘッダー1
	sendbuf[1] = (unsigned char)0xAF;			// ヘッダー2
	sendbuf[2] = (unsigned char)id;				// サーボID
	sendbuf[3] = (unsigned char)0x00;			// フラグ
	sendbuf[4] = (unsigned char)0x24;			// アドレス(0x1E=30)
	sendbuf[5] = (unsigned char)0x01;			// 長さ(1byte)
	sendbuf[6] = (unsigned char)0x01;			// 個数
	sendbuf[7] = (unsigned char)torque;		    // トルク

												// チェックサムの計算
	sum = sendbuf[2];
	for (i = 3; i < 8; i++) {
		sum = (unsigned char)(sum ^ sendbuf[i]);
	}
	sendbuf[8] = sum;								// チェックサム

	flushPort();				// シリアル通信入力バッファクリア（追加）

	return writePort(sendbuf, 9);				// 送信したバイト数を返す／失敗時は-1を返す
}

frs_t futaba_rs::sv_read(int id)
{
	unsigned char	sendbuf[32];
	unsigned char	readbuf[128];
	unsigned char	sum;
	int				i;
	int temp_cnt = 0;
	frs_t rDATA;
	int nread;
	int recv;

	// バッファクリア
	memset(sendbuf, 0x00, sizeof(sendbuf));

	// パケット作成
	sendbuf[0] = (unsigned char)0xFA;				// ヘッダー1
	sendbuf[1] = (unsigned char)0xAF;				// ヘッダー2
	sendbuf[2] = (unsigned char)id;					// サーボID
	sendbuf[3] = (unsigned char)0x09;				// フラグ(0x01 | 0x04<<1)	No.42〜59を返す
	sendbuf[4] = (unsigned char)0x00;				// アドレス(0x00)
	sendbuf[5] = (unsigned char)0x00;				// 長さ(0byte)
	sendbuf[6] = (unsigned char)0x01;				// 個数
													// チェックサムの計算
	sum = sendbuf[2];
	for (i = 3; i < 7; i++) {
		sum = (unsigned char)(sum ^ sendbuf[i]);
	}
	sendbuf[7] = sum;								// チェックサム

	flushPort();						// シリアル通信入力バッファクリア（追加）

	writePort(sendbuf, 8);				// 送信したバイト数を返す

	if (readPort(readbuf, 26) != 26){
			printf("packet time out\n");
			rDATA.error = TIMEOUT_ERROR;
			return rDATA;
	}

	// 受信データの確認
	sum = readbuf[2];
	for (i = 3; i < 26; i++) {
		sum = sum ^ readbuf[i];
	}

	if (sum) {
		// チェックサムエラー
		rDATA.error = CHECKSUM_ERROR;
		return rDATA;
	}

	rDATA.angle = ((readbuf[8] << 8) & 0x0000FF00) | (readbuf[7] & 0x000000FF);
	rDATA.time = ((readbuf[10] << 8) & 0x0000FF00) | (readbuf[9] & 0x000000FF);
	rDATA.speed = ((readbuf[12] << 8) & 0x0000FF00) | (readbuf[11] & 0x000000FF);
	rDATA.load = ((readbuf[14] << 8) & 0x0000FF00) | (readbuf[13] & 0x000000FF);
	rDATA.temperature = ((readbuf[16] << 8) & 0x0000FF00) | (readbuf[15] & 0x000000FF);

	// デバッグ用
	// if (readbuf[3] & TEMPERATURE_ERROR ) printf("TEMPERATURE_ERROR");
	// if (readbuf[3] & TEMPERATURE_ALARM ) printf("TEMPERATURE_ALARM");
	// if (readbuf[3] & FLASH_WRITE_ERROR ) printf("FLASH_WRITE_ERROR");
	// if (readbuf[3] & PROCESSING_ERROR )	printf("PROCESSING_ERROR");

	rDATA.error = NO_ERROR;	// エラーないよ
	if (readbuf[3] != NO_ERROR)	rDATA.error = readbuf[3];

	return rDATA;
}

frs_t futaba_rs::sv_read2(int id)
{
	unsigned char	sendbuf[32];
	unsigned char	readbuf[128];
	unsigned char	sum;
	unsigned char	temp_buf[1];
	int				i;
	frs_t rDATA;
	int nread;
	int recv;

	// バッファクリア
	memset(sendbuf, 0x00, sizeof(sendbuf));

	// パケット作成
	sendbuf[0] = (unsigned char)0xFA;				// ヘッダー1
	sendbuf[1] = (unsigned char)0xAF;				// ヘッダー2
	sendbuf[2] = (unsigned char)id;					// サーボID
	sendbuf[3] = (unsigned char)0x0F;				// フラグ(0x0F) 指定アドレスからの指定の長さを返答
	sendbuf[4] = (unsigned char)0x2A;				// アドレス(0x2A) 現在位置
	sendbuf[5] = (unsigned char)0x0A;				// 長さ(10byte)
	sendbuf[6] = (unsigned char)0x00;				// 個数 (任意アドレスリターンの場合はcnt=0x00)

													// チェックサムの計算
	sum = sendbuf[2];
	for (i = 3; i < 7; i++) {
		sum = (unsigned char)(sum ^ sendbuf[i]);
	}
	sendbuf[7] = sum;								// チェックサム

	flushPort();				// シリアル通信入力バッファクリア（追加）

	writePort(sendbuf, 8);				// 送信したバイト数を返す

	if (readPort(readbuf, 18) != 18){
			printf("packet time out\n");
			rDATA.error = TIMEOUT_ERROR;
			return rDATA;
	}

	// 受信データの確認
	sum = readbuf[2];
	for (i = 3; i < 18; i++) {
		sum = sum ^ readbuf[i];
	}
	if (sum) {			// チェックサムエラー
		rDATA.error = CHECKSUM_ERROR;
		return rDATA;
	}

	rDATA.angle = ((readbuf[8] << 8) & 0x0000FF00) | (readbuf[7] & 0x000000FF);
	rDATA.time = ((readbuf[10] << 8) & 0x0000FF00) | (readbuf[9] & 0x000000FF);
	rDATA.speed = ((readbuf[12] << 8) & 0x0000FF00) | (readbuf[11] & 0x000000FF);
	rDATA.load = ((readbuf[14] << 8) & 0x0000FF00) | (readbuf[13] & 0x000000FF);
	rDATA.temperature = ((readbuf[16] << 8) & 0x0000FF00) | (readbuf[15] & 0x000000FF);

	// デバッグ用
	// if (readbuf[3] & TEMPERATURE_ERROR ) printf("TEMPERATURE_ERROR");
	// if (readbuf[3] & TEMPERATURE_ALARM ) printf("TEMPERATURE_ALARM");
	// if (readbuf[3] & FLASH_WRITE_ERROR ) printf("FLASH_WRITE_ERROR");
	// if (readbuf[3] & PROCESSING_ERROR )	printf("PROCESSING_ERROR");

	rDATA.error = NO_ERROR;	// エラーないよ
	if (readbuf[3] != NO_ERROR)	rDATA.error = readbuf[3];

	return rDATA;
}


ssize_t futaba_rs::sv_move_long(frs_t sendDATA[],unsigned char svcnt) {

	unsigned char	sendbuf[200];
	unsigned char	sum;
	int				i,j;

	// バッファクリア
	memset(sendbuf, 0x00, sizeof(sendbuf));

	// パケット作成
	sendbuf[0] = (unsigned char)0xFA;				    // ヘッダー1
	sendbuf[1] = (unsigned char)0xAF;				    // ヘッダー2
	sendbuf[2] = (unsigned char)0x00;				    // ID
	sendbuf[3] = (unsigned char)0x00;				    // フラグ
	sendbuf[4] = (unsigned char)0x1E;				    // アドレス(0x1E=30)
	sendbuf[5] = (unsigned char)0x05;				    // 長さ(5byte)
	sendbuf[6] = svcnt;									// サーボの個数

	j = 7;

	for (i = 0; i < svcnt; i++) {

		sendbuf[j] = (unsigned char)sendDATA[i].id;								// ID

		sendbuf[j + 1] = (unsigned char)(sendDATA[i].g_angle & 0x00FF);			// 位置
		sendbuf[j + 2] = (unsigned char)((sendDATA[i].g_angle & 0xFF00) >> 8);	// 位置

		sendbuf[j + 3] = (unsigned char)(sendDATA[i].g_time & 0x00FF);			// 速度
		sendbuf[j + 4] = (unsigned char)((sendDATA[i].g_time & 0xFF00) >> 8);	// 速度

		j = j + 5;
	}
													// チェックサムの計算
	sum = sendbuf[2];
	for (i = 3; i < j; i++) {
		sum = (unsigned char)(sum ^ sendbuf[i]);
	}
	sendbuf[j] = sum;								// チェックサム
	j++;

	flushPort();									// シリアル通信入力バッファクリア（追加）
	return writePort(sendbuf, j);					// 送信したバイト数を返す／失敗時は-1を返す
}

int futaba_rs::sv_read_torque(int id)
{
	unsigned char	sendbuf[32];
	unsigned char	readbuf[128];
	unsigned char	sum;
	int	i;

	// バッファクリア
	memset(sendbuf, 0x00, sizeof(sendbuf));
	memset(readbuf, 0x00, sizeof(readbuf));

	// パケット作成
	sendbuf[0] = (unsigned char)0xFA;				// ヘッダー1
	sendbuf[1] = (unsigned char)0xAF;				// ヘッダー2
	sendbuf[2] = (unsigned char)id;					// サーボID

	sendbuf[3] = (unsigned char)0x0F;				// フラグ(0x0F) 指定アドレスからの指定の長さを返答

	sendbuf[4] = (unsigned char)0x24;				// アドレス(0x07) 返信遅延時間
	sendbuf[5] = (unsigned char)0x01;				// 長さ(1byte)
	sendbuf[6] = (unsigned char)0x00;				// 個数 (任意アドレスリターンの場合はcnt=0x00)
													// チェックサムの計算
	sum = sendbuf[2];
	for (i = 3; i < 7; i++) {
		sum = (unsigned char)(sum ^ sendbuf[i]);
	}
	sendbuf[7] = sum;								// チェックサム

	flushPort();									// シリアル通信入力バッファクリア（追加）
	writePort(sendbuf, 8);				// 送信したバイト数を返す

	if (readPort(readbuf, 9) != 9){
			printf("packet time out\n");
			return -98;
	}

	// 受信データの確認
	sum = readbuf[2];
	for (i = 3; i < 9; i++) {
		sum = sum ^ readbuf[i];
	}
	if (sum) {	// チェックサムエラー
		return -99;
	}

	return readbuf[7];
}

//サーボモーターとのシリアルポートで接続する
bool futaba_rs::init(const char *comport_in,int baudrate)
{
	// // https://mcommit.hatenadiary.com/entry/2017/07/09/210840
	// // こちら参照コード
	
	int error_flag = 0;
	struct termios tio;    
    speed_t baud;
    
	if(baudrate==9600) baud=B9600;
    if(baudrate==38400) baud=B38400;
    if(baudrate==57600) baud=B57600;
	if(baudrate==115200) baud=B115200;
    if(baudrate==230400) baud=B230400;
    if(baudrate==460800) baud=B460800;

	bzero(&tio, sizeof(tio)); // clear struct for new port settings
	
	servo_fd_ = open(comport_in, O_RDWR|O_NOCTTY|O_NONBLOCK);	// ポートオープンの仕方が重要
     if (servo_fd_ < 0) {
         printf("open error\n");
         return -1;
     }

     tio.c_cflag += CREAD;               // 受信有効
     tio.c_cflag += CLOCAL;              // ローカルライン（モデム制御なし）
     tio.c_cflag += CS8;                 // データビット:8bit
     tio.c_cflag += 0;                   // ストップビット:1bit
     tio.c_cflag += 0;                   // パリティ:None

    cfsetispeed( &tio, baud );
    cfsetospeed( &tio, baud );

	tio.c_oflag      = 0;
	tio.c_lflag      = 0;
	tio.c_cc[VTIME]  = 0;
	tio.c_cc[VMIN]   = 0;

	cfmakeraw(&tio);                    // RAWモード

 	error_flag = tcsetattr(servo_fd_, TCSANOW, &tio ); // デバイスに設定を行う (成功したら0を返す)
     if (error_flag){ 
 		printf("serial device setting(tcsetattr) error\n");
 		return -1;
 	}

     error_flag = ioctl(servo_fd_, TCSETS, &tio);            // ポートの設定を有効にする
     if (error_flag== -1 ){ 
 		printf("serial device setting(ioctl) error\n");
 		return -1;
 	}

 	error_flag = tcflush(servo_fd_,TCIFLUSH);				// 入力バッファクリア（追加）
    if (error_flag == -1 ){ 
 	 	printf("serial device setting(tcflush) error\n");
 		return -1;
 	}

 	flag_opened = 1;	// フラグを立てる
	tx_time_per_byte_ = (1000.0 / (double)baudrate) * 10.0;

    return 0;
}

//サーボモーターとのシリアルポートを閉じる
bool futaba_rs::sv_close(void)
{
  if(flag_opened != 1) return false;

  flag_opened = 0 ;
  close(servo_fd_);
  return true;
}

// サーボから指定したバイト数のパケットを受信する(タイムアウト機能付き)
int futaba_rs::readPort(u_int8_t *readbuf, int length)
{
	int i;
	unsigned char	temp_buf[1];

	setPacketTimeout(length);		// タイムアウトを初期化

	for(i=0; i<length ;){
		if( read(servo_fd_, temp_buf, 1) == 1 ){		// 1バイト受信したら
			readbuf[i] = temp_buf[0];
			i++;								// 受信したらカウンタを進める
		}

		if (isPacketTimeout()){					// タイムアウト
			return -1;
		}
	}
	return length;
}

// 受信パケットのバッファをクリア
int futaba_rs::flushPort()
{
  return tcflush(servo_fd_,TCIFLUSH);
}

// サーボに指定したバイト数のパケット送信
int futaba_rs::writePort(u_int8_t *packet, int length)
{
  return write(servo_fd_, packet, length);
}

// タイムアウトの時間をセット（通信速度・レイテンシタイマから計算）
// (Dynamixel SDKより拝借)
void futaba_rs::setPacketTimeout(u_int16_t packet_length)
{
  packet_start_time_  = getCurrentTime();
  packet_timeout_     = (tx_time_per_byte_ * (double)packet_length) + (LATENCY_TIMER * 2.0) + 2.0;
}

// タイムアウトしたかどうかのチェック
// (Dynamixel SDKより拝借)
bool futaba_rs::isPacketTimeout()
{
  if(getTimeSinceStart() > packet_timeout_)
  {
    packet_timeout_ = 0;
    return true;
  }
  return false;
}

// 現在時間の取得
// (Dynamixel SDKより拝借)
double futaba_rs::getCurrentTime()
{
	struct timespec tv;
	clock_gettime(CLOCK_REALTIME, &tv);
	return ((double)tv.tv_sec * 1000.0 + (double)tv.tv_nsec * 0.001 * 0.001);
}

// 計測スタートからの時間
// (Dynamixel SDKより拝借)
double futaba_rs::getTimeSinceStart()
{
  double time;

  time = getCurrentTime() - packet_start_time_;
  if(time < 0.0)
    packet_start_time_ = getCurrentTime();

  return time;
}