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

sv_r pubData[SRVCNT];
sv_r subData[SRVCNT];

SVMotor SSV;

sensor_msgs::JointState js_pub;
sensor_msgs::JointState js_sub;

void RSGetDataALL(void)
{
	int i;
	sv_r rData[SRVCNT];
	for (i = 0; i < SRVCNT; i++) {

		rData[i] = SSV.sv_read2(id_list[i]);

		if (rData[i].error == 0){
			pubData[i].id			= id_list[i];
			pubData[i].angle		= rData[i].angle;
			pubData[i].load		= rData[i].load;
			pubData[i].speed		= rData[i].speed;
			pubData[i].temperature = rData[i].temperature;
			pubData[i].time		= rData[i].time;
			pubData[i].error		= rData[i].error;
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
		js_pub.position[i] = (float)2.0*M_PI*(pubData[i].angle / 10.0)/360.0;		// deg 2 rad
		js_pub.velocity[i] = (float)2.0*M_PI*pubData[i].speed/360.0;		// deg/s 2 rad/s
		js_pub.effort[i] = pubData[i].load;
	}
}

//コールバックがあるとグローバルに読み込み
void monitorJointState_callback(const sensor_msgs::JointState::ConstPtr& jointstate){
	int i,j;
	for(i=0;i<SRVCNT;i++){
		for(j=0;j<SRVCNT;j++){
			if(joint_list[i] == jointstate->name[j]){
				//printf("%s / %lf\n",jointstate->name[j].c_str(),jointstate->position[j] );
    			//js_sub.name[i] = jointstate->name[j];    // 
        		//js_sub.position[i] = jointstate->position[j];    // ポジション読み出し
    			//js_sub.velocity[i] = jointstate->velocity[j];    // 速度読み出し
    			//js_sub.effort[i] = jointstate->effort[j];    // 負荷読み出し

				subData[i].id = id_list[i];
				subData[i].g_angle =(short)  3600.0 * (jointstate->position[j]/(2.0*M_PI));
				subData[i].g_time = 2;	// サーボの移動速度を20msecに固定
		
				//printf("%s / %lf|| %d / %d / %d\n",jointstate->name[j].c_str(),jointstate->position[j],subData[i].id , subData[i].g_angle , subData[i].g_time );
			}
		}
	}
}

int main(int argc, char **argv)
{
	ros::init(argc, argv, "rs_controller");
	ros::NodeHandle node;

	//パブリッシャの作成
	ros::Publisher pub_joints;
	pub_joints = node.advertise<sensor_msgs::JointState>("/rs_controller/joint_states",10);
//	pub_joints = node.advertise<sensor_msgs::JointState>("/joint_states",10);

	// サブスクライバの作成
	ros::Subscriber sub_joints;
	sub_joints = node.subscribe("/joint_states", 10, monitorJointState_callback);		// moveitから取る
//	sub_joints = node.subscribe("/move_group/fake_controller_joint_states", 10, monitorJointState_callback);		// moveitから取る


	ros::Rate loop_rate(100);	// 100Hz

	if (SSV.init(get_port, get_baud) != true) {		//	COMポート RS232Cの初期化
		//	printf("ポート(%s)がオープン出来ませんでした。\n",OPN_COM);
		while (1);
	}; 

	int i;
	for(i=0;i<SRVCNT;i++){
		SSV.sv_torque(id_list[i], 1);	// トルクオン
	}

	while(ros::ok()){

		RSGetDataALL();		// サーボ取得
		Data2Jointstate();
		pub_joints.publish(js_pub);

		SSV.sv_move_long(subData,SRVCNT);

		ros::spinOnce();   // コールバック関数を呼ぶ
    	loop_rate.sleep();
	}

	for(i=0;i<SRVCNT;i++){
		SSV.sv_torque(id_list[i], 0);	// トルク
	}

	SSV.close();	// シリアルポートクローズ

	return 0;
}

