
// ROBOT_CTLDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "ROBOT_CTL.h"
#include "ROBOT_CTLDlg.h"
#include "DlgProxy.h"
#include "afxdialogex.h"
#include "serial.h"
#include "sv_motor.h"

#include "TimeLineDialog.h"

#include <time.h>     // for clock()

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//iniファイルから取得するシリアル設定
#define SETFILE _T( "./setting.ini")
char get_port[10];
int get_baud;


sv_r Data[100];		// とりあえずID:0からID:99まで確保(サーボのデータ)

int sv_ctl_f = 0;	// サーボコントロールフラグ

//int gyou = 0;	// 行数

SVMotor SSV;		// サーボ制御用クラス

struct MotionPose {			// ポーズの構造体
	short	time;
	short	angle[100];
	CString pname;
	int status;
};

struct MotionType {			// モーションの構造体(1024ポーズまで)
	short	flame;
	MotionPose pose[1024];
	CString mname;
};

MotionType MotionBuf;	// モーションバッファ


clock_t Processing_time, Processing_time1, Processing_time2, Processing_time3;	// 処理時間の入れ物

							//CSVを保存するときのファイル名
CString filetime;
CString select_filename;

//キャプチャフラグ
int flag_rec = 0;
//再生フラグ
int flag_play = 0;
int flag_motion_play = 0;

int saisei_kaishi_G = 0;

// 関数宣言！
void sv_open(void);
void sv_close(void);
void RSGetDataALL(void);
void RSGetDataALL2(void);
void thread_motor(void);
void thread_rec(void);	//スレッドで生成される関数
void thread_play(void);	//スレッドで生成される関数
void thread_motion_play(void);

// アプリケーションのバージョン情報に使われる CAboutDlg ダイアログ

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// ダイアログ データ
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

// 実装
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CROBOT_CTLDlg ダイアログ


IMPLEMENT_DYNAMIC(CROBOT_CTLDlg, CDialogEx);

CROBOT_CTLDlg::CROBOT_CTLDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_ROBOT_CTL_DIALOG, pParent)
	, com_number(_T(""))
	, com_baudrate(_T(""))
	, com_openclode(_T(""))

	, temp0001(_T(""))
	, flip_radio(0)
	, csvlist(_T(""))
	, current_filename(_T(""))
	, select_move(_T(""))
	, motion_file_select(_T(""))
	, motion_name(_T(""))
	, saisei_kaishi(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_pAutoProxy = NULL;
}

CROBOT_CTLDlg::~CROBOT_CTLDlg()
{
	// このダイアログ用のオートメーション プロキシがある場合は、このダイアログ
	//  へのポインターを NULL に戻します、それによってダイアログが削除されたこと
	//  がわかります。
	if (m_pAutoProxy != NULL)
		m_pAutoProxy->m_pDialog = NULL;
}

void CROBOT_CTLDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT2, com_number);
	DDX_Text(pDX, IDC_EDIT3, com_baudrate);
	DDX_Text(pDX, IDC_EDIT5, com_openclode);

	DDX_Text(pDX, IDC_ANGLE_61, sv_angle_r[61]);
	DDX_Text(pDX, IDC_TIME_61, sv_time_r[61]);
	DDX_Text(pDX, IDC_SPEED_61, sv_speed_r[61]);
	DDX_Text(pDX, IDC_LOAD_61, sv_load_r[61]);
	DDX_Text(pDX, IDC_TEMP_61, sv_temperature_r[61]);

	DDX_Text(pDX, IDC_ANGLE_62, sv_angle_r[62]);
	DDX_Text(pDX, IDC_TIME_62, sv_time_r[62]);
	DDX_Text(pDX, IDC_SPEED_62, sv_speed_r[62]);
	DDX_Text(pDX, IDC_LOAD_62, sv_load_r[62]);
	DDX_Text(pDX, IDC_TEMP_62, sv_temperature_r[62]);

	DDX_Text(pDX, IDC_ANGLE_63, sv_angle_r[63]);
	DDX_Text(pDX, IDC_TIME_63, sv_time_r[63]);
	DDX_Text(pDX, IDC_SPEED_63, sv_speed_r[63]);
	DDX_Text(pDX, IDC_LOAD_63, sv_load_r[63]);
	DDX_Text(pDX, IDC_TEMP_63, sv_temperature_r[63]);

	DDX_Text(pDX, IDC_ANGLE_64, sv_angle_r[64]);
	DDX_Text(pDX, IDC_TIME_64, sv_time_r[64]);
	DDX_Text(pDX, IDC_SPEED_64, sv_speed_r[64]);
	DDX_Text(pDX, IDC_LOAD_64, sv_load_r[64]);
	DDX_Text(pDX, IDC_TEMP_64, sv_temperature_r[64]);

	DDX_Text(pDX, IDC_ANGLE_65, sv_angle_r[65]);
	DDX_Text(pDX, IDC_TIME_65, sv_time_r[65]);
	DDX_Text(pDX, IDC_SPEED_65, sv_speed_r[65]);
	DDX_Text(pDX, IDC_LOAD_65, sv_load_r[65]);
	DDX_Text(pDX, IDC_TEMP_65, sv_temperature_r[65]);

	DDX_Text(pDX, IDC_ANGLE_66, sv_angle_r[66]);
	DDX_Text(pDX, IDC_TIME_66, sv_time_r[66]);
	DDX_Text(pDX, IDC_SPEED_66, sv_speed_r[66]);
	DDX_Text(pDX, IDC_LOAD_66, sv_load_r[66]);
	DDX_Text(pDX, IDC_TEMP_66, sv_temperature_r[66]);

	DDX_Text(pDX, IDC_ANGLE_67, sv_angle_r[67]);
	DDX_Text(pDX, IDC_TIME_67, sv_time_r[67]);
	DDX_Text(pDX, IDC_SPEED_67, sv_speed_r[67]);
	DDX_Text(pDX, IDC_LOAD_67, sv_load_r[67]);
	DDX_Text(pDX, IDC_TEMP_67, sv_temperature_r[67]);

	DDX_Text(pDX, IDC_ANGLE_68, sv_angle_r[68]);
	DDX_Text(pDX, IDC_TIME_68, sv_time_r[68]);
	DDX_Text(pDX, IDC_SPEED_68, sv_speed_r[68]);
	DDX_Text(pDX, IDC_LOAD_68, sv_load_r[68]);
	DDX_Text(pDX, IDC_TEMP_68, sv_temperature_r[68]);


	DDX_Text(pDX, IDC_ANGLE_71, sv_angle_r[71]);
	DDX_Text(pDX, IDC_TIME_71, sv_time_r[71]);
	DDX_Text(pDX, IDC_SPEED_71, sv_speed_r[71]);
	DDX_Text(pDX, IDC_LOAD_71, sv_load_r[71]);
	DDX_Text(pDX, IDC_TEMP_71, sv_temperature_r[71]);

	DDX_Text(pDX, IDC_ANGLE_72, sv_angle_r[72]);
	DDX_Text(pDX, IDC_TIME_72, sv_time_r[72]);
	DDX_Text(pDX, IDC_SPEED_72, sv_speed_r[72]);
	DDX_Text(pDX, IDC_LOAD_72, sv_load_r[72]);
	DDX_Text(pDX, IDC_TEMP_72, sv_temperature_r[72]);

	DDX_Text(pDX, IDC_ANGLE_73, sv_angle_r[73]);
	DDX_Text(pDX, IDC_TIME_73, sv_time_r[73]);
	DDX_Text(pDX, IDC_SPEED_73, sv_speed_r[73]);
	DDX_Text(pDX, IDC_LOAD_73, sv_load_r[73]);
	DDX_Text(pDX, IDC_TEMP_73, sv_temperature_r[73]);

	DDX_Text(pDX, IDC_ANGLE_74, sv_angle_r[74]);
	DDX_Text(pDX, IDC_TIME_74, sv_time_r[74]);
	DDX_Text(pDX, IDC_SPEED_74, sv_speed_r[74]);
	DDX_Text(pDX, IDC_LOAD_74, sv_load_r[74]);
	DDX_Text(pDX, IDC_TEMP_74, sv_temperature_r[74]);

	DDX_Text(pDX, IDC_ANGLE_75, sv_angle_r[75]);
	DDX_Text(pDX, IDC_TIME_75, sv_time_r[75]);
	DDX_Text(pDX, IDC_SPEED_75, sv_speed_r[75]);
	DDX_Text(pDX, IDC_LOAD_75, sv_load_r[75]);
	DDX_Text(pDX, IDC_TEMP_75, sv_temperature_r[75]);

	DDX_Text(pDX, IDC_ANGLE_76, sv_angle_r[76]);
	DDX_Text(pDX, IDC_TIME_76, sv_time_r[76]);
	DDX_Text(pDX, IDC_SPEED_76, sv_speed_r[76]);
	DDX_Text(pDX, IDC_LOAD_76, sv_load_r[76]);
	DDX_Text(pDX, IDC_TEMP_76, sv_temperature_r[76]);

	DDX_Text(pDX, IDC_ANGLE_77, sv_angle_r[77]);
	DDX_Text(pDX, IDC_TIME_77, sv_time_r[77]);
	DDX_Text(pDX, IDC_SPEED_77, sv_speed_r[77]);
	DDX_Text(pDX, IDC_LOAD_77, sv_load_r[77]);
	DDX_Text(pDX, IDC_TEMP_77, sv_temperature_r[77]);

	DDX_Text(pDX, IDC_ANGLE_78, sv_angle_r[78]);
	DDX_Text(pDX, IDC_TIME_78, sv_time_r[78]);
	DDX_Text(pDX, IDC_SPEED_78, sv_speed_r[78]);
	DDX_Text(pDX, IDC_LOAD_78, sv_load_r[78]);
	DDX_Text(pDX, IDC_TEMP_78, sv_temperature_r[78]);


	DDX_Text(pDX, IDC_ANGLE_81, sv_angle_r[81]);
	DDX_Text(pDX, IDC_TIME_81, sv_time_r[81]);
	DDX_Text(pDX, IDC_SPEED_81, sv_speed_r[81]);
	DDX_Text(pDX, IDC_LOAD_81, sv_load_r[81]);
	DDX_Text(pDX, IDC_TEMP_81, sv_temperature_r[81]);

	DDX_Text(pDX, IDC_ANGLE_82, sv_angle_r[82]);
	DDX_Text(pDX, IDC_TIME_82, sv_time_r[82]);
	DDX_Text(pDX, IDC_SPEED_82, sv_speed_r[82]);
	DDX_Text(pDX, IDC_LOAD_82, sv_load_r[82]);
	DDX_Text(pDX, IDC_TEMP_82, sv_temperature_r[82]);

	DDX_Text(pDX, IDC_ANGLE_83, sv_angle_r[83]);
	DDX_Text(pDX, IDC_TIME_83, sv_time_r[83]);
	DDX_Text(pDX, IDC_SPEED_83, sv_speed_r[83]);
	DDX_Text(pDX, IDC_LOAD_83, sv_load_r[83]);
	DDX_Text(pDX, IDC_TEMP_83, sv_temperature_r[83]);

	DDX_Text(pDX, IDC_ANGLE_91, sv_angle_r[91]);
	DDX_Text(pDX, IDC_TIME_91, sv_time_r[91]);
	DDX_Text(pDX, IDC_SPEED_91, sv_speed_r[91]);
	DDX_Text(pDX, IDC_LOAD_91, sv_load_r[91]);
	DDX_Text(pDX, IDC_TEMP_91, sv_temperature_r[91]);

	DDX_Text(pDX, IDC_ANGLE_92, sv_angle_r[92]);
	DDX_Text(pDX, IDC_TIME_92, sv_time_r[92]);
	DDX_Text(pDX, IDC_SPEED_92, sv_speed_r[92]);
	DDX_Text(pDX, IDC_LOAD_92, sv_load_r[92]);
	DDX_Text(pDX, IDC_TEMP_92, sv_temperature_r[92]);

	DDX_Text(pDX, IDC_ANGLE_93, sv_angle_r[93]);
	DDX_Text(pDX, IDC_TIME_93, sv_time_r[93]);
	DDX_Text(pDX, IDC_SPEED_93, sv_speed_r[93]);
	DDX_Text(pDX, IDC_LOAD_93, sv_load_r[93]);
	DDX_Text(pDX, IDC_TEMP_93, sv_temperature_r[93]);


	DDX_Check(pDX, IDC_CHK_TORQUE_61, sv_torque_w[61]);
	DDX_Check(pDX, IDC_CHK_TORQUE_62, sv_torque_w[62]);
	DDX_Check(pDX, IDC_CHK_TORQUE_63, sv_torque_w[63]);
	DDX_Check(pDX, IDC_CHK_TORQUE_64, sv_torque_w[64]);
	DDX_Check(pDX, IDC_CHK_TORQUE_65, sv_torque_w[65]);
	DDX_Check(pDX, IDC_CHK_TORQUE_66, sv_torque_w[66]);
	DDX_Check(pDX, IDC_CHK_TORQUE_67, sv_torque_w[67]);
	DDX_Check(pDX, IDC_CHK_TORQUE_68, sv_torque_w[68]);

	DDX_Check(pDX, IDC_CHK_TORQUE_71, sv_torque_w[71]);
	DDX_Check(pDX, IDC_CHK_TORQUE_72, sv_torque_w[72]);
	DDX_Check(pDX, IDC_CHK_TORQUE_73, sv_torque_w[73]);
	DDX_Check(pDX, IDC_CHK_TORQUE_74, sv_torque_w[74]);
	DDX_Check(pDX, IDC_CHK_TORQUE_75, sv_torque_w[75]);
	DDX_Check(pDX, IDC_CHK_TORQUE_76, sv_torque_w[76]);
	DDX_Check(pDX, IDC_CHK_TORQUE_77, sv_torque_w[77]);
	DDX_Check(pDX, IDC_CHK_TORQUE_78, sv_torque_w[78]);

	DDX_Check(pDX, IDC_CHK_TORQUE_81, sv_torque_w[81]);
	DDX_Check(pDX, IDC_CHK_TORQUE_82, sv_torque_w[82]);
	DDX_Check(pDX, IDC_CHK_TORQUE_83, sv_torque_w[83]);

	DDX_Check(pDX, IDC_CHK_TORQUE_91, sv_torque_w[91]);
	DDX_Check(pDX, IDC_CHK_TORQUE_92, sv_torque_w[92]);
	DDX_Check(pDX, IDC_CHK_TORQUE_93, sv_torque_w[93]);


	DDX_Slider(pDX, IDC_ANGLE_SL61, sv_angle_rw[61]);
	DDX_Slider(pDX, IDC_ANGLE_SL62, sv_angle_rw[62]);
	DDX_Slider(pDX, IDC_ANGLE_SL63, sv_angle_rw[63]);
	DDX_Slider(pDX, IDC_ANGLE_SL64, sv_angle_rw[64]);
	DDX_Slider(pDX, IDC_ANGLE_SL65, sv_angle_rw[65]);
	DDX_Slider(pDX, IDC_ANGLE_SL66, sv_angle_rw[66]);
	DDX_Slider(pDX, IDC_ANGLE_SL67, sv_angle_rw[67]);
	DDX_Slider(pDX, IDC_ANGLE_SL68, sv_angle_rw[68]);

	DDX_Slider(pDX, IDC_ANGLE_SL71, sv_angle_rw[71]);
	DDX_Slider(pDX, IDC_ANGLE_SL72, sv_angle_rw[72]);
	DDX_Slider(pDX, IDC_ANGLE_SL73, sv_angle_rw[73]);
	DDX_Slider(pDX, IDC_ANGLE_SL74, sv_angle_rw[74]);
	DDX_Slider(pDX, IDC_ANGLE_SL75, sv_angle_rw[75]);
	DDX_Slider(pDX, IDC_ANGLE_SL76, sv_angle_rw[76]);
	DDX_Slider(pDX, IDC_ANGLE_SL77, sv_angle_rw[77]);
	DDX_Slider(pDX, IDC_ANGLE_SL78, sv_angle_rw[78]);

	DDX_Slider(pDX, IDC_ANGLE_SL81, sv_angle_rw[81]);
	DDX_Slider(pDX, IDC_ANGLE_SL82, sv_angle_rw[82]);
	DDX_Slider(pDX, IDC_ANGLE_SL83, sv_angle_rw[83]);

	DDX_Slider(pDX, IDC_ANGLE_SL91, sv_angle_rw[91]);
	DDX_Slider(pDX, IDC_ANGLE_SL92, sv_angle_rw[92]);
	DDX_Slider(pDX, IDC_ANGLE_SL93, sv_angle_rw[93]);


	//	DDV_MinMaxInt(pDX, sv_angle_rw[61], -1600, 1600);

	DDX_Text(pDX, IDC_ANGLE_V61, sv_angle_rw_v[61]);
	DDX_Text(pDX, IDC_ANGLE_V62, sv_angle_rw_v[62]);
	DDX_Text(pDX, IDC_ANGLE_V63, sv_angle_rw_v[63]);
	DDX_Text(pDX, IDC_ANGLE_V64, sv_angle_rw_v[64]);
	DDX_Text(pDX, IDC_ANGLE_V65, sv_angle_rw_v[65]);
	DDX_Text(pDX, IDC_ANGLE_V66, sv_angle_rw_v[66]);
	DDX_Text(pDX, IDC_ANGLE_V67, sv_angle_rw_v[67]);
	DDX_Text(pDX, IDC_ANGLE_V68, sv_angle_rw_v[68]);

	DDX_Text(pDX, IDC_ANGLE_V71, sv_angle_rw_v[71]);
	DDX_Text(pDX, IDC_ANGLE_V72, sv_angle_rw_v[72]);
	DDX_Text(pDX, IDC_ANGLE_V73, sv_angle_rw_v[73]);
	DDX_Text(pDX, IDC_ANGLE_V74, sv_angle_rw_v[74]);
	DDX_Text(pDX, IDC_ANGLE_V75, sv_angle_rw_v[75]);
	DDX_Text(pDX, IDC_ANGLE_V76, sv_angle_rw_v[76]);
	DDX_Text(pDX, IDC_ANGLE_V77, sv_angle_rw_v[77]);
	DDX_Text(pDX, IDC_ANGLE_V78, sv_angle_rw_v[78]);

	DDX_Text(pDX, IDC_ANGLE_V81, sv_angle_rw_v[81]);
	DDX_Text(pDX, IDC_ANGLE_V82, sv_angle_rw_v[82]);
	DDX_Text(pDX, IDC_ANGLE_V83, sv_angle_rw_v[83]);

	DDX_Text(pDX, IDC_ANGLE_V91, sv_angle_rw_v[91]);
	DDX_Text(pDX, IDC_ANGLE_V92, sv_angle_rw_v[92]);
	DDX_Text(pDX, IDC_ANGLE_V93, sv_angle_rw_v[93]);

	DDX_Control(pDX, IDC_ANGLE_SL61, sv_angle_sl_ctl[61]);
	DDX_Control(pDX, IDC_ANGLE_SL62, sv_angle_sl_ctl[62]);
	DDX_Control(pDX, IDC_ANGLE_SL63, sv_angle_sl_ctl[63]);
	DDX_Control(pDX, IDC_ANGLE_SL64, sv_angle_sl_ctl[64]);
	DDX_Control(pDX, IDC_ANGLE_SL65, sv_angle_sl_ctl[65]);
	DDX_Control(pDX, IDC_ANGLE_SL66, sv_angle_sl_ctl[66]);
	DDX_Control(pDX, IDC_ANGLE_SL67, sv_angle_sl_ctl[67]);
	DDX_Control(pDX, IDC_ANGLE_SL68, sv_angle_sl_ctl[68]);

	DDX_Control(pDX, IDC_ANGLE_SL71, sv_angle_sl_ctl[71]);
	DDX_Control(pDX, IDC_ANGLE_SL72, sv_angle_sl_ctl[72]);
	DDX_Control(pDX, IDC_ANGLE_SL73, sv_angle_sl_ctl[73]);
	DDX_Control(pDX, IDC_ANGLE_SL74, sv_angle_sl_ctl[74]);
	DDX_Control(pDX, IDC_ANGLE_SL75, sv_angle_sl_ctl[75]);
	DDX_Control(pDX, IDC_ANGLE_SL76, sv_angle_sl_ctl[76]);
	DDX_Control(pDX, IDC_ANGLE_SL77, sv_angle_sl_ctl[77]);
	DDX_Control(pDX, IDC_ANGLE_SL78, sv_angle_sl_ctl[78]);

	DDX_Control(pDX, IDC_ANGLE_SL81, sv_angle_sl_ctl[81]);
	DDX_Control(pDX, IDC_ANGLE_SL82, sv_angle_sl_ctl[82]);
	DDX_Control(pDX, IDC_ANGLE_SL83, sv_angle_sl_ctl[83]);

	DDX_Control(pDX, IDC_ANGLE_SL91, sv_angle_sl_ctl[91]);
	DDX_Control(pDX, IDC_ANGLE_SL92, sv_angle_sl_ctl[92]);
	DDX_Control(pDX, IDC_ANGLE_SL93, sv_angle_sl_ctl[93]);

	DDX_Control(pDX, IDC_SV_OPEN, sv_opne_ctlf);
	DDX_Control(pDX, IDC_SV_CLOSE, sv_close_ctlf);
	DDX_Text(pDX, IDC_TEMP0001, temp0001);
	DDX_Radio(pDX, IDC_RADIO1, flip_radio);
	DDX_LBString(pDX, IDC_LIST1, csvlist);
	DDX_Control(pDX, IDC_LIST1, csv_list);
	DDX_Text(pDX, IDC_EDIT1, current_filename);
	DDX_Text(pDX, IDC_EDIT7, wait_value);
	DDX_Control(pDX, IDC_LIST3, move_list);
	DDX_LBString(pDX, IDC_LIST3, select_move);
	DDX_Control(pDX, IDC_LIST4, motion_file_list);
	DDX_LBString(pDX, IDC_LIST4, motion_file_select);
	DDX_Text(pDX, IDC_EDIT4, motion_name);
	DDX_Text(pDX, IDC_saisei_kaishi, saisei_kaishi);
}

BEGIN_MESSAGE_MAP(CROBOT_CTLDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_CLOSE()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &CROBOT_CTLDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON1, &CROBOT_CTLDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CROBOT_CTLDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_SV_OPEN, &CROBOT_CTLDlg::OnBnClickedSvOpen)
	ON_BN_CLICKED(IDC_SV_CLOSE, &CROBOT_CTLDlg::OnBnClickedSvClose)
	ON_BN_CLICKED(IDC_BUTTON7, &CROBOT_CTLDlg::OnBnClickedButton7)
	ON_BN_CLICKED(IDC_BUTTON6, &CROBOT_CTLDlg::OnBnClickedButton6)
	ON_BN_CLICKED(IDC_BUTTON9, &CROBOT_CTLDlg::OnBnClickedButton9)
	ON_BN_CLICKED(IDC_BUTTON8, &CROBOT_CTLDlg::OnBnClickedButton8)
	ON_BN_CLICKED(IDC_BUTTON10, &CROBOT_CTLDlg::OnBnClickedButton10)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_INVERT_LR, &CROBOT_CTLDlg::OnBnClickedInvertLr)
	ON_BN_CLICKED(IDC_COPY_L2R, &CROBOT_CTLDlg::OnBnClickedCopyL2r)
	ON_BN_CLICKED(IDC_COPY_R2L, &CROBOT_CTLDlg::OnBnClickedCopyR2l)
	ON_BN_CLICKED(IDC_BTN_GetDelay, &CROBOT_CTLDlg::OnBnClickedBtnGetdelay)
	ON_BN_CLICKED(IDC_BTN_Delay0, &CROBOT_CTLDlg::OnBnClickedBtnDelay0)
	ON_BN_CLICKED(IDC_BTN_SetDelay, &CROBOT_CTLDlg::OnBnClickedBtnSetdelay)
	ON_BN_CLICKED(IDC_REC, &CROBOT_CTLDlg::OnBnClickedRec)
	ON_BN_CLICKED(IDC_STOP, &CROBOT_CTLDlg::OnBnClickedStop)
	ON_BN_CLICKED(IDC_PLAY, &CROBOT_CTLDlg::OnBnClickedPlay)
	ON_LBN_SELCHANGE(IDC_LIST1, &CROBOT_CTLDlg::OnLbnSelchangeList1)
	ON_BN_CLICKED(IDC_BUTTON3, &CROBOT_CTLDlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON4, &CROBOT_CTLDlg::OnBnClickedButton4)
	ON_BN_CLICKED(IDC_BUTTON11, &CROBOT_CTLDlg::OnBnClickedButton11)
	ON_BN_CLICKED(IDC_BUTTON5, &CROBOT_CTLDlg::OnBnClickedButton5)
	ON_BN_CLICKED(IDC_BUTTON12, &CROBOT_CTLDlg::OnBnClickedButton12)
	ON_BN_CLICKED(IDC_BUTTON14, &CROBOT_CTLDlg::OnBnClickedButton14)
	ON_BN_CLICKED(IDC_BUTTON13, &CROBOT_CTLDlg::OnBnClickedButton13)
	ON_BN_CLICKED(IDC_BUT_CAP, &CROBOT_CTLDlg::OnBnClickedButCap)
	ON_BN_CLICKED(IDC_BUT_KAKIKAE, &CROBOT_CTLDlg::OnBnClickedButKakikae)
	ON_BN_CLICKED(IDC_BUT_SOUNYU, &CROBOT_CTLDlg::OnBnClickedButSounyu)
	ON_BN_CLICKED(IDC_BUT_SAISEI, &CROBOT_CTLDlg::OnBnClickedButSaisei)
	ON_BN_CLICKED(IDC_BUT_CLEAR, &CROBOT_CTLDlg::OnBnClickedButClear)
	ON_BN_CLICKED(IDC_BUT_DELETE, &CROBOT_CTLDlg::OnBnClickedButDelete)
	ON_BN_CLICKED(IDC_BUT_DO_POSE, &CROBOT_CTLDlg::OnBnClickedButDoPose)
	ON_BN_CLICKED(IDC_MOTION_SAVE, &CROBOT_CTLDlg::OnBnClickedMotionSave)
	ON_BN_CLICKED(IDC_MOTION_LOAD, &CROBOT_CTLDlg::OnBnClickedMotionLoad)
	ON_BN_CLICKED(IDC_BUTTON21, &CROBOT_CTLDlg::OnBnClickedButton21)
	ON_BN_CLICKED(IDC_BUTTON15, &CROBOT_CTLDlg::OnBnClickedButton15)
END_MESSAGE_MAP()


// CROBOT_CTLDlg メッセージ ハンドラー

BOOL CROBOT_CTLDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// "バージョン情報..." メニューをシステム メニューに追加します。

	// IDM_ABOUTBOX は、システム コマンドの範囲内になければなりません。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// このダイアログのアイコンを設定します。アプリケーションのメイン ウィンドウがダイアログでない場合、
	//  Framework は、この設定を自動的に行います。
	SetIcon(m_hIcon, TRUE);			// 大きいアイコンの設定
	SetIcon(m_hIcon, FALSE);		// 小さいアイコンの設定

	// TODO: 初期化をここに追加します。 ☆☆☆☆☆☆☆☆☆☆☆☆☆☆☆☆☆☆☆☆☆☆☆☆☆☆☆☆☆☆☆☆☆☆☆☆☆☆☆☆

	int i;



	UpdateData(TRUE);


	saisei_kaishi = 0;	// モーションの再生開始位置 デフォルト0(頭から)


	//全サーボ動作速度を40msecに設定
	for (i = 0; i < 100; i++) {
		Data[i].g_time = 4;
	}
	
	// スライダの設定用の一時変数最大・最少・デフォルト／名前
	int max, min;
	char part_name[10];


	for (i = 61; i < 69; i++) {
		//////////////////スライダーの範囲を設定する

		sprintf_s(part_name, "SURVO_%d", i);

		min = GetPrivateProfileInt("SURVO_MIN", part_name, -100, SETFILE);
		max = GetPrivateProfileInt("SURVO_MAX", part_name, 100, SETFILE);

		//スライダー設定
		sv_angle_sl_ctl[i].SetRange(min, max);		// 最大・最少
		sv_angle_rw[i] = 50;						// デフォルト値

		//////////////////スライダーの範囲を設定する
	}

	for (i = 71; i < 79; i++) {
		//////////////////スライダーの範囲を設定する

		sprintf_s(part_name, "SURVO_%d", i);

		min = GetPrivateProfileInt("SURVO_MIN", part_name, -100, SETFILE);
		max = GetPrivateProfileInt("SURVO_MAX", part_name, 100, SETFILE);

		//スライダー設定
		sv_angle_sl_ctl[i].SetRange(min, max);		// 最大・最少
		sv_angle_rw[i] = 50;						// デフォルト値

		//////////////////スライダーの範囲を設定する
	}

	for (i = 81; i < 84; i++) {
		//////////////////スライダーの範囲を設定する

		sprintf_s(part_name, "SURVO_%d", i);

		min = GetPrivateProfileInt("SURVO_MIN", part_name, -100, SETFILE);
		max = GetPrivateProfileInt("SURVO_MAX", part_name, 100, SETFILE);

		//スライダー設定
		sv_angle_sl_ctl[i].SetRange(min, max);		// 最大・最少
		sv_angle_rw[i] = 50;						// デフォルト値

		//////////////////スライダーの範囲を設定する
	}

	for (i = 91; i < 94; i++) {
		//////////////////スライダーの範囲を設定する

		sprintf_s(part_name, "SURVO_%d", i);

		min = GetPrivateProfileInt("SURVO_MIN", part_name, -100, SETFILE);
		max = GetPrivateProfileInt("SURVO_MAX", part_name, 100, SETFILE);

		//スライダー設定
		sv_angle_sl_ctl[i].SetRange(min, max);		// 最大・最少
		sv_angle_rw[i] = 50;						// デフォルト値

		//////////////////スライダーの範囲を設定する
	}

	// INIファイルから取得
	GetPrivateProfileString("serial", "COM", "INI ERROR", get_port, sizeof(get_port), SETFILE);
	get_baud = GetPrivateProfileInt("serial", "BAUD", 0, SETFILE);

	com_number = get_port;
	com_baudrate.Format(_T("%d bps"), get_baud);
	com_openclode = "CLOSE";


	// ファイルリスト作成 --------------------------------------------------------------------

	HANDLE hFind;
	WIN32_FIND_DATA fd;

	/* 最初のファイル検索 */ //--------------------------------------RT
	hFind = FindFirstFile("*.RT.csv", &fd);

	/* 検索失敗? */
	if (hFind == INVALID_HANDLE_VALUE) {}
	else {
		do {
			/* 結果の表示 */
			csv_list.AddString(fd.cFileName);
		} while (FindNextFile(hFind, &fd)); //次のファイルを検索
	}

	/* 検索終了 */
	FindClose(hFind);


	/* 最初のファイル検索 */ //--------------------------------------MT
	hFind = FindFirstFile("*.MT.csv", &fd);

	/* 検索失敗? */
	if (hFind == INVALID_HANDLE_VALUE) {}
	else {
		do {
			/* 結果の表示 */
			motion_file_list.AddString(fd.cFileName);
		} while (FindNextFile(hFind, &fd)); //次のファイルを検索
	}

	/* 検索終了 */
	FindClose(hFind);

	// ファイルリスト作成 --------------------------------------------------------------------

	wait_value = 100;	// モーションの待ち時間のデフォルト設定
	MotionBuf.flame = 0;

	
	UpdateData(FALSE);

	

	//表示更新用のタイマーを16msで起動 (タイマーを起動しなければ、設定モードに入れる)
	SetTimer(5678, 16, NULL);

	

	return TRUE;  // フォーカスをコントロールに設定した場合を除き、TRUE を返します。
}

void CROBOT_CTLDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// ダイアログに最小化ボタンを追加する場合、アイコンを描画するための
//  下のコードが必要です。ドキュメント/ビュー モデルを使う MFC アプリケーションの場合、
//  これは、Framework によって自動的に設定されます。

void CROBOT_CTLDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 描画のデバイス コンテキスト

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// クライアントの四角形領域内の中央
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// アイコンの描画
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// ユーザーが最小化したウィンドウをドラッグしているときに表示するカーソルを取得するために、
//  システムがこの関数を呼び出します。
HCURSOR CROBOT_CTLDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// コントローラーがオブジェクトの 1 つをまだ保持している場合、
//  オートメーションサーバーはユーザーが UI を閉じる際に終了で
//  きません。これらのメッセージ ハンドラーはプロキシがまだ使用中
//  かどうかを確認し、それから UI が非表示になりますがダイアロ
//  グはそれが消された場合その場所に残ります。

void CROBOT_CTLDlg::OnClose()
{
	if (CanExit())
		CDialogEx::OnClose();
}


BOOL CROBOT_CTLDlg::CanExit()
{
	// プロキシ オブジェクトがまだ残っている場合、オートメーション
	//  コントローラーはこのアプリケーションをまだ保持しています。
	//  ダイアログの周囲は残しますが UI は非表示になります。
	if (m_pAutoProxy != NULL)
	{
		ShowWindow(SW_HIDE);
		return FALSE;
	}

	return TRUE;
}



void CROBOT_CTLDlg::OnBnClickedOk()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	CDialogEx::OnOK();
}


void CROBOT_CTLDlg::OnBnClickedButton1()	// 全身トルクオン！
{
	


	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int i;

	for (i = 61; i < 69; i++) {
		sv_torque_w[i] = 1;
	}

	for (i = 71; i < 79; i++) {
		sv_torque_w[i] = 1;
	}

	for (i = 81; i < 84; i++) {
		sv_torque_w[i] = 1;
	}

	for (i = 91; i < 94; i++) {
		sv_torque_w[i] = 1;
	}

	UpdateData(FALSE);

}

void CROBOT_CTLDlg::OnBnClickedButton2()	// 全身脱力！
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int i;

	for (i = 61; i < 69; i++) {
		sv_torque_w[i] = 0;
	}

	for (i = 71; i < 79; i++) {
		sv_torque_w[i] = 0;
	}

	for (i = 81; i < 84; i++) {
		sv_torque_w[i] = 0;
	}

	for (i = 91; i < 94; i++) {
		sv_torque_w[i] = 0;
	}

	UpdateData(FALSE);
}


void CROBOT_CTLDlg::OnBnClickedSvOpen()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。


	com_openclode = "OPEN";
	UpdateData(FALSE);
	
	//	sv_open();
	sv_ctl_f = 1;	//スレッドコントロール用のフラグ (スレッドを立ち上げる前に1にして、これを0にするとスレッドの無限ループが終わる)
	
	// とりあえず全サーボトルクオフ
	OnBnClickedButton2();	// 脱力ボタンを呼び出し

	////////////スレッド用パラメータ
	HANDLE handle;

	////////////スレッドの生成
	handle = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)thread_motor, NULL, 0, NULL);

	sv_opne_ctlf.EnableWindow(FALSE);
	sv_close_ctlf.EnableWindow(TRUE);
}



void CROBOT_CTLDlg::OnBnClickedSvClose()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	sv_ctl_f = 0;
	//Sleep(1000);

	com_openclode = "CLOSE";
	UpdateData(FALSE);
	//sv_close();

	sv_opne_ctlf.EnableWindow(TRUE);
	sv_close_ctlf.EnableWindow(FALSE);

}



//----------------------------サーボモーターの関連関数 -------------------------


void sv_open(void) {


	//SM.begin(115200);

}

void sv_close(void) {


	//SM.~SerialWrapper();	//ポートクローズ

}

void RSGetDataALL(void)
{
	int i;
	sv_r rData[100];		// とりあえずID:0からID:99まで確保(サーボのデータ)

	for (i = 61; i < 69; i++) {
		
		rData[i] = SSV.sv_read2(i);
		
		Data[i].angle		= rData[i].angle;
		Data[i].load		= rData[i].load;
		Data[i].speed		= rData[i].speed;
		Data[i].temperature = rData[i].temperature;
		Data[i].time		= rData[i].time;
		Data[i].error		= rData[i].error;
	}

	for (i = 71; i < 79; i++) {

		rData[i] = SSV.sv_read2(i);

		Data[i].angle		= rData[i].angle;
		Data[i].load		= rData[i].load;
		Data[i].speed		= rData[i].speed;
		Data[i].temperature = rData[i].temperature;
		Data[i].time		= rData[i].time;
		Data[i].error		= rData[i].error;
	}
	for (i = 81; i < 84; i++) {

		rData[i] = SSV.sv_read2(i);

		Data[i].angle		= rData[i].angle;
		Data[i].load		= rData[i].load;
		Data[i].speed		= rData[i].speed;
		Data[i].temperature = rData[i].temperature;
		Data[i].time		= rData[i].time;
		Data[i].error		= rData[i].error;
	}

	for (i = 91; i < 94; i++) {

		rData[i] = SSV.sv_read2(i);

		Data[i].angle		= rData[i].angle;
		Data[i].load		= rData[i].load;
		Data[i].speed		= rData[i].speed;
		Data[i].temperature = rData[i].temperature;
		Data[i].time		= rData[i].time;
		Data[i].error		= rData[i].error;
	}

}

void RSGetDataALL2(void)
{
	SSV.sv_readFF(Data);
	// アクロバティック全データ取得
}


void CROBOT_CTLDlg::OnBnClickedButton7() //左側トルクオフ
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	int i;

	for (i = 61; i < 69; i++) {
		sv_torque_w[i] = 0;
	}

	UpdateData(FALSE);

}


void CROBOT_CTLDlg::OnBnClickedButton6()	//右側トルクオフ
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int i;

	for (i = 71; i < 79; i++) {
		sv_torque_w[i] = 0;
	}

	UpdateData(FALSE);
}


void CROBOT_CTLDlg::OnBnClickedButton9()	// 頭トルクオフ
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	int i;

	for (i = 91; i < 94; i++) {
		sv_torque_w[i] = 0;
	}


	UpdateData(FALSE);

}


void CROBOT_CTLDlg::OnBnClickedButton8()	// 腰トルクオフ
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	int i;

	for (i = 81; i < 84; i++) {
		sv_torque_w[i] = 0;
	}


	UpdateData(FALSE);
}


void CROBOT_CTLDlg::OnBnClickedButton10()	// ホームポジション
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int i;

	//ホームポジションでゴールポジションを初期化

	

	// ホームポジションのモーションを生成 ☆ ☆ ☆ ☆ ☆ ☆ ☆ ☆ ☆ ☆

	for (i = 0; i < 100; i++) {
		MotionBuf.pose[0].angle[i] = 0;
	}

	MotionBuf.pose[0].angle[63] =  1200;
	MotionBuf.pose[0].angle[73] = -1200;

	MotionBuf.pose[0].time = 100;		// 100*10msec = 1秒
	MotionBuf.flame = 1;				// トータル1ポーズ

	// ホームポジションのモーションを生成 ☆ ☆ ☆ ☆ ☆ ☆ ☆ ☆ ☆ ☆



	//////////// スレッド用パラメータ
	HANDLE handle;

	//////////// スレッドの生成
	handle = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)thread_motion_play, 0, 0, NULL);


	UpdateData(FALSE);
}




//モーター制御用のスレッド
void thread_motor(void)
{

	int i;
	clock_t now_time = 0,old_time = 0;	// 時間計測用


	if (SSV.init(get_port, get_baud) != true) {		//	COMポート RS232Cの初期化
												//	printf("ポート(%s)がオープン出来ませんでした。\n",OPN_COM);
//		port_states = 0;

		while (1);
	};


	// とりあえず全サーボトルクオフ
	for (i = 0; i < 100; i++) {
		SSV.sv_torque(i, 0);
	}

	while (sv_ctl_f) {
		
		// 処理時間の取得
		now_time = clock();						// 現在時間の更新
		Processing_time = now_time - old_time;	// 処理時間の算出
		old_time = now_time;					// 過去時間の更新

		// サーボの値を取得
		//RSGetDataALL2();		// アクロバティックスーパーサーボ取得
		RSGetDataALL();		// ノーマルサーボ取得


		Processing_time1 = clock() - now_time;

		// トルクに関して、違っていたら保存＆反映
		for (i = 0; i < 100; i++) {
			if (Data[i].torque != Data[i].old_torque) SSV.sv_torque(i, Data[i].torque);		// 違ってたら違うモノに変更
			Data[i].old_torque = Data[i].torque;											// 前回のトルクを保存
		}

		Processing_time2 = clock() - Processing_time1 - now_time;

		// 全サーボに目標角度と目標速度を送信！
		SSV.sv_move_long(Data);

		Processing_time3 = clock() - Processing_time2 - now_time - Processing_time1;

	}

	SSV.close();	// シリアルポートクローズ
	
}

void CROBOT_CTLDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: ここにメッセージ ハンドラー コードを追加するか、既定の処理を呼び出します。


	if (nIDEvent == 5678) {

		int i;

		UpdateData(TRUE);		// スライダの位置・ボックスの値を変数に代入

		for (i = 61; i < 69; i++) {
			sv_angle_r[i].Format(_T("%.1f deg"), ((float)Data[i].angle / 10));	//受信した値[角度]を格納
			sv_time_r[i].Format(_T("%.2f sec"), ((float)Data[i].time / 100));	//受信した値[時間]を格納
			sv_speed_r[i].Format(_T("%d d/s"), Data[i].speed);					//受信した値[速度]を格納
			sv_load_r[i].Format(_T("%d mA"), Data[i].load);						//受信した値[負荷]を格納
			sv_temperature_r[i].Format(_T("%d℃"), Data[i].temperature);		//受信した値[温度]を格納

			if (Data[i].error != 0) sv_angle_r[i].Format(_T("XXXXX"));			// エラーのときは[XXXXX]を表示

			Data[i].torque = sv_torque_w[i];									// トルクチェックボックスをトルクの値に反映

			if (Data[i].torque == 0) {			// トルクがオフの場合
				sv_angle_rw[i] = Data[i].angle;							// 角度→スライダー
				sv_angle_rw_v[i].Format(_T("%d"), Data[i].angle);		// 角度→値ボックス
			}
			else {								// トルクがオンの場合
				if ((flag_play == 0)&&(flag_motion_play == 0)) {	// 再生中でない場合
					Data[i].g_angle = sv_angle_rw[i];						// スライダー→角度
					sv_angle_rw_v[i].Format(_T("%d"), Data[i].g_angle);		// 角度→値ボックス
				}
				else {					// 再生中の場合
					sv_angle_rw[i] = Data[i].g_angle;							// 角度→スライダー
					sv_angle_rw_v[i].Format(_T("%d"), Data[i].g_angle);		// 角度→値ボックス
				}
			}
		}

		for (i = 71; i < 79; i++) {
			sv_angle_r[i].Format(_T("%.1f deg"), ((float)Data[i].angle / 10));	//受信した値[角度]を格納
			sv_time_r[i].Format(_T("%.2f sec"), ((float)Data[i].time / 100));	//受信した値[時間]を格納
			sv_speed_r[i].Format(_T("%d d/s"), Data[i].speed);					//受信した値[速度]を格納
			sv_load_r[i].Format(_T("%d mA"), Data[i].load);						//受信した値[負荷]を格納
			sv_temperature_r[i].Format(_T("%d℃"), Data[i].temperature);		//受信した値[温度]を格納

			if (Data[i].error != 0) sv_angle_r[i].Format(_T("XXXXX"));			// エラーのときは[XXXXX]を表示

			Data[i].torque = sv_torque_w[i];									// トルクチェックボックスをトルクの値に反映



			if (Data[i].torque == 0) {			// トルクがオフの場合
				sv_angle_rw[i] = Data[i].angle;							// 角度→スライダー
				sv_angle_rw_v[i].Format(_T("%d"), Data[i].angle);		// 角度→値ボックス
			}
			else {								// トルクがオンの場合
									
				if ((flag_play == 0) && (flag_motion_play == 0)) {	// 再生中でない場合
					Data[i].g_angle = sv_angle_rw[i];						// スライダー→角度
					sv_angle_rw_v[i].Format(_T("%d"), Data[i].g_angle);		// 角度→値ボックス
				}
				else {					// 再生中の場合
					sv_angle_rw[i] = Data[i].g_angle;							// 角度→スライダー
					sv_angle_rw_v[i].Format(_T("%d"), Data[i].g_angle);		// 角度→値ボックス
				}
			}
		}



		for (i = 81; i < 84; i++) {
			sv_angle_r[i].Format(_T("%.1f deg"), ((float)Data[i].angle / 10));	//受信した値[角度]を格納
			sv_time_r[i].Format(_T("%.2f sec"), ((float)Data[i].time / 100));	//受信した値[時間]を格納
			sv_speed_r[i].Format(_T("%d d/s"), Data[i].speed);					//受信した値[速度]を格納
			sv_load_r[i].Format(_T("%d mA"), Data[i].load);						//受信した値[負荷]を格納
			sv_temperature_r[i].Format(_T("%d℃"), Data[i].temperature);		//受信した値[温度]を格納

			if (Data[i].error != 0) sv_angle_r[i].Format(_T("XXXXX"));			// エラーのときは[XXXXX]を表示

			Data[i].torque = sv_torque_w[i];									// トルクチェックボックスをトルクの値に反映

			if (Data[i].torque == 0) {			// トルクがオフの場合
				sv_angle_rw[i] = Data[i].angle;							// 角度→スライダー
				sv_angle_rw_v[i].Format(_T("%d"), Data[i].angle);		// 角度→値ボックス
			}
			else {								// トルクがオンの場合
				if ((flag_play == 0) && (flag_motion_play == 0)) {	// 再生中でない場合
					Data[i].g_angle = sv_angle_rw[i];						// スライダー→角度
					sv_angle_rw_v[i].Format(_T("%d"), Data[i].g_angle);		// 角度→値ボックス
				}
				else {					// 再生中の場合
					sv_angle_rw[i] = Data[i].g_angle;							// 角度→スライダー
					sv_angle_rw_v[i].Format(_T("%d"), Data[i].g_angle);		// 角度→値ボックス
				}
			}
		}

		for (i = 91; i < 94; i++) {
			sv_angle_r[i].Format(_T("%+06.1f deg"), ((float)Data[i].angle / 10));	//受信した値[角度]を格納
			sv_time_r[i].Format(_T("%04.2f sec"), ((float)Data[i].time / 100));	//受信した値[時間]を格納
			sv_speed_r[i].Format(_T("%+04d d/s"), Data[i].speed);					//受信した値[速度]を格納
			sv_load_r[i].Format(_T("%03d mA"), Data[i].load);						//受信した値[負荷]を格納
			sv_temperature_r[i].Format(_T("%d℃"), Data[i].temperature);		//受信した値[温度]を格納

			if (Data[i].error != 0) sv_angle_r[i].Format(_T("XXXXX"));			// エラーのときは[XXXXX]を表示

			Data[i].torque = sv_torque_w[i];									// トルクチェックボックスをトルクの値に反映

			if (Data[i].torque == 0) {			// トルクがオフの場合
				sv_angle_rw[i] = Data[i].angle;							// 角度→スライダー
				sv_angle_rw_v[i].Format(_T("%d"), Data[i].angle);		// 角度→値ボックス
			}
			else {								// トルクがオンの場合
				if ((flag_play == 0) && (flag_motion_play == 0)) {	// 再生中でない場合
					Data[i].g_angle = sv_angle_rw[i];						// スライダー→角度
					sv_angle_rw_v[i].Format(_T("%d"), Data[i].g_angle);		// 角度→値ボックス
				}
				else {					// 再生中の場合
					sv_angle_rw[i] = Data[i].g_angle;							// 角度→スライダー
					sv_angle_rw_v[i].Format(_T("%d"), Data[i].g_angle);		// 角度→値ボックス
				}
			}
		}


		// 制御時間の表示
		temp0001.Format(_T("処理時間: %d msec (%d /%d /%d)"), Processing_time, Processing_time1, Processing_time2, Processing_time3);

		// ラジオボタンのいろいろ

		if (flip_radio == 0) {
			// 通常時 なにもしない
		
		}else if (flip_radio == 1) {
			for (i = 1; i < 9; i++) {			// 61->71　～ 68->78
				sv_angle_rw[70 + i] = -sv_angle_rw[60 + i];
			}
		}else if (flip_radio == 2) {
			for (i = 1; i < 9; i++) {			// 61<-71　～ 68<-78
				sv_angle_rw[60 + i] = -sv_angle_rw[70 + i];
			}
		}

		UpdateData(FALSE);	//変数の値をスライダの位置・ボックスの値に反映

		if ((flag_rec == 0) && (flag_play == 0) && (flag_motion_play==0)) {	//	
			SetWindowText(_T("ROBOT_CTL"));
		}


		saisei_kaishi_G = saisei_kaishi;	// 再生開始位置をグローバル変数に上書き

	}

	
	CDialogEx::OnTimer(nIDEvent);
	
}


void CROBOT_CTLDlg::OnBnClickedInvertLr()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int temp,i;

	for (i = 1; i < 9; i++) {			// 61<->71　～ 68<->78
		temp = sv_angle_rw[60 + i];
		sv_angle_rw[60 + i] = - sv_angle_rw[70 + i];
		sv_angle_rw[70 + i] = - temp;
	}

	sv_angle_rw[81] = -sv_angle_rw[81];	// 符号反転 81
	sv_angle_rw[82] = -sv_angle_rw[82]; // 符号反転 82
										// そのまま 83
	sv_angle_rw[91] = -sv_angle_rw[91];	// 符号反転 91
	sv_angle_rw[92] = -sv_angle_rw[92];	// 符号反転 92
										// そのまま 93
	UpdateData(FALSE);
}


void CROBOT_CTLDlg::OnBnClickedCopyL2r()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	int  i;

	for (i = 1; i < 9; i++) {			// 61->71　～ 68->78
		sv_angle_rw[70 + i] = -sv_angle_rw[60 + i];
	}
	UpdateData(FALSE);
}


void CROBOT_CTLDlg::OnBnClickedCopyR2l()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	int  i;

	for (i = 1; i < 9; i++) {			// 61<-71　～ 68<-78
		sv_angle_rw[60 + i] = -sv_angle_rw[70 + i];
	}
	UpdateData(FALSE);
}


void CROBOT_CTLDlg::OnBnClickedBtnGetdelay()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int i = 0;

	// ポートオープン
	if (SSV.init(get_port, get_baud) != true) {		//	COMポート RS232Cの初期化
													//	printf("ポート(%s)がオープン出来ませんでした。\n",OPN_COM);
													//		port_states = 0;

		while (1);
	};


	for (i = 61; i < 69; i++) {
		sv_angle_r[i].Format(_T("D:%d"), SSV.sv_read_Responsetime(i));	// 受信した値[ディレイ]を格納
	}

	for (i = 71; i < 79; i++) {
		sv_angle_r[i].Format(_T("D:%d"), SSV.sv_read_Responsetime(i));	// 受信した値[ディレイ]を格納
	}

	for (i = 81; i < 84; i++) {
		sv_angle_r[i].Format(_T("D:%d"), SSV.sv_read_Responsetime(i));	// 受信した値[ディレイ]を格納
	}

	for (i = 91; i < 94; i++) {
		sv_angle_r[i].Format(_T("D:%d"), SSV.sv_read_Responsetime(i));	// 受信した値[ディレイ]を格納
	}

	SSV.close();	// シリアルポートクローズ

	UpdateData(FALSE);
}


void CROBOT_CTLDlg::OnBnClickedBtnDelay0()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int i = 0;

	// ポートオープン
	if (SSV.init(get_port, get_baud) != true) {		//	COMポート RS232Cの初期化
													//	printf("ポート(%s)がオープン出来ませんでした。\n",OPN_COM);
													//		port_states = 0;

		while (1);
	};


	for (i = 61; i < 69; i++) {
		SSV.sv_write_Responsetime(i, 0);
		SSV.sv_flash_write(i);	// 書き込み

	}

	for (i = 71; i < 79; i++) {
		SSV.sv_write_Responsetime(i, 0);
		SSV.sv_flash_write(i);	// 書き込み
	}

	for (i = 81; i < 84; i++) {
		SSV.sv_write_Responsetime(i, 0);
		SSV.sv_flash_write(i);	// 書き込み
	}

	for (i = 91; i < 94; i++) {
		SSV.sv_write_Responsetime(i, 0);
		SSV.sv_flash_write(i);	// 書き込み
	}

	SSV.close();	// シリアルポートクローズ

	UpdateData(FALSE);




}


void CROBOT_CTLDlg::OnBnClickedBtnSetdelay()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int i;

	// ポートオープン
	if (SSV.init(get_port, get_baud) != true) {		//	COMポート RS232Cの初期化
													//	printf("ポート(%s)がオープン出来ませんでした。\n",OPN_COM);
													//		port_states = 0;

		while (1);
	};
	
	// レスポンスタイム 400μ秒
	SSV.sv_write_Responsetime(61, 0);
	SSV.sv_write_Responsetime(62, 8);
	SSV.sv_write_Responsetime(63, 16);
	SSV.sv_write_Responsetime(64, 24);
	SSV.sv_write_Responsetime(65, 32);
	SSV.sv_write_Responsetime(66, 40);
	SSV.sv_write_Responsetime(67, 48);
	SSV.sv_write_Responsetime(68, 56);

	SSV.sv_write_Responsetime(71, 64);
	SSV.sv_write_Responsetime(72, 72);
	SSV.sv_write_Responsetime(73, 80);
	SSV.sv_write_Responsetime(74, 88);
	SSV.sv_write_Responsetime(75, 96);
	SSV.sv_write_Responsetime(76, 104);
	SSV.sv_write_Responsetime(77, 112);
	SSV.sv_write_Responsetime(78, 120);

	SSV.sv_write_Responsetime(81, 128);
	SSV.sv_write_Responsetime(82, 136);
	SSV.sv_write_Responsetime(83, 144);

	SSV.sv_write_Responsetime(91, 152);
	SSV.sv_write_Responsetime(92, 160);
	SSV.sv_write_Responsetime(93, 168);


	for (i = 61; i < 69; i++) {
		SSV.sv_flash_write(i);	// 書き込み

	}

	for (i = 71; i < 79; i++) {
		SSV.sv_flash_write(i);	// 書き込み
	}

	for (i = 81; i < 84; i++) {
		SSV.sv_flash_write(i);	// 書き込み
	}

	for (i = 91; i < 94; i++) {
		SSV.sv_flash_write(i);	// 書き込み
	}


	SSV.close();	// シリアルポートクローズ

	UpdateData(FALSE);


}


void CROBOT_CTLDlg::OnBnClickedRec()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	//現在時間の取得
	CTime cTime = CTime::GetCurrentTime();           // 現在時刻

	//現在時間をファイル名に
	filetime = cTime.Format("RTT_%Y%m%d_%H%M%S.RT.csv");   // "YYYY/mm/dd HH:MM:SS"形式の時刻文字列を取得	// ファイル名 RTT リアルタイムティーチ

	//保存中のファイル名をウィンドウバーに表示
	CString window_name;
	window_name = _T("ROBOT_CTL (キャプチャ中) -") + filetime;

	SetWindowText(window_name);

	current_filename = filetime;
	select_filename = filetime;
	UpdateData(FALSE);


	////////////スレッド用パラメータ
	HANDLE handle;

	////////////スレッドの生成
	handle = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)thread_rec, NULL, 0, NULL);

}


void CROBOT_CTLDlg::OnBnClickedStop()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	flag_rec = 0;
	flag_play = 0;


	SetWindowText(_T("ROBOT_CTL"));


	// ファイル一覧の更新

	HANDLE hFind;
	WIN32_FIND_DATA fd;

	/* 最初のファイル検索 */
	hFind = FindFirstFile("*.RT.csv", &fd);

	csv_list.ResetContent();

	/* 検索失敗? */
	if (hFind == INVALID_HANDLE_VALUE) {}
	else {
		do {
			/* 結果の表示 */
			csv_list.AddString(fd.cFileName);
		} while (FindNextFile(hFind, &fd)); //次のファイルを検索
	}

	/* 検索終了 */
	FindClose(hFind);


}

//録画用スレッド
void thread_rec(void) {

	int i;

	FILE *csv_fp;  /* ファイルポインタ */
	fopen_s(&csv_fp, filetime, "w");

	flag_rec = 1;

	while (flag_rec) {
		Sleep(16);

		for (i = 61; i < 69; i++) {
			//トルクオフなら取得値　オンなら指示角 g_angle
			if (Data[i].torque == 0) fprintf_s(csv_fp, "%d,", Data[i].angle);
			else fprintf_s(csv_fp, "%d,", Data[i].g_angle);
		}

		for (i = 71; i < 79; i++) {
			//トルクオフなら取得値　オンなら指示角 g_angle
			if (Data[i].torque == 0) fprintf_s(csv_fp, "%d,", Data[i].angle);
			else fprintf_s(csv_fp, "%d,", Data[i].g_angle);
		}

		for (i = 81; i < 84; i++) {
			//トルクオフなら取得値　オンなら指示角 g_angle
			if (Data[i].torque == 0) fprintf_s(csv_fp, "%d,", Data[i].angle);
			else fprintf_s(csv_fp, "%d,", Data[i].g_angle);
		}

		for (i = 91; i < 94; i++) {
			//トルクオフなら取得値　オンなら指示角 g_angle
			if (Data[i].torque == 0) fprintf_s(csv_fp, "%d,", Data[i].angle);
			else fprintf_s(csv_fp, "%d,", Data[i].g_angle);
		}

		fprintf_s(csv_fp, "\n");

	}
	fclose(csv_fp);

}

//再生用スレッド
void thread_play(void) {
	int i;
	FILE *csv_fp;  /* ファイルポインタ */
	fopen_s(&csv_fp, select_filename, "r");


	while ((flag_play) && (feof(csv_fp) == 0)) {


		for (i = 61; i < 69; i++) {
			fscanf_s(csv_fp, "%d,", &Data[i].g_angle);
		}

		for (i = 71; i < 79; i++) {
			fscanf_s(csv_fp, "%d,", &Data[i].g_angle);
		}

		for (i = 81; i < 84; i++) {
			fscanf_s(csv_fp, "%d,", &Data[i].g_angle);
		}

		for (i = 91; i < 94; i++) {
			fscanf_s(csv_fp, "%d,", &Data[i].g_angle);
		}


		fscanf_s(csv_fp, "\n");

		Sleep(16);
	}

	fclose(csv_fp);
	flag_play = 0;
	
}

void CROBOT_CTLDlg::OnBnClickedPlay()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	flag_play = 1;

	//保存中のファイル名をウィンドウバーに表示
	CString window_name;
	window_name = _T("ROBOT_CTL (RT再生中) -") + filetime;

	SetWindowText(window_name);

	//////////スレッド用パラメータ
	HANDLE play_handle;

	//////////スレッドの生成
	play_handle = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)thread_play, NULL, 0, NULL);
}

void CROBOT_CTLDlg::OnLbnSelchangeList1()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	select_filename = csvlist;
	current_filename = select_filename;
	UpdateData(FALSE);

}


void CROBOT_CTLDlg::OnBnClickedButton3()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	DeleteFile(select_filename);

	// ファイル一覧の更新

	HANDLE hFind;
	WIN32_FIND_DATA fd;

	/* 最初のファイル検索 */
	hFind = FindFirstFile("*.csv", &fd);

	csv_list.ResetContent();

	/* 検索失敗? */
	if (hFind == INVALID_HANDLE_VALUE) {}
	else {
		do {
			/* 結果の表示 */
			csv_list.AddString(fd.cFileName);
		} while (FindNextFile(hFind, &fd)); //次のファイルを検索
	}

	/* 検索終了 */
	FindClose(hFind);
}


void CROBOT_CTLDlg::OnBnClickedButton4()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int i;

	for (i = 61; i < 69; i++) {
		sv_torque_w[i] = 1;
	}

	UpdateData(FALSE);
}


void CROBOT_CTLDlg::OnBnClickedButton11()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	int i;


	for (i = 71; i < 79; i++) {
		sv_torque_w[i] = 1;
	}


	UpdateData(FALSE);
}


void CROBOT_CTLDlg::OnBnClickedButton5()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int i;


	for (i = 91; i < 94; i++) {
		sv_torque_w[i] = 1;
	}

	UpdateData(FALSE);
}


void CROBOT_CTLDlg::OnBnClickedButton12()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int i;
	
	for (i = 81; i < 84; i++) {
		sv_torque_w[i] = 1;
	}


	UpdateData(FALSE);
}


void CROBOT_CTLDlg::OnBnClickedButton14()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	int i;

	for (i = 91; i < 94; i++) {
		sv_angle_rw[i] = 0;	// スライダー
	}

	UpdateData(FALSE);
}


void CROBOT_CTLDlg::OnBnClickedButton13()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int i;

	for (i = 81; i < 84; i++) {
		sv_angle_rw[i] = 0;	// スライダー
	}

	UpdateData(FALSE);
}


void thread_motion_play(void) {

	int i, j, k;
	short old_pos[100];

	flag_motion_play = 1;	// モーション再生フラグ


	for (k = saisei_kaishi_G; k < MotionBuf.flame; k++) {

		for (i = 0; i < 100; i++) {
			old_pos[i] = Data[i].g_angle;		// 今のポジションを保存
		}

		for (j = 1; j < MotionBuf.pose[k].time ; j++) {

			for (i = 0; i < 100; i++) {
				Data[i].g_angle = old_pos[i] + (MotionBuf.pose[k].angle[i] - old_pos[i]) * ((double)j / (double)MotionBuf.pose[k].time);
			}

			Sleep(10);

		}

		for (i = 0; i < 100; i++) {
			Data[i].g_angle = MotionBuf.pose[k].angle[i];
		}

		if (flag_motion_play == 0) break;	// もし再生フラグが倒れていたら中断

	}
	
	flag_motion_play = 0;	// モーション再生フラグ

}


void CROBOT_CTLDlg::OnBnClickedButCap()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int i;
	char templist[80];

	for (i = 0; i<100; i++) {
		MotionBuf.pose[MotionBuf.flame].angle[i] = Data[i].g_angle;	// 今のポジションを保存
	}

	MotionBuf.pose[MotionBuf.flame].time = wait_value;

	sprintf_s(templist, 80, "%03d : pose : T=%d", MotionBuf.flame, wait_value);

	move_list.AddString(_T(templist));

	MotionBuf.flame++;

}


void CROBOT_CTLDlg::OnBnClickedButKakikae()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int i;
	int temp_g;
	char templist[80];

	temp_g = move_list.FindString(0, select_move);

	//一旦削除
	move_list.DeleteString(temp_g);

	for (i = 0; i<100; i++) {
		MotionBuf.pose[temp_g].angle[i] = Data[i].g_angle;	// 今のポジションを保存
	}

	MotionBuf.pose[temp_g].time = wait_value;

	//再度書き込み
	sprintf_s(templist, 80, "%03d : pose : T=%d", temp_g, wait_value);
	move_list.AddString(_T(templist));
}




void CROBOT_CTLDlg::OnBnClickedButSounyu()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	
	int i, j;
	int temp_g;
	char templist[80];

	temp_g = move_list.FindString(0, select_move);

	// ずらす
	for (j = MotionBuf.flame + 1 ; j > temp_g; j--) {

		for (i = 0; i<100 + 1; i++) {

			MotionBuf.pose[j].angle[i] = MotionBuf.pose[j-1].angle[i];
		}

		MotionBuf.pose[j].time = MotionBuf.pose[j - 1].time;

	}
	

	// 挿入点に保存
	for (i = 0; i<100; i++) {
		MotionBuf.pose[temp_g].angle[i] = Data[i].g_angle;
	}

	MotionBuf.pose[temp_g].time = wait_value;

	MotionBuf.flame++;
	

	// 表示もずらす
	
	for (j = temp_g; j < MotionBuf.flame; j++) {

		move_list.DeleteString(j);

		sprintf_s(templist, 80, "%03d : pose : T=%d", j, MotionBuf.pose[j].time);
		move_list.AddString(_T(templist));
	}

	
}


void CROBOT_CTLDlg::OnBnClickedButSaisei()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。


	//////////スレッド用パラメータ
	HANDLE play_handle;

	//////////スレッドの生成
	play_handle = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)thread_motion_play, NULL, 0, NULL);

	SetWindowText(_T("ROBOT_CTL (モーション再生中)"));

}


void CROBOT_CTLDlg::OnBnClickedButClear()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	move_list.ResetContent();
	MotionBuf.flame = 0;
}


void CROBOT_CTLDlg::OnBnClickedButDelete()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int i, j;
	int temp_g;
	char templist[80];

	temp_g = move_list.FindString(0, select_move);
	move_list.DeleteString(temp_g);

	for (j = temp_g; j<MotionBuf.flame - 1; j++) {
		move_list.DeleteString(j);

		for (i = 0; i < 100; i++) {
			MotionBuf.pose[j].angle[i] = MotionBuf.pose[j+1].angle[i];
			//JMOVESAVE[j][i] = JMOVESAVE[j + 1][i];
		}
		MotionBuf.pose[j].time = MotionBuf.pose[j+1].time;

		sprintf_s(templist, 80, "%03d : pose : T=%d", j, MotionBuf.pose[j].time);
		//sprintf_s(templist, 80, "%03d : motion : wait %d", j, JMOVESAVE[j][JOINT_N]);
		move_list.AddString(_T(templist));
	}

	MotionBuf.flame--;

}


void CROBOT_CTLDlg::OnBnClickedButDoPose()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。


	int i;
	int temp_g;

	temp_g = move_list.FindString(0, select_move);

	for (i = 0; i < 100; i++) {
		sv_angle_rw[i] = MotionBuf.pose[temp_g].angle[i];
	}
	UpdateData(FALSE);
}


void CROBOT_CTLDlg::OnBnClickedMotionSave()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	int i, j;
	char temp[80];

	sprintf_s(temp, 80, "%s.MT.csv", motion_name);

	FILE *csv_fp;  /* ファイルポインタ */
	fopen_s(&csv_fp, temp, "w");

	for (j = 0; j < MotionBuf.flame; j++) {
		fprintf_s(csv_fp, "%d|", MotionBuf.pose[j].time);
		for (i = 0; i<100; i++) {
			fprintf_s(csv_fp, "%d,", MotionBuf.pose[j].angle[i]);
		}
		fprintf_s(csv_fp, "\n");
	}
	fclose(csv_fp);

	if (motion_file_list.FindString(-1, motion_name) == LB_ERR) motion_file_list.AddString(temp);

}


void CROBOT_CTLDlg::OnBnClickedMotionLoad()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int i;
	char templist[80];
	CString templist_buf;
	FILE *csv_fp;  /* ファイルポインタ */

	//クリア
	move_list.ResetContent();
	MotionBuf.flame = 0;

	fopen_s(&csv_fp, motion_file_select, "r");

	while (feof(csv_fp) == 0) {
		fscanf_s(csv_fp, "%d|", &MotionBuf.pose[MotionBuf.flame].time);
		for (i = 0; i<100; i++) {
			fscanf_s(csv_fp, "%d,", &MotionBuf.pose[MotionBuf.flame].angle[i]);
		}
		fscanf_s(csv_fp, "\n");
		sprintf_s(templist, 80, "%03d : pose : T=%d", MotionBuf.flame, MotionBuf.pose[MotionBuf.flame].time);
		move_list.AddString(_T(templist));
		MotionBuf.flame++;
	}
	fclose(csv_fp);

	//ファイル名だけを抜き出す
	motion_name = motion_file_select.Left(motion_file_select.GetLength() - 7);
	UpdateData(FALSE);

}


void CROBOT_CTLDlg::OnBnClickedButton21()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	flag_motion_play = 0;
	//再生フラグを強制的に止める

}


void CROBOT_CTLDlg::OnBnClickedButton15()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// モーダレスウィンドウ作成
	TimeLineDialog *m_pmodeless = new TimeLineDialog(this);
	m_pmodeless->ShowWindow(SW_SHOW);


}