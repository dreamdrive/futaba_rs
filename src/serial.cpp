//---------------------------------------------------------------------------
//�V���A���ʐM�\����class serial
//serial.cpp,h
//�vwindows.h ->Linux �ɑΉ�
//modified 060530
//http://www.linux.or.jp/JF/JFdocs/Serial-Programming-HOWTO-3.html

//#include "stdafx.h"
#include <string.h>
#define LINUX

#ifdef LINUX
#undef WIN32 
#include <strings.h>//for bzero()
#include <stdlib.h>//for exit
#include <unistd.h>//for read
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#endif

#include <stdio.h>
#include "serial.h"

#ifdef LINUX
  int fd, res;
  struct termios oldtio,newtio;
#endif

//---------------------------------------------------------------------------
bool serial::init(char *comport_in,int baudrate)
{
bool flag=true;

#ifdef WIN32
DCB dcb1;//�V���A���ʐM�̃p�����[�^���Z�b�g����\���́i�E�C���h�E�Y�W���j

sprintf_s(comport,comport_in);

if(flag_opened==1)this->close();
hcom=CreateFile(comport,GENERIC_READ|GENERIC_WRITE,0,
                NULL,OPEN_EXISTING,NULL,NULL);
GetCommState(hcom,&dcb1);//�|�[�g�̏�Ԏ擾

dcb1.BaudRate=baudrate;
//��Windows�R���g���[���p�l�����Őݒ肵��,���̂��Ƃ����Ƀu���[�N�|�C���g�������āC
//�ϐ��̒l������Ƃ킩��

dcb1.fParity=1;//0
dcb1.Parity=NOPARITY;//for AI_Motor
//dcb1.Parity=EVENPARITY;
dcb1.StopBits=ONESTOPBIT;//for AI_Motor
dcb1.ByteSize=8;//for AI Motor
dcb1.fNull=FALSE;//�������Ă����Ȃ��ƃG���[���p�o
//dcb1.EvtChar=STX;
flag=SetCommState(hcom,&dcb1);

    if(flag==true)
    {
     GetCommMask(hcom,&mask);
     //mask=EV_RXCHAR|EV_RXFLAG;
     mask=EV_RXCHAR;
     flag=SetCommMask(hcom,mask);
    }
    if(flag==true)
    {
    GetCommTimeouts(hcom,&ctmo);
    ctmo.ReadIntervalTimeout=1000;//0;//30;//SICKLMS�ł�6ms
    ctmo.ReadTotalTimeoutMultiplier=5000;//=1;//0928
    //ctmo.ReadTotalTimeoutConstant=1000;//=6;//0928
    flag=SetCommTimeouts(hcom,&ctmo);
    }

//�o�b�t�@�C�j�V�����C�Y
PurgeComm(hcom,PURGE_TXABORT|PURGE_RXABORT|PURGE_TXCLEAR|PURGE_RXCLEAR);
//GetCommProperties(hcom,&cmp);
flag_opened=1;
#endif


#ifdef LINUX
 fd = open(comport_in, O_RDWR | O_NOCTTY ); 
// fd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY ); 
 if (fd <0) {
   perror(comport_in); 
   //printf("init() error. %s is not opend. fd=%d\n", comport_in, fd);//debug
   return(false); 
 }
 //printf("init() com opened\n");//debug
 flag_opened=1;

 tcgetattr(fd,&oldtio); /* ���݂̃|�[�g�ݒ��Ҕ� */

 bzero(&newtio, sizeof(newtio));
 
 tcflag_t baud;
 if(baudrate==9600) baud=B9600;
 if(baudrate==38400) baud=B38400;
 if(baudrate==57600) baud=B57600;
 if(baudrate==115200) baud=B115200;
 if(baudrate==230400) baud=B230400;
 if(baudrate==460800) baud=B460800;

  newtio.c_cc[VMIN] = 1;     // �Œ�ꕶ������M    ����������
  newtio.c_cc[VTIME] = 0;   //��������������

  newtio.c_cflag = baud | IGNPAR | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;
	

 /* set input mode (non-canonical, no echo,...) */
 newtio.c_lflag = 0;
#endif

return(flag);
}
//---------------------------------------------------------------------------
void serial::purge(void)
{
//WinAPI��PurgeComm�����s����
  #ifdef WIN32
  PurgeComm(hcom,PURGE_TXABORT|PURGE_RXABORT|PURGE_TXCLEAR|PURGE_RXCLEAR);
  #endif

  #ifdef LINUX
  tcflush(fd, TCIFLUSH);
  #endif

}
//---------------------------------------------------------------------------
bool serial::close(void)
{
  if(flag_opened!=1)return false;
  flag_opened=0;

  #ifdef WIN32
  return CloseHandle(hcom);
  #endif

  #ifdef LINUX
  ::close(fd);
  return true;
  #endif
}
//---------------------------------------------------------------------------
int serial::receive(char *buf_ptr,int size)
{
//�󂯎�����o�C�g������Ԃ�->�ǂ��Ȃ��DEOF�𒴂����Ƃ��ɂ͂܂�
//ReadFile�̕Ԃ�l��true�ŁCbyte��0�̂Ƃ�
//->�t�@�C���|�C���^��EOF�𒴂��Ă��܂����C�Ƃ����Ӗ��炵��
unsigned long byte,event;
byte=0;
bool flag=false;

#ifdef WIN32
    while(!flag)
    {
    //��������M����܂ő҂�
    WaitCommEvent(hcom,&event,NULL);
     if(event|EV_RXCHAR)//��������M�������H
     {
       if(ReadFile(hcom,buf_ptr,size,&byte,NULL))
       flag=true;
	   break;
     }
     byte=0;//������
    }
#endif

#ifdef LINUX
	tcflush(fd, TCIFLUSH);
 tcsetattr(fd,TCSANOW,&newtio);
   byte = read(fd,buf_ptr,size);   /* �҂��������͂��ꂽ��߂� */
#endif

return byte;
}
//---------------------------------------------------------------------------
bool serial::receive2(char *buf_ptr,int size)
{
//�󂯎�����o�C�g������Ԃ�->�ǂ��Ȃ��DEOF�𒴂����Ƃ��ɂ͂܂�
//ReadFile�̕Ԃ�l��true�ŁCbyte��0�̂Ƃ�
//->�t�@�C���|�C���^��EOF�𒴂��Ă��܂����C�Ƃ����Ӗ��炵��
//ACK����M���邽��,���[�v�����܂�܂킳�Ȃ��D

unsigned long byte;
bool flag=false;

#ifdef WIN32
//��������M����܂ő҂�->ACK��M���ɂ͑҂��Ă͂����Ȃ�
//WaitCommEvent(hcom,&event,NULL);
       if(ReadFile(hcom,buf_ptr,size,&byte,NULL))
       flag=true;
#endif

return flag;
}
//---------------------------------------------------------------------------
int serial::send(char *buf_ptr,int size)
{
//�o�b�t�@�̓��e�𑗂�
unsigned long byte;
if(flag_opened!=1)
{
  printf("send() error. port Not opend\n");//debug
  printf("flag_opened=%d\n",flag_opened);//debug
  return -1;
}

#ifdef WIN32
    if(WriteFile(hcom,buf_ptr,size,&byte,NULL))
    //��buffer����ǂݏo����hcom(�ʐM�|�[�g)�֏�������
    {
      return byte;
    }
    else return -1;
#endif

#ifdef LINUX
    byte = write(fd,buf_ptr,size);
    if(byte==0)
      {
	//printf("write error byte 0\n");//debug
	return -1;
      }
    //printf("write byte=%d\n",byte);
    return (byte);
#endif

}
//---------------------------------------------------------------------------

