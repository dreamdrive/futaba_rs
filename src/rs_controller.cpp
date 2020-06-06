#include <ros/ros.h>
#include "sensor_msgs/JointState.h"
#include "sv_motor.h"

// デフォルトのシリアルポート デバイス名(起動時のrosparamでも変更可能)
#define SERIAL_PORT "/dev/ttyUSB0"
#define SERIAL_BAUD 460800

// 接続するサーボの個数
#define SRVCNT 19

// 接続する双葉サーボモーターのIDの列挙
int id_list[SRVCNT]={61 ,62 ,63 ,64 ,65 ,66 ,67 ,68 ,71 ,72 ,73 ,74 ,75 , 76 ,77 ,78 ,91 ,92 ,93};

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
	"body_to_kubi1",
	"kubi1_to_kubi2",
	"kubi_to_head"
};

sv_r pubData[SRVCNT];
sv_r subData[SRVCNT];
SVMotor RS;

sensor_msgs::JointState js_pub;
sensor_msgs::JointState js_sub;

void RSGetDataALL(void)
{
	int i;
	sv_r rData[SRVCNT];		// 受信データ
	for (i = 0; i < SRVCNT; i++) {
		rData[i] = RS.sv_read2(id_list[i]);

		if (rData[i].error == NO_ERROR){
			pubData[i].id			= id_list[i];	// 受信データをパブリッシュ用データに変換
			pubData[i].angle		= rData[i].angle;
			pubData[i].load			= rData[i].load;
			pubData[i].speed		= rData[i].speed;
			pubData[i].temperature 	= rData[i].temperature;
			pubData[i].time			= rData[i].time;
			pubData[i].error		= rData[i].error;
		}else{
			ROS_ERROR("Futaba Servo ERROR ( ID : %d | code : %04X )",id_list[i],rData[i].error);
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
		js_pub.velocity[i] = (float)2.0*M_PI*pubData[i].speed/360.0;				// deg/s 2 rad/s
		js_pub.effort[i] = pubData[i].load;
	}
}

//コールバックがあるとグローバルに読み込み(jsからサーボ値に変換)
void monitorJointState_callback(const sensor_msgs::JointState::ConstPtr& jointstate){
	int i,j;
	for(i=0;i<SRVCNT;i++){
		for(j=0;j<SRVCNT;j++){
			if(joint_list[i] == jointstate->name[j]){

				subData[i].id = id_list[i];
				subData[i].g_angle =(short)  3600.0 * (jointstate->position[j]/(2.0*M_PI));
				subData[i].g_time = 2;	// サーボの移動速度を20msecに固定 (50Hz)
		
			}
		}
	}
}

int main(int argc, char **argv)
{
	std::string port_name;
	int baud_rate;
	int i;
	int result;

	ros::init(argc, argv, "rs_controller");
	ros::NodeHandle node;

	ros::NodeHandle private_nh("~");

	port_name = SERIAL_PORT;
	private_nh.getParam("port_name",port_name);

	baud_rate = SERIAL_BAUD;
	private_nh.getParam("baud_rate",baud_rate);

	printf("port open ( %s )  baudrate ( %d )\n",port_name.data(),baud_rate);

	//	COMポート RS232Cの初期化
	if (RS.init(port_name.data(), baud_rate) != 0) {
		printf("can't open serial port ( %s ) \n",port_name.data());
		return 0;
	}; 

	for(i=0;i<SRVCNT;i++){

		result = RS.sv_read_torque(id_list[i]);	// トルクリード
		if (result >= 0){
			ROS_INFO("Succeeded to connet (id : %d ) torque : %d", id_list[i], result);
		}else{
			ROS_ERROR("Failed to connet (id : %d ) error : %d", id_list[i], result);
			RS.sv_close();
			return 0;
		}
	}

	for(i=0;i<SRVCNT;i++){
		result = RS.sv_torque(id_list[i], 1);	// トルクオン
		if (result == 9){
			ROS_INFO("Succeeded to torque on id : %d", id_list[i]);
		}else{
			ROS_ERROR("Failed to rtorque on id : %d", id_list[i]);	
		}
	}

	// 初期化ポーズ
	sv_r init_pose[SRVCNT];
	for (i = 0; i < SRVCNT; i++) {
		init_pose[i].id	= id_list[i];
		init_pose[i].g_angle = 0;
		init_pose[i].g_time = 100;	// 100*10msec		
	}
	RS.sv_move_long(init_pose,SRVCNT);

	//パブリッシャの作成
	ros::Publisher pub_joints;
	pub_joints = node.advertise<sensor_msgs::JointState>("/futaba_rs/joint_states",10);

	// サブスクライバの作成
	ros::Subscriber sub_joints;
	sub_joints = node.subscribe("/joint_states", 10, monitorJointState_callback);		// moveitから取る

	ros::Rate loop_rate(100);	// 100Hz

	while(ros::ok()){

		RSGetDataALL();					// サーボから値を取得
		Data2Jointstate();				// サーボから取り出した値をjointstateにセット
		pub_joints.publish(js_pub);		// 

		ros::spinOnce();   // コールバック関数を呼ぶ

		RS.sv_move_long(subData,SRVCNT);

    	loop_rate.sleep();
	}

	for(i=0;i<SRVCNT;i++){
		RS.sv_torque(id_list[i], 0);	// トルクオフ
	}

	RS.sv_close();// シリアルポートクローズ

	return 0;
}
