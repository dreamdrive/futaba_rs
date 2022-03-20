# futaba_rs_controller パッケージ

複数個のFUTABA製のサーボをROSのjoint_statesを使って、ドライブするパッケージです。  
サーボモーターの定義はyamlで記述しています。  

## 実行方法

### ビルド

ROSワークスペースのsrcフォルダの中にクローンして、catkin_make

```
$ cd ~/catkin_ws/src
$ git clone https://github.com/dreamdrive/futaba_rs.git
$ cd ..
$ catkin_make
```

### 実行

```
$ roslaunch futaba_rs_controller futaba_rs_controller.launch 
```

### 事前のシリアルポート設定

事前にFTDIのレイテンシタイマを1msecに設定しておくことをオススメします。

```
 $ sudo chmod a+w /sys/bus/usb-serial/devices/ttyUSB0/latency_timer
 $ echo 1 > /sys/bus/usb-serial/devices/ttyUSB0/latency_timer
```

## launchファイル設定

futaba_rs_controller.launch 内のros paramで、シリアルポート名、ボーレート、制御周期(Hz)が変更できます。
制御周期の最大値は、サーボ数に依存します。

```
  <arg name="port_name"                default="/dev/ttyUSB0"/>
  <arg name="baud_rate"                default="460800"/>
  <arg name="ctl_rate_hz"              default="10"/>
```

## 概要

topic "/futaba_rs_controller/goal_joint_states"を購読して、サーボモーターを動作実行します。 
joint_statesのsubcriberは専用スレッドでノーウェイトでスピンしています。

jointstateの情報は、rosparamの「ctl_rate_hz」で指定した制御周期でサーボモーターに配信されます。 
制御周期より速い周期で配信されたjointstateの古いデータは破棄されて、一番新しいjointstateが、サーボモーターに反映されます。

同じ制御周期で、現在のサーボモーターの状況が、topic "/futaba_rs_controller/joint_states"とtopic "/futaba_rs_controller/futaba_rs_states"に、配信されます、

購読するjoint_statesは、"position"しか見ていませんが、配信するjoint_statesは、"position"に加えて"velocity"と"effort"も返します。
"effort"には、電流値を入れています。
また、futaba_rs_statesは、オリジナルメッセージで、サーボモーターのメモリマップ上の生の値が入っています。


## オリジナルメッセージ : futaba_rs_states

```
int32[] id          # サーボID
int16[] angle       # 現在の角度 (サーボから受信した値)
int16[] time        # 移動時間 (サーボから受信した値)
int16[] speed       # 角速度 (サーボから受信した値)
int16[] load        # 負荷 (サーボから受信した値)
int16[] temperature # 温度 (サーボから受信した値)
uint16[] error      # エラー
```

## ジョイント名とサーボIDの定義ファイル(YAML)

ジョイント名とサーボIDの組み合わせは、`config`フォルダのyamlファイルに記述します。
```
joint_name:
  ID: xx
```

という形式で、列挙します。

## 履歴
* 2020/06/26 :  とりあえず、リリース

## Author
Onomichi Hirokazu (Micchy)  
http://dream-drive.net  
Tailway Software.  
https://tailway.net/

## License
This is under MIT License.  
また、本パッケージには、[dynamixel_workbench_controllers](http://wiki.ros.org/dynamixel_workbench_controllers)を参考にしている部分が多く含まれます。
