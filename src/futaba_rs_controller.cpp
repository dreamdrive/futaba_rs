#include <ros/ros.h>
#include <yaml-cpp/yaml.h>

#include "sensor_msgs/JointState.h"
#include "futaba_rs_controller/futaba_rs.h"
#include "futaba_rs_controller/FutabaRsState.h"

// 設定はここから -----------------------------------------------

// デフォルトのシリアルポート デバイス名
// (※起動時のrosparamでも変更可能)
#define SERIAL_PORT "/dev/ttyUSB0"

// デフォルトのシリアルポート ボーレート
// (※起動時のrosparamでも変更可)
#define SERIAL_BAUD 460800

// サーボにコマンドを送る周期(Hz)＆サーボの状態をjoint_statesでpublishする周期(Hz)
// この周期に合わせて、サーボの角速度も計算
// (※ subscriberは非同期なので、この周期に影響されない)
// (※起動時のrosparamでも変更可能)
#define CTL_RATE_HZ 10

// サーボの最大個数 (接続するサーボの最大個数yamlに記述するサーボ数を下回らなければ問題ない)
#define MAX_SRVCNT 99

// ジョイント名に使える名前の文字数の上限
#define JOINT_NAME_WC 30

// 設定はここまで -----------------------------------------------

int ctl_rate_hz;					// 制御周期
int srvcnt = 0;						// サーボモーターの個数
frs_t receivedData[MAX_SRVCNT];		// サーボから受信したデータ
frs_t subData[MAX_SRVCNT];			// 購読したデータ

int id_list[MAX_SRVCNT];						// IDリスト
char joint_list[MAX_SRVCNT][JOINT_NAME_WC];		// ジョイント名リスト

futaba_rs Futaba_RS;

sensor_msgs::JointState js_pub;
sensor_msgs::JointState js_sub;
futaba_rs_controller::FutabaRsState frstate_pub;

void RSGetDataALL(void)
{
	int i;
	frs_t rData[srvcnt];		// 受信データ
	for (i = 0; i < srvcnt; i++) {
		rData[i] = Futaba_RS.sv_read2(id_list[i]);

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

	js_pub.name.resize(srvcnt);
	js_pub.position.resize(srvcnt);
	js_pub.velocity.resize(srvcnt);
	js_pub.effort.resize(srvcnt);
	
	for(i=0;i<srvcnt;i++){
		js_pub.name[i] = joint_list[i];
		js_pub.position[i] = (float)2.0*M_PI*(receivedData[i].angle / 10.0)/360.0;		// deg 2 rad
		js_pub.velocity[i] = (float)2.0*M_PI*receivedData[i].speed / 360.0;				// deg/s 2 rad/s
		js_pub.effort[i] = receivedData[i].load;
	}

	frstate_pub.angle.resize(srvcnt);
	frstate_pub.error.resize(srvcnt);
	frstate_pub.id.resize(srvcnt);
	frstate_pub.load.resize(srvcnt);
	frstate_pub.speed.resize(srvcnt);
	frstate_pub.temperature.resize(srvcnt);
	frstate_pub.time.resize(srvcnt);
	
	for(i=0;i<srvcnt;i++){
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
	for(i=0;i<srvcnt;i++){
		for(j=0;j<srvcnt;j++){
			if(joint_list[i] == jointstate->name[j]){
				subData[i].id = id_list[i];
				subData[i].g_angle =(short)  3600.0 * (jointstate->position[j]/(2.0*M_PI));
				subData[i].g_time = (unsigned short)(100/ctl_rate_hz);	// サーボの移動速度 単位はn*10msec
			}
		}
	}
}

//
// YAMLの読み込み
// getDynamixelsInfo(dynamixel_hardware.h)から拝借
//
bool getDynamixelsInfo(const std::string yaml_file){
	int cnt_frsv = 0;
	YAML::Node dynamixel;
	dynamixel = YAML::LoadFile(yaml_file.c_str());

	if (dynamixel == NULL)
		return false;

	for (YAML::const_iterator it_file = dynamixel.begin(); it_file != dynamixel.end(); it_file++)
		{
		std::string name = it_file->first.as<std::string>();
		if (name.size() == 0)
			{
			continue;
			}

		YAML::Node item = dynamixel[name];
		for (YAML::const_iterator it_item = item.begin(); it_item != item.end(); it_item++)
			{
			std::string item_name = it_item->first.as<std::string>();
			int32_t value = it_item->second.as<int32_t>();

			if (item_name == "ID"){
					id_list[cnt_frsv] = value;								// サーボのIDを列挙
					sprintf(joint_list[cnt_frsv],"%s",name.data());			// ジョイント名を列挙
					cnt_frsv++;												// ジョイント数＝サーボ数　をカウント
				}
			}
		}
		srvcnt = cnt_frsv;					// 接続されたサーボの個数を確定
	return true;
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

	ctl_rate_hz = CTL_RATE_HZ;
	private_nh.getParam("ctl_rate_hz",ctl_rate_hz);

    std::string yaml_file;// = "~/catkin_ws/src/futaba_rs/config/example.yaml";
    //private_nh.param<std::string>("futaba_rs_info", yaml_file, "/home/hirokazu/catkin_ws/src/futaba_rs/config/upperbody2.yaml");
	private_nh.getParam("/futaba_rs_info",yaml_file);

	ROS_INFO("YAML FILE :  ( %s )",yaml_file.data());

    result = false;
    result = getDynamixelsInfo(yaml_file);
    if (result == false){
    	ROS_ERROR("Please check YAML file");
    	return 0;
    }

	ROS_INFO("port open ( %s )  baudrate ( %d )",port_name.data(),baud_rate);

	//	COMポート RS232Cの初期化
	if (Futaba_RS.init(port_name.data(), baud_rate) != 0) {
		ROS_ERROR("can't open serial port ( %s )",port_name.data());
		return 0;
	}; 

	for(i=0;i<srvcnt;i++){
		result = Futaba_RS.sv_read_torque(id_list[i]);	// トルク読み出し(ping代わり)
		if (result >= 0){
			ROS_INFO("Succeeded to connet (id : %d ) torque : %d", id_list[i], result);
		}else{
			ROS_ERROR("Failed to connet (id : %d ) error : %d", id_list[i], result);
			Futaba_RS.sv_close();
			return 0;
		}
	}

	for(i=0;i<srvcnt;i++){
		result = Futaba_RS.sv_torque(id_list[i], 1);	// トルクオン
		if (result == 9){
			ROS_INFO("Succeeded to torque on id : %d", id_list[i]);
		}else{
			ROS_ERROR("Failed to rtorque on id : %d", id_list[i]);	
		}
	}

	// 初期化ポーズ(全サーボ原点0)
	frs_t init_pose[srvcnt];
	for (i = 0; i < srvcnt; i++) {
		init_pose[i].id	= id_list[i];
		init_pose[i].g_angle = 0;
		init_pose[i].g_time = 100;	// 100*10msec		
	}
	Futaba_RS.sv_move_long(init_pose,srvcnt);

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

	ros::Rate loop_rate(ctl_rate_hz);	// ROSメインループ周期

	while(ros::ok()){
		Futaba_RS.sv_move_long(subData,srvcnt);

		RSGetDataALL();					// サーボから値を取得
		Data2Jointstate();				// サーボから取り出した値をjointstateとfrstateにセット
		pub_joints.publish(js_pub);		// jointstateをpublish
		pub_frstate.publish(frstate_pub);	// frstateをpublish

		//ros::spinOnce();   			// コールバック関数を呼ぶ AsyncSpinnerにつき不要
    	loop_rate.sleep();
	}

	for(i=0;i<srvcnt;i++){
		Futaba_RS.sv_torque(id_list[i], 0);	// トルクオフ
	}

	Futaba_RS.sv_close();// シリアルポートクローズ

	return 0;
}
