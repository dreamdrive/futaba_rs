//#include "stdafx.h"
#include <string.h>
#include "sv_motor.h"

// サーボの動作
void SVMotor::sv_move(int id, short sPos, unsigned short sTime) {
	
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

	send((char *)sendbuf, 12);

}

// サーボIDとトルクのオンオフ
void SVMotor::sv_torque(int id, int torque) {

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

	send((char *)sendbuf, 9);

}

sv_r SVMotor::sv_read(int id)
{
	unsigned char	sendbuf[32];
	unsigned char	readbuf[128];
	unsigned char	sum;
	int				i;
	sv_r rDATA;

	// バッファクリア
	memset(sendbuf, 0x00, sizeof(sendbuf));

	// パケット作成
	sendbuf[0] = (unsigned char)0xFA;				// ヘッダー1
	sendbuf[1] = (unsigned char)0xAF;				// ヘッダー2
	sendbuf[2] = (unsigned char)id;					// サーボID
	sendbuf[3] = (unsigned char)0x09;				// フラグ(0x01 | 0x04<<1)
	sendbuf[4] = (unsigned char)0x00;				// アドレス(0x00)
	sendbuf[5] = (unsigned char)0x00;				// 長さ(0byte)
	sendbuf[6] = (unsigned char)0x01;				// 個数
													// チェックサムの計算
	sum = sendbuf[2];
	for (i = 3; i < 7; i++) {
		sum = (unsigned char)(sum ^ sendbuf[i]);
	}
	sendbuf[7] = sum;								// チェックサム


	send((char *)sendbuf, 8);


	// 読み込みバッファクリア
	memset(readbuf, 0x00, sizeof(readbuf));

	// 受信！
	receive((char *)readbuf, 26);

	// 受信データの確認
	sum = readbuf[2];
	for (i = 3; i < 26; i++) {
		sum = sum ^ readbuf[i];
	}

	if (sum) {
		// チェックサムエラー
		rDATA.error = -3;
		return rDATA;
	}

	rDATA.angle = ((readbuf[8] << 8) & 0x0000FF00) | (readbuf[7] & 0x000000FF);
	rDATA.time = ((readbuf[10] << 8) & 0x0000FF00) | (readbuf[9] & 0x000000FF);
	rDATA.speed = ((readbuf[12] << 8) & 0x0000FF00) | (readbuf[11] & 0x000000FF);
	rDATA.load = ((readbuf[14] << 8) & 0x0000FF00) | (readbuf[13] & 0x000000FF);
	rDATA.temperature = ((readbuf[16] << 8) & 0x0000FF00) | (readbuf[15] & 0x000000FF);

	rDATA.error = 0;	// エラーないよ

	

	return rDATA;
}


sv_r SVMotor::sv_read2(int id)
{
	unsigned char	sendbuf[32];
	unsigned char	readbuf[128];
	unsigned char	sum;
	int				i;
	sv_r rDATA;

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


	send((char *)sendbuf, 8);


	// 読み込みバッファクリア
	memset(readbuf, 0x00, sizeof(readbuf));

	// 受信！
	receive((char *)readbuf, 18);

	// 受信データの確認
	sum = readbuf[2];
	for (i = 3; i < 18; i++) {
		sum = sum ^ readbuf[i];
	}


	if (sum) {
		// チェックサムエラー
		rDATA.error = -3;
		return rDATA;
	}

	rDATA.angle = ((readbuf[8] << 8) & 0x0000FF00) | (readbuf[7] & 0x000000FF);
	rDATA.time = ((readbuf[10] << 8) & 0x0000FF00) | (readbuf[9] & 0x000000FF);
	rDATA.speed = ((readbuf[12] << 8) & 0x0000FF00) | (readbuf[11] & 0x000000FF);
	rDATA.load = ((readbuf[14] << 8) & 0x0000FF00) | (readbuf[13] & 0x000000FF);
	rDATA.temperature = ((readbuf[16] << 8) & 0x0000FF00) | (readbuf[15] & 0x000000FF);

	rDATA.error = 0;	// エラーないよ

	return rDATA;
}


void SVMotor::sv_move_long(sv_r sendDATA[100]) {

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
	sendbuf[6] = (unsigned char)0x16;				    // 個数22個(0x16=22)

	j = 7;

	for (i = 61; i < 69; i++) {

		sendbuf[j] = i;															// ID

		sendbuf[j + 1] = (unsigned char)(sendDATA[i].g_angle & 0x00FF);			// 位置
		sendbuf[j + 2] = (unsigned char)((sendDATA[i].g_angle & 0xFF00) >> 8);	// 位置
		
		sendbuf[j + 3] = (unsigned char)(sendDATA[i].g_time & 0x00FF);			// 速度
		sendbuf[j + 4] = (unsigned char)((sendDATA[i].g_time & 0xFF00) >> 8);	// 速度

		j = j + 5;
	}

	for (i = 71; i < 79; i++) {

		sendbuf[j] = i;															// ID

		sendbuf[j + 1] = (unsigned char)(sendDATA[i].g_angle & 0x00FF);			// 位置
		sendbuf[j + 2] = (unsigned char)((sendDATA[i].g_angle & 0xFF00) >> 8);	// 位置

		sendbuf[j + 3] = (unsigned char)(sendDATA[i].g_time & 0x00FF);			// 速度
		sendbuf[j + 4] = (unsigned char)((sendDATA[i].g_time & 0xFF00) >> 8);	// 速度

		j = j + 5;
	}
	for (i = 81; i < 84; i++) {

		sendbuf[j] = i;															// ID

		sendbuf[j + 1] = (unsigned char)(sendDATA[i].g_angle & 0x00FF);			// 位置
		sendbuf[j + 2] = (unsigned char)((sendDATA[i].g_angle & 0xFF00) >> 8);	// 位置

		sendbuf[j + 3] = (unsigned char)(sendDATA[i].g_time & 0x00FF);			// 速度
		sendbuf[j + 4] = (unsigned char)((sendDATA[i].g_time & 0xFF00) >> 8);	// 速度

		j = j + 5;
	}

	for (i = 91; i < 94; i++) {

		sendbuf[j] = i;															// ID

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
	send((char *)sendbuf, j);						// 送信

}

unsigned char SVMotor::sv_read_Responsetime(int id)
{
	unsigned char	sendbuf[32];
	unsigned char	readbuf[128];
	unsigned char	sum;
	int				i;
//	sv_r rDATA;

	// バッファクリア
	memset(sendbuf, 0x00, sizeof(sendbuf));

	// パケット作成
	sendbuf[0] = (unsigned char)0xFA;				// ヘッダー1
	sendbuf[1] = (unsigned char)0xAF;				// ヘッダー2
	sendbuf[2] = (unsigned char)id;					// サーボID

	sendbuf[3] = (unsigned char)0x0F;				// フラグ(0x0F) 指定アドレスからの指定の長さを返答

	sendbuf[4] = (unsigned char)0x07;				// アドレス(0x07) 返信遅延時間
	sendbuf[5] = (unsigned char)0x01;				// 長さ(1byte)
	sendbuf[6] = (unsigned char)0x00;				// 個数 (任意アドレスリターンの場合はcnt=0x00)
													// チェックサムの計算
	sum = sendbuf[2];
	for (i = 3; i < 7; i++) {
		sum = (unsigned char)(sum ^ sendbuf[i]);
	}
	sendbuf[7] = sum;								// チェックサム


	send((char *)sendbuf, 8);



	// 読み込みバッファクリア
	memset(readbuf, 0x00, sizeof(readbuf));

	// 受信！
	receive((char *)readbuf, 9);

	// 受信データの確認
	sum = readbuf[2];
	for (i = 3; i < 9; i++) {
		sum = sum ^ readbuf[i];
	}

	if (sum) {
		// チェックサムエラー
		return -3;
	}

	return readbuf[7];
}






void SVMotor::sv_readFF(sv_r *rDATA)
{
	unsigned char	sendbuf[32];
	unsigned char	readbuf[128];
	unsigned char	sum;
	int				i = 0,j = 0;
	int				sv_no = 0;

	// バッファクリア
	memset(sendbuf, 0x00, sizeof(sendbuf));

	// パケット作成
	sendbuf[0] = (unsigned char)0xFA;				// ヘッダー1
	sendbuf[1] = (unsigned char)0xAF;				// ヘッダー2
	sendbuf[2] = (unsigned char)0xFF;				// サーボID
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
	
	send((char *)sendbuf, 8);	// 送信
	
	for (j = 0; j < 22; j++) {

		// 読み込みバッファクリア
		memset(readbuf, 0x00, sizeof(readbuf));

		// 受信！						時間がないので フラグチェックしないreceive2で受信
		receive2((char *)readbuf, 18);


		// 受信データの確認
		sv_no = readbuf[2];	// ID 一時保存

		sum   = readbuf[2];
		for (i = 3; i < 18; i++) {
			sum = sum ^ readbuf[i];
		}

		if (sum) {
			// チェックサムエラー
			rDATA[sv_no].error = -3;
			return ;
		}

		rDATA[sv_no].angle = ((readbuf[8] << 8) & 0x0000FF00) | (readbuf[7] & 0x000000FF);
		rDATA[sv_no].time = ((readbuf[10] << 8) & 0x0000FF00) | (readbuf[9] & 0x000000FF);
		rDATA[sv_no].speed = ((readbuf[12] << 8) & 0x0000FF00) | (readbuf[11] & 0x000000FF);
		rDATA[sv_no].load = ((readbuf[14] << 8) & 0x0000FF00) | (readbuf[13] & 0x000000FF);
		rDATA[sv_no].temperature = ((readbuf[16] << 8) & 0x0000FF00) | (readbuf[15] & 0x000000FF);
		rDATA[sv_no].error = 0;	// エラーないよ

	}

}


// サーボのレスポンスタイム設定
void SVMotor::sv_write_Responsetime(int id, unsigned char r_time) {

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
	sendbuf[4] = (unsigned char)0x07;			// アドレス(0x1E=30)
	sendbuf[5] = (unsigned char)0x01;			// 長さ(1byte)
	sendbuf[6] = (unsigned char)0x01;			// 個数
	sendbuf[7] = (unsigned char)r_time;		    // トルク

												// チェックサムの計算
	sum = sendbuf[2];
	for (i = 3; i < 8; i++) {
		sum = (unsigned char)(sum ^ sendbuf[i]);
	}
	sendbuf[8] = sum;								// チェックサム

	send((char *)sendbuf, 9);

}

// フラッシュへの書き込み
void SVMotor::sv_flash_write(int id) {

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
	sendbuf[3] = (unsigned char)0x40;			// フラグ
	sendbuf[4] = (unsigned char)0xFF;			// アドレス(0x1E=30)
	sendbuf[5] = (unsigned char)0x00;			// 長さ(1byte)
	sendbuf[6] = (unsigned char)0x00;			// 個数

												// チェックサムの計算
	sum = sendbuf[2];
	for (i = 3; i < 7; i++) {
		sum = (unsigned char)(sum ^ sendbuf[i]);
	}
	sendbuf[7] = sum;								// チェックサム

	send((char *)sendbuf, 8);

}


int SVMotor::sv_read_torque(int id)
{
	unsigned char	sendbuf[32];
	unsigned char	readbuf[128];
	unsigned char	sum;
	int				i;
	//	sv_r rDATA;

	// バッファクリア
	memset(sendbuf, 0x00, sizeof(sendbuf));

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


	send((char *)sendbuf, 8);



	// 読み込みバッファクリア
	memset(readbuf, 0x00, sizeof(readbuf));

	// 受信！
	receive((char *)readbuf, 9);

	// 受信データの確認
	sum = readbuf[2];
	for (i = 3; i < 9; i++) {
		sum = sum ^ readbuf[i];
	}

	if (sum) {
		// チェックサムエラー
		return -3;
	}

	return readbuf[7];
}

