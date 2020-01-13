#include <ros/ros.h>
#include "sensor_msgs/JointState.h"
#include "serial.h"
#include "sv_motor.h"

// デフォルトのシリアルポート デバイス名　速度
#define SERIAL_PORT "/dev/ttyUSB0"
#define SERIAL_BAUD 460800

// 接続するサーボの個数
#define SRVCNT 22

// 接続するサーボモーターのIDの列挙
int id_list[SRVCNT]={61 ,62 ,63 ,64 ,65 ,66 ,67 ,68 ,71 ,72 ,73 ,74 ,75 , 76 ,77 ,78 ,81 ,82 ,83 ,91 ,92 ,93};

// JointState::nameの列挙、順番は上記のモーターIDを対応するように並べる
char joint_list[SRVCNT][30]={
	"body_to_shoulder1R",
	"shoulder1R_to_shoulder2R",
	"shoulder2R_to_shoulder3R",
	"shoulder3R_to_arm1R",
	"arm1R_to_arm2R",
	"arm2R_to_arm3R",
	"arm3R_to_arm4R",
	"arm4R_to_arm5R",

	"body_to_shoulder1L",
	"shoulder1L_to_shoulder2L",
	"shoulder2L_to_shoulder3L",
	"shoulder3L_to_arm1L",
	"arm1L_to_arm2L",
	"arm2L_to_arm3L",
	"arm3L_to_arm4L",
	"arm4L_to_arm5L",

	"base_to_koshi",
	"koshi1_to_koshi2",
	"koshi2_to_body",

	"body_to_kubi1",
	"kubi1_to_kubi2",
	"kubi_to_head"
};

char get_port[] = SERIAL_PORT;
int get_baud = SERIAL_BAUD;

sv_r Data[SRVCNT];		//とりあえず0〜99のIDを
SVMotor SSV;

sensor_msgs::JointState js_pub;


void RSGetDataALL(void)
{
	int i;
	sv_r rData[SRVCNT];
	for (i = 0; i < SRVCNT; i++) {

		rData[i] = SSV.sv_read2(id_list[i]);

		if (rData[i].error == 0){
			Data[i].id			= id_list[i];
			Data[i].angle		= rData[i].angle;
			Data[i].load		= rData[i].load;
			Data[i].speed		= rData[i].speed;
			Data[i].temperature = rData[i].temperature;
			Data[i].time		= rData[i].time;
			Data[i].error		= rData[i].error;
		}
	}

}

void Data2Jointstate(void){
	int i;

	js_pub.header.stamp = ros::Time::now();

	js_pub.name.resize(SRVCNT);
	js_pub.position.resize(SRVCNT);
	js_pub.velocity.resize(SRVCNT);
	js_pub.effort.resize(SRVCNT);
	
	for(i=0;i<SRVCNT;i++){
		js_pub.name[i] = joint_list[i];
		js_pub.position[i] = (float)2.0*M_PI*(Data[i].angle / 10.0)/360.0;		// deg 2 rad
		js_pub.velocity[i] = (float)2.0*M_PI*Data[i].speed/360.0;		// deg/s 2 rad/s
		js_pub.effort[i] = Data[i].load;
	}
}

int main(int argc, char **argv)
{
	ros::init(argc, argv, "rs_controller");
	ros::NodeHandle node;

	//パブリッシャの作成
	ros::Publisher pub_joints;
//	pub_joints = node.advertise<sensor_msgs::JointState>("/rs_controller/joint_states",10);
	pub_joints = node.advertise<sensor_msgs::JointState>("/joint_states",10);

	ros::Rate loop_rate(100);	// 100Hz

	if (SSV.init(get_port, get_baud) != true) {		//	COMポート RS232Cの初期化
		//	printf("ポート(%s)がオープン出来ませんでした。\n",OPN_COM);
		while (1);
	}; 

	while(ros::ok()){

		RSGetDataALL();		// ノーマルサーボ取得
		Data2Jointstate();

		//printf("ID78 = %d ERR = %d\n",Data[78].angle,Data[78].error);

		pub_joints.publish(js_pub);

		ros::spinOnce();   // コールバック関数を呼ぶ
    	loop_rate.sleep();
	}

		SSV.close();	// シリアルポートクローズ

	return 0;
}

