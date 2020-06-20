# futaba_rs_controller パッケージ
 FUTABAサーボ


$ roslaunch futaba_rs_controller futaba_rs_controller.launch 


/futaba_rs_controller/goal_joint_states
をサブスクライブして、サーボモーターを動作実行します。
サブスクライブは、専用スレッドでノーウェイトでスピンしています。

ただし、rosparamで与えられた制御周期のみサーボを動作させるため、動作実行の際は、一番直近のサブスクライブした値を反映します

実行後のサーボモーターの状況は

/futaba_rs_controller/joint_states
と
/futaba_rs_controller/futaba_rs_states
に、パブリッシュされます。
futaba_rs_statesは、サーボモーターのメモリマップ上の生の値が入っています。

パブリッシュされる周期は、rosparamで与えた制御周期と一致します。



入力 : jointstates
出力 : jointstates


使用する場合は、
futaba_rs_controller.cpp
で、サーボモーターの個数
サーボモーターのIDの列挙
関節に割り当てる関節名(JointStateで使用する)を書き換える必要があります。

futaba_rs_controller.launch 

  <arg name="port_name"                default="/dev/ttyUSB0"/>
  <arg name="baud_rate"                default="460800"/>
  <arg name="ctl_rate"                default="10"/>

