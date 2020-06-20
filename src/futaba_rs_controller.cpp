#include <ros/ros.h>

#include "sensor_msgs/JointState.h"
#include "futaba_rs.h"
#include "futaba_rs_controller/FutabaRsState.h"

// 設定はここから -----------------------------------------------

// デフォルトのシリアルポート デバイス名
// (※起動時のrosparamでも変更可能)
#define SERIAL_PORT "/dev/ttyUSB0"

// デフォルトのシリアルポート ボーレート
// (※起動時のrosparamでも変更可能)
#define SERIAL_BAUD 460800

// サーボにコマンドを送る周期(Hz)＆サーボの状態をjoint_statesでpublishする周期(Hz)
// この周期に合わせて、サーボの角速度も計算
// (※ subscriberは非同期なので、この周期に影響されない)
// (※起動時のrosparamでも変更可能)
#define CTL_RATE 10

// 接続するサーボの個数 ☆☆☆☆☆☆ ロボットに合わせて要変更
#define SRVCNT 19

// 接続する双葉サーボモーターのIDの列挙 ☆☆☆☆☆☆ ロボットに合わせて要変更
// (※ 上記のサーボの個数と一致すること)
int id_list[SRVCNT]={61 ,62 ,63 ,64 ,65 ,66 ,67 ,68 ,71 ,72 ,73 ,74 ,75 , 76 ,77 ,78 ,91 ,92 ,93};

// JointState::nameの列挙、順番は上記のモーターIDを対応するように並べる ☆☆☆☆☆☆ ロボットに合わせて要変更
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

// 設定はここまで -----------------------------------------------

int ctl_rate;
frs_t receivedData[SRVCNT];
frs_t subData[SRVCNT];
futaba_rs frs;

sensor_msgs::JointState js_pub;
sensor_msgs::JointState js_sub;
futaba_rs_controller::FutabaRsState frstate_pub;

void RSGetDataALL(void)
{
	int i;
	frs_t rData[SRVCNT];		// 受信データ
	for (i = 0; i < SRVCNT; i++) {
		rData[i] = frs.sv_read2(id_list[i]);

		if (rData[i].error == NO_ERROR){
			receivedData[i].id			= id_list[i];	// 受信データをパブリッシュ用データに変換
			receivedData[i].angle		= rData[i].angle;
			receivedData[i].load			= rData[i].load;
			receivedData[i].speed		= rData[i].speed;
			receivedData[i].temperature 	= rData[i].temperature;
			receivedData[i].time			= rData[i].time;
			receivedData[i].error		= rData[i].error;
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
		js_pub.position[i] = (float)2.0*M_PI*(receivedData[i].angle / 10.0)/360.0;		// deg 2 rad
		js_pub.velocity[i] = (float)2.0*M_PI*receivedData[i].speed / 360.0;				// deg/s 2 rad/s
		js_pub.effort[i] = receivedData[i].load;
	}

	frstate_pub.angle.resize(SRVCNT);
	frstate_pub.error.resize(SRVCNT);
	frstate_pub.id.resize(SRVCNT);
	frstate_pub.load.resize(SRVCNT);
	frstate_pub.speed.resize(SRVCNT);
	frstate_pub.temperature.resize(SRVCNT);
	frstate_pub.time.resize(SRVCNT);
	
	for(i=0;i<SRVCNT;i++){
		frstate_pub.angle[i] = receivedData[i].angle;
		frstate_pub.error[i] = receivedData[i].error;
		frstate_pub.id[i] = receivedData[i].id;
		frstate_pub.load[i] = receivedData[i].load;
		frstate_pub.speed[i] = receivedData[i].speed;
		frstate_pub.temperature[i] = receivedData[i].temperature;
		frstate_pub.time[i] = receivedData[i].time;
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
				subData[i].g_time = (unsigned short)(100/ctl_rate);	// サーボの移動速度 単位はn*10msec
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

	ros::init(argc, argv, "futaba_rs_controller");
	ros::NodeHandle node;

	ros::NodeHandle private_nh("~");

	port_name = SERIAL_PORT;
	private_nh.getParam("port_name",port_name);

	baud_rate = SERIAL_BAUD;
	private_nh.getParam("baud_rate",baud_rate);

	ctl_rate = CTL_RATE;
	private_nh.getParam("ctl_rate",ctl_rate);

	ROS_INFO("port open ( %s )  baudrate ( %d )\n",port_name.data(),baud_rate);

	//	COMポート RS232Cの初期化
	if (frs.init(port_name.data(), baud_rate) != 0) {
		ROS_ERROR("can't open serial port ( %s ) \n",port_name.data());
		return 0;
	}; 

	for(i=0;i<SRVCNT;i++){
		result = frs.sv_read_torque(id_list[i]);	// トルク読み出し(ping代わり)
		if (result >= 0){
			ROS_INFO("Succeeded to connet (id : %d ) torque : %d", id_list[i], result);
		}else{
			ROS_ERROR("Failed to connet (id : %d ) error : %d", id_list[i], result);
			frs.sv_close();
			return 0;
		}
	}

	for(i=0;i<SRVCNT;i++){
		result = frs.sv_torque(id_list[i], 1);	// トルクオン
		if (result == 9){
			ROS_INFO("Succeeded to torque on id : %d", id_list[i]);
		}else{
			ROS_ERROR("Failed to rtorque on id : %d", id_list[i]);	
		}
	}

	// 初期化ポーズ(全サーボ原点0)
	frs_t init_pose[SRVCNT];
	for (i = 0; i < SRVCNT; i++) {
		init_pose[i].id	= id_list[i];
		init_pose[i].g_angle = 0;
		init_pose[i].g_time = 100;	// 100*10msec		
	}
	frs.sv_move_long(init_pose,SRVCNT);

	//パブリッシャの作成 (joint_states)
	ros::Publisher pub_joints;
	pub_joints = node.advertise<sensor_msgs::JointState>("/futaba_rs_controller/joint_states",10);

	//パブリッシャの作成 (futaba_rs_states)
	ros::Publisher pub_frstate;
	pub_frstate = node.advertise<futaba_rs_controller::FutabaRsState>("/futaba_rs_controller/futaba_rs_states",10);

	// サブスクライバの作成
	ros::Subscriber sub_joints;
	sub_joints = node.subscribe("/futaba_rs_controller/goal_joint_states", 10, monitorJointState_callback);	

    ros::AsyncSpinner spinner(1);	// spinを処理するスレッド数を引数に渡す
    spinner.start();				// subscriberは別スレッドでひたすらspin

	ros::Rate loop_rate(ctl_rate);	// ROSメインループ周期

	while(ros::ok()){

		frs.sv_move_long(subData,SRVCNT);

		RSGetDataALL();					// サーボから値を取得
		Data2Jointstate();				// サーボから取り出した値をjointstateとfrstateにセット
		pub_joints.publish(js_pub);		// jointstateをpublish
		pub_frstate.publish(frstate_pub);	// frstateをpublish

		//ros::spinOnce();   			// コールバック関数を呼ぶ AsyncSpinnerにつき不要
    	loop_rate.sleep();
	}

	for(i=0;i<SRVCNT;i++){
		frs.sv_torque(id_list[i], 0);	// トルクオフ
	}

	frs.sv_close();// シリアルポートクローズ

	return 0;
}
