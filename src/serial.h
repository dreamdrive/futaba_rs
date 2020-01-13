//シリアル通信構造体class serial
//serial.cpp,h
//modified 060530
//windows/linuxクロスプラットフォーム
//---------------------------------------------------------------------------
#ifndef serialH
#define serialH
#ifdef WIN32
  #include <windows.h>
#endif
//---------------------------------------------------------------------------

class serial
{
#ifdef WIN32
    HANDLE hcom;
    DWORD mask;
    COMMTIMEOUTS ctmo;
    OVERLAPPED o;
    COMMPROP cmp;
#endif
public:
    char flag_opened;//comポートが開かれているかどうか
    char comport[16];//comポート名
    int baudrate;//ボーレートをここに出力

    bool init(char *comport_in,int baudrate);
    bool close(void);
    void purge(void);//WinAPIのPurgeCommを実行する
    int receive(char *buf_ptr,int size);//受け取るバッファの場所とサイズ
    int send(char *buf_ptr,int size);//送るバッファの場所とサイズ
    bool receive2(char *buf_ptr,int size);//ACKを受け取る関数

};
//---------------------------------------------------------------------------
#endif
