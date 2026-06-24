#include "turn_on_rai_robot/rai_robot.h"
#include "turn_on_rai_robot/Quaternion_Solution.h"
#include "rai_robot_msg/msg/data.hpp" 

sensor_msgs::msg::Imu Mpu6050;

using std::placeholders::_1;
using namespace std;
rclcpp::Node::SharedPtr node_handle = nullptr;


bool check_AutoCharge_data = false;
bool charge_set_state = false;





int main(int argc, char** argv)
{
  rclcpp::init(argc, argv); 
  turn_on_robot Robot_Control;
  Robot_Control.Control();
    return 0;  
} 






short turn_on_robot::IMU_Trans(uint8_t Data_High,uint8_t Data_Low)
{
  short transition_16;
  transition_16 = 0;
  transition_16 |=  Data_High<<8;   
  transition_16 |=  Data_Low;
  return transition_16;     
}
float turn_on_robot::Odom_Trans(uint8_t Data_High,uint8_t Data_Low)
{
  float data_return;
  short transition_16;
  transition_16 = 0;
  transition_16 |=  Data_High<<8;  
  transition_16 |=  Data_Low;      
  data_return   =  (transition_16 / 1000)+(transition_16 % 1000)*0.001; 
  return data_return;
}





void turn_on_robot::Cmd_Vel_Callback(const geometry_msgs::msg::Twist::SharedPtr twist_aux)
{
  short  transition;  

  Send_Data.tx[0]=FRAME_HEADER; 
  Send_Data.tx[1] = AutoRecharge; 
  Send_Data.tx[2] = 0; 

  
  
  transition=0;
  transition = twist_aux->linear.x*1000; 
  Send_Data.tx[4] = transition;     
  Send_Data.tx[3] = transition>>8;  

  
  
  transition=0;
  transition = twist_aux->linear.y*1000;
  Send_Data.tx[6] = transition;
  Send_Data.tx[5] = transition>>8;

  
  
  transition=0;
  transition = twist_aux->angular.z*1000;
  Send_Data.tx[8] = transition;
  Send_Data.tx[7] = transition>>8;

  Send_Data.tx[9]=Check_Sum(9,SEND_DATA_CHECK); 
  Send_Data.tx[10]=FRAME_TAIL; 
  try
  {
    Stm32_Serial.write(Send_Data.tx,sizeof (Send_Data.tx)); 
  }
  catch (serial::IOException& e)   
  {
    RCLCPP_ERROR(this->get_logger(),("Unable to send data through serial port")); 
  }
}





void turn_on_robot::Publish_ImuSensor()
{
  sensor_msgs::msg::Imu Imu_Data_Pub; 
  Imu_Data_Pub.header.stamp = rclcpp::Node::now(); 
  Imu_Data_Pub.header.frame_id = gyro_frame_id; 
                                                
  Imu_Data_Pub.orientation.x = Mpu6050.orientation.x; 
  Imu_Data_Pub.orientation.y = Mpu6050.orientation.y; 
  Imu_Data_Pub.orientation.z = Mpu6050.orientation.z;
  Imu_Data_Pub.orientation.w = Mpu6050.orientation.w;
  Imu_Data_Pub.orientation_covariance[0] = 1e6; 
  Imu_Data_Pub.orientation_covariance[4] = 1e6;
  Imu_Data_Pub.orientation_covariance[8] = 1e-6;
  Imu_Data_Pub.angular_velocity.x = Mpu6050.angular_velocity.x; 
  Imu_Data_Pub.angular_velocity.y = Mpu6050.angular_velocity.y;
  Imu_Data_Pub.angular_velocity.z = Mpu6050.angular_velocity.z;
  Imu_Data_Pub.angular_velocity_covariance[0] = 1e6; 
  Imu_Data_Pub.angular_velocity_covariance[4] = 1e6;
  Imu_Data_Pub.angular_velocity_covariance[8] = 1e-6;
  Imu_Data_Pub.linear_acceleration.x = Mpu6050.linear_acceleration.x; 
  Imu_Data_Pub.linear_acceleration.y = Mpu6050.linear_acceleration.y; 
  Imu_Data_Pub.linear_acceleration.z = Mpu6050.linear_acceleration.z;  
  imu_publisher->publish(Imu_Data_Pub); 
}





void turn_on_robot::Publish_Odom()
{
    
    
    tf2::Quaternion q;
    q.setRPY(0,0,Robot_Pos.Z);
    geometry_msgs::msg::Quaternion odom_quat=tf2::toMsg(q);
    
    nav_msgs::msg::Odometry odom; 
    odom.header.stamp = rclcpp::Node::now(); ; 
    odom.header.frame_id = odom_frame_id; 
    odom.pose.pose.position.x = Robot_Pos.X; 
    odom.pose.pose.position.y = Robot_Pos.Y;
    odom.pose.pose.position.z = Robot_Pos.Z;
    odom.pose.pose.orientation = odom_quat; 

    odom.child_frame_id = robot_frame_id; 
    odom.twist.twist.linear.x =  Robot_Vel.X; 
    odom.twist.twist.linear.y =  Robot_Vel.Y; 
    odom.twist.twist.angular.z = Robot_Vel.Z; 

    
    
    if(Robot_Vel.X== 0&&Robot_Vel.Y== 0&&Robot_Vel.Z== 0)
      
      
      memcpy(&odom.pose.covariance, odom_pose_covariance2, sizeof(odom_pose_covariance2)),
      memcpy(&odom.twist.covariance, odom_twist_covariance2, sizeof(odom_twist_covariance2));
    else
      
      
      memcpy(&odom.pose.covariance, odom_pose_covariance, sizeof(odom_pose_covariance)),
      memcpy(&odom.twist.covariance, odom_twist_covariance, sizeof(odom_twist_covariance));       
    odom_publisher->publish(odom); 
}





void turn_on_robot::Publish_Voltage()
{
    std_msgs::msg::Float32 voltage_msgs; 
    static float Count_Voltage_Pub=0;
    if(Count_Voltage_Pub++>10)
      {
        Count_Voltage_Pub=0;  
        voltage_msgs.data = Power_voltage; 
        voltage_publisher->publish(voltage_msgs); 
      }
}







void turn_on_robot::Publish_RED()
{
    std_msgs::msg::UInt8 msg;
    msg.data=Red;
    RED_publisher->publish(msg); 

}





void turn_on_robot::Publish_Charging()
{
    static bool last_charging;
    std_msgs::msg::Bool msg;
    msg.data=Charging;
    Charging_publisher->publish(msg); 
    if(last_charging==false && Charging==true)cout<<GREEN<<"Robot is charging."<<endl<<RESET;
    if(last_charging==true && Charging==false)cout<<RED  <<"Robot charging has disconnected."<<endl<<RESET;
    last_charging=Charging;
}





void turn_on_robot::Publish_ChargingCurrent()
{
    std_msgs::msg::Float32 msg; 
    msg.data=Charging_Current;
    Charging_current_publisher->publish(msg);
}






void turn_on_robot::Red_Vel_Callback(const geometry_msgs::msg::Twist::SharedPtr twist_aux)
{
  short  transition;  

  Send_Data.tx[0]=FRAME_HEADER; 

  Send_Data.tx[1] = 3; 
  Send_Data.tx[2] = 0; 

  
  
  transition=0;
  transition = twist_aux->linear.x*1000; 
  Send_Data.tx[4] = transition;     
  Send_Data.tx[3] = transition>>8;  

  
  
  transition=0;
  transition = twist_aux->linear.y*1000;
  Send_Data.tx[6] = transition;
  Send_Data.tx[5] = transition>>8;

  
  
  transition=0;
  transition = twist_aux->angular.z*1000;
  Send_Data.tx[8] = transition;
  Send_Data.tx[7] = transition>>8;

  Send_Data.tx[9]=Check_Sum(9,SEND_DATA_CHECK);  
  Send_Data.tx[10]=FRAME_TAIL; 
  try
  {
    Stm32_Serial.write(Send_Data.tx,sizeof (Send_Data.tx)); 
  }
  catch (serial::IOException& e)   
  {
    RCLCPP_ERROR(this->get_logger(),("Unable to send data through serial port")); 
  }
}






void turn_on_robot::Recharge_Flag_Callback(const std_msgs::msg::Int8::SharedPtr Recharge_Flag)
{
  AutoRecharge=Recharge_Flag->data;
}


void turn_on_robot::Set_Charge_Callback(const shared_ptr<turtlesim::srv::Spawn::Request> req,shared_ptr<turtlesim::srv::Spawn::Response> res)
{
    Send_Data.tx[0]=FRAME_HEADER; 

    if(round(req->x)==1)
      Send_Data.tx[1] = 1;
    else if(round(req->x)==2)
      Send_Data.tx[1] = 2; 
    else if(round(req->x)==0)
      Send_Data.tx[1] = 0,AutoRecharge=0;

    Send_Data.tx[2] = 0; 
    Send_Data.tx[3] = 0;  
    Send_Data.tx[4] = 0;   
    Send_Data.tx[5] = 0;
    Send_Data.tx[6] = 0;
    Send_Data.tx[7] = 0;
    Send_Data.tx[8] = 0;
    Send_Data.tx[9]=Check_Sum(9,SEND_DATA_CHECK); 
    Send_Data.tx[10]=FRAME_TAIL; 
    try
    {
      Stm32_Serial.write(Send_Data.tx,sizeof (Send_Data.tx)); 
    }
    catch (serial::IOException& e)   
    {
      res->name = "false";
    }

    if( Send_Data.tx[1]==0 )
    {
      if(charge_set_state==0)
        AutoRecharge=0,res->name = "true";
      else
        res->name = "false";
    }
    else
    {
      if(charge_set_state==1)
        res->name = "true";
      else
        res->name = "false";
    }
    return;
}









unsigned char turn_on_robot::Check_Sum(unsigned char Count_Number,unsigned char mode)
{
  unsigned char check_sum=0,k;
  
  if(mode==0) 
  {
   for(k=0;k<Count_Number;k++)
    {
     check_sum=check_sum^Receive_Data.rx[k]; 
     }
  }
  if(mode==1) 
  {
   for(k=0;k<Count_Number;k++)
    {
     check_sum=check_sum^Send_Data.tx[k]; 
     }
  }
  return check_sum; 
}


unsigned char turn_on_robot::Check_Sum_AutoCharge(unsigned char Count_Number,unsigned char mode)
{
  unsigned char check_sum=0,k;
  if(mode==0) 
  {
   for(k=0;k<Count_Number;k++)
    {
     check_sum=check_sum^Receive_AutoCharge_Data.rx[k]; 
    }
  }

  return check_sum;
}









bool turn_on_robot::Get_Sensor_Data()
{ 
  short transition_16=0, j=0, Header_Pos=0, Tail_Pos=0; 
  static int flag_error=0,temp=1; 
  uint8_t Receive_Data_Pr[RECEIVE_DATA_SIZE]={0},Receive_Data_Tr[temp]={0}; 
  if(flag_error==0) {
    try { Stm32_Serial.read(Receive_Data_Pr,sizeof (Receive_Data_Pr)); }
    catch (const serial::SerialException& e) {
        RCLCPP_WARN(this->get_logger(), "Serial read error: %s", e.what());
        return false;
    }
    }   
  else if (flag_error==1) {
    try { Stm32_Serial.read(Receive_Data_Tr,sizeof (Receive_Data_Tr)); }
    catch (const serial::SerialException& e) {
        RCLCPP_WARN(this->get_logger(), "Serial read error: %s", e.what());
        flag_error=0; return false;
    }}

  




  

  
  for(j=0;j<24;j++)
  {
    if(Receive_Data_Pr[j]==FRAME_HEADER)
    Header_Pos=j;
    else if(Receive_Data_Pr[j]==FRAME_TAIL)
    Tail_Pos=j;    
  }

  if(Tail_Pos==(Header_Pos+23))
  {
    
    
    
    memcpy(Receive_Data.rx, Receive_Data_Pr, sizeof(Receive_Data_Pr));
    flag_error=0; 
  }
  else if(Header_Pos==(1+Tail_Pos))
  {
    
    
    
    
    temp=Header_Pos; 
    flag_error=1; 
    return false;
  }
  else 
  {
    
    
    
    
    return false;
  }    
  





  Receive_Data.Frame_Header= Receive_Data.rx[0]; 
  Receive_Data.Frame_Tail= Receive_Data.rx[23];  

  if (Receive_Data.Frame_Header == FRAME_HEADER ) 
  {
    if (Receive_Data.Frame_Tail == FRAME_TAIL) 
    { 
      if (Receive_Data.rx[22] == Check_Sum(22,READ_DATA_CHECK)) 
      {
        Receive_Data.Flag_Stop=Receive_Data.rx[1]; 
        Robot_Vel.X = Odom_Trans(Receive_Data.rx[2],Receive_Data.rx[3]); 
        Robot_Vel.Y = Odom_Trans(Receive_Data.rx[4],Receive_Data.rx[5]); 
                                                                         
        Robot_Vel.Z = Odom_Trans(Receive_Data.rx[6],Receive_Data.rx[7]); 
        
        
        
        Mpu6050_Data.accele_x_data = IMU_Trans(Receive_Data.rx[8],Receive_Data.rx[9]);   
        Mpu6050_Data.accele_y_data = IMU_Trans(Receive_Data.rx[10],Receive_Data.rx[11]); 
        Mpu6050_Data.accele_z_data = IMU_Trans(Receive_Data.rx[12],Receive_Data.rx[13]); 
        Mpu6050_Data.gyros_x_data = IMU_Trans(Receive_Data.rx[14],Receive_Data.rx[15]);  
        Mpu6050_Data.gyros_y_data = IMU_Trans(Receive_Data.rx[16],Receive_Data.rx[17]);  
        Mpu6050_Data.gyros_z_data = IMU_Trans(Receive_Data.rx[18],Receive_Data.rx[19]);  
        
        
        Mpu6050.linear_acceleration.x = Mpu6050_Data.accele_x_data / ACCEl_RATIO;
        Mpu6050.linear_acceleration.y = Mpu6050_Data.accele_y_data / ACCEl_RATIO;
        Mpu6050.linear_acceleration.z = Mpu6050_Data.accele_z_data / ACCEl_RATIO;
        
        
        
        
        Mpu6050.angular_velocity.x =  Mpu6050_Data.gyros_x_data * GYROSCOPE_RATIO;
        Mpu6050.angular_velocity.y =  Mpu6050_Data.gyros_y_data * GYROSCOPE_RATIO;
        Mpu6050.angular_velocity.z =  Mpu6050_Data.gyros_z_data * GYROSCOPE_RATIO;

        
        
        transition_16 = 0;
        transition_16 |=  Receive_Data.rx[20]<<8;
        transition_16 |=  Receive_Data.rx[21];  
        Power_voltage = transition_16/1000+(transition_16 % 1000)*0.001; 

        return true;
     }
    }
  } 
  return false;
}





bool turn_on_robot::Get_Sensor_Data_New()
{
  short transition_16=0; 
  
  uint8_t check=0,check2=0, error=1,error2=1,Receive_Data_Pr[1]; 
  static int count,count2; 
  try {
    Stm32_Serial.read(Receive_Data_Pr,sizeof(Receive_Data_Pr));
} catch (const serial::SerialException& e) {
    RCLCPP_WARN(this->get_logger(), "Serial read error (New): %s", e.what());
    return false;
} catch (const serial::IOException& e) {
    RCLCPP_WARN(this->get_logger(), "Serial IO error (New): %s", e.what());
    return false;
}
  




  

  Receive_Data.rx[count] = Receive_Data_Pr[0]; 
  Receive_AutoCharge_Data.rx[count2] = Receive_Data_Pr[0];

  Receive_Data.Frame_Header = Receive_Data.rx[0]; 
  Receive_Data.Frame_Tail = Receive_Data.rx[23];  

  
  if((Receive_Data_Pr[0] == AutoCharge_HEADER )||count2>0)
    count2++;
  else
    count2=0; 

  if(Receive_Data_Pr[0] == FRAME_HEADER || count>0) 
    count++;
  else 
    count=0;

  
  if(count2 == AutoCharge_DATA_SIZE)
  {
    count2=0;
    if(Receive_AutoCharge_Data.rx[AutoCharge_DATA_SIZE-1]==AutoCharge_TAIL) 
    {
      check2 =  Check_Sum_AutoCharge(6,0);
      if(check2 == Receive_AutoCharge_Data.rx[AutoCharge_DATA_SIZE-2]) 
      {
        error2=0;
      }
      if(error2 == 0)  
      {
        transition_16 = 0;
        transition_16   |=  Receive_AutoCharge_Data.rx[1]<<8;
        transition_16   |=  Receive_AutoCharge_Data.rx[2]; 
        Charging_Current = transition_16/1000+(transition_16 % 1000)*0.001; 
        
        Red =  Receive_AutoCharge_Data.rx[3];    
        Charging = Receive_AutoCharge_Data.rx[4];

        charge_set_state = Receive_AutoCharge_Data.rx[5];

        check_AutoCharge_data = true; 
      }
    }
  }

  if(count == 24) 
  {
    count=0;  
    if(Receive_Data.Frame_Tail == FRAME_TAIL) 
    {
      check=Check_Sum(22,READ_DATA_CHECK);  

      if(check == Receive_Data.rx[22])  
      {
        error=0;  
      }
      if(error == 0)
      {
        






        Receive_Data.Flag_Stop=Receive_Data.rx[1]; 
        Robot_Vel.X = Odom_Trans(Receive_Data.rx[2],Receive_Data.rx[3]); 
          
        Robot_Vel.Y = Odom_Trans(Receive_Data.rx[4],Receive_Data.rx[5]); 
                                                                          
        Robot_Vel.Z = Odom_Trans(Receive_Data.rx[6],Receive_Data.rx[7]); 
          
        
        
        Mpu6050_Data.accele_x_data = IMU_Trans(Receive_Data.rx[8],Receive_Data.rx[9]);   
        Mpu6050_Data.accele_y_data = IMU_Trans(Receive_Data.rx[10],Receive_Data.rx[11]); 
        Mpu6050_Data.accele_z_data = IMU_Trans(Receive_Data.rx[12],Receive_Data.rx[13]); 
        Mpu6050_Data.gyros_x_data = IMU_Trans(Receive_Data.rx[14],Receive_Data.rx[15]);  
        Mpu6050_Data.gyros_y_data = IMU_Trans(Receive_Data.rx[16],Receive_Data.rx[17]);  
        Mpu6050_Data.gyros_z_data = IMU_Trans(Receive_Data.rx[18],Receive_Data.rx[19]);  
        
        
        Mpu6050.linear_acceleration.x = Mpu6050_Data.accele_x_data / ACCEl_RATIO;
        Mpu6050.linear_acceleration.y = Mpu6050_Data.accele_y_data / ACCEl_RATIO;
        Mpu6050.linear_acceleration.z = Mpu6050_Data.accele_z_data / ACCEl_RATIO;
        
        
        
        
        Mpu6050.angular_velocity.x =  Mpu6050_Data.gyros_x_data * GYROSCOPE_RATIO;
        Mpu6050.angular_velocity.y =  Mpu6050_Data.gyros_y_data * GYROSCOPE_RATIO;
        Mpu6050.angular_velocity.z =  Mpu6050_Data.gyros_z_data * GYROSCOPE_RATIO;

        
        
        transition_16 = 0;
        transition_16 |=  Receive_Data.rx[20]<<8;
        transition_16 |=  Receive_Data.rx[21];  
        Power_voltage = transition_16/1000+(transition_16 % 1000)*0.001; 
          
        return true;
      }
    }
  }
  return false;
}





void turn_on_robot::Control()
{
  
  _Last_Time = rclcpp::Node::now();
  while(rclcpp::ok())
  {
    try
    {
    
    _Now = rclcpp::Node::now();
    Sampling_Time = (_Now - _Last_Time).seconds();  
                                                 
    if (true == Get_Sensor_Data_New()) 
                                   
    {
      
      Robot_Vel.X = Robot_Vel.X*odom_x_scale;
      Robot_Vel.Y = Robot_Vel.Y*odom_y_scale;
      if( Robot_Vel.Z>=0 )
        Robot_Vel.Z = Robot_Vel.Z*odom_z_scale_positive;
      else
        Robot_Vel.Z = Robot_Vel.Z*odom_z_scale_negative;

      Robot_Pos.X+=(Robot_Vel.X * cos(Robot_Pos.Z) - Robot_Vel.Y * sin(Robot_Pos.Z)) * Sampling_Time; 
      Robot_Pos.Y+=(Robot_Vel.X * sin(Robot_Pos.Z) + Robot_Vel.Y * cos(Robot_Pos.Z)) * Sampling_Time; 
      Robot_Pos.Z+=Robot_Vel.Z * Sampling_Time; 

      
      
      Quaternion_Solution(Mpu6050.angular_velocity.x, Mpu6050.angular_velocity.y, Mpu6050.angular_velocity.z,\
                Mpu6050.linear_acceleration.x, Mpu6050.linear_acceleration.y, Mpu6050.linear_acceleration.z);

      Publish_Odom();      
      Publish_ImuSensor(); 
      Publish_Voltage();   

      _Last_Time = _Now; 
      
    }
    
    
    if(check_AutoCharge_data)
    {
      Publish_Charging();  
      Publish_RED();       
      Publish_ChargingCurrent(); 
      check_AutoCharge_data = false;
    }

    rclcpp::spin_some(this->get_node_base_interface());
    }
    catch (const serial::SerialException & e) {
      RCLCPP_WARN(this->get_logger(), "Serial exception in Control loop: %s", e.what());
      try { Stm32_Serial.close(); } catch(...) {}
      rclcpp::sleep_for(std::chrono::milliseconds(500));
      try { Stm32_Serial.open(); } catch(...) {}
    }
    catch (const serial::IOException & e) {
      RCLCPP_WARN(this->get_logger(), "Serial IO error in Control loop: %s", e.what());
    }
    catch (const rclcpp::exceptions::RCLError & e) {
      RCLCPP_ERROR(this->get_logger(), "RCL error: %s", e.what());
    }
  }  
}    





turn_on_robot::turn_on_robot():rclcpp::Node ("rai_robot")
{
  Sampling_Time=0;
  Power_voltage=0;
  
  
  memset(&Robot_Pos, 0, sizeof(Robot_Pos));
  memset(&Robot_Vel, 0, sizeof(Robot_Vel));
  memset(&Receive_Data, 0, sizeof(Receive_Data)); 
  memset(&Send_Data, 0, sizeof(Send_Data));
  memset(&Mpu6050_Data, 0, sizeof(Mpu6050_Data));

  
  
  
  
  this->declare_parameter<int>("serial_baud_rate");
  this->declare_parameter<std::string>("usart_port_name", "/dev/rai_controller");
  this->declare_parameter<std::string>("odom_frame_id", "odom");
  this->declare_parameter<std::string>("robot_frame_id", "base_footprint");
  this->declare_parameter<std::string>("gyro_frame_id", "gyro_link");
  this->declare_parameter<double>("odom_x_scale");
  this->declare_parameter<double>("odom_y_scale");
  this->declare_parameter<double>("odom_z_scale_positive");
  this->declare_parameter<double>("odom_z_scale_negative");

  this->get_parameter("serial_baud_rate", serial_baud_rate);
  this->get_parameter("usart_port_name", usart_port_name);
  this->get_parameter("odom_frame_id", odom_frame_id);
  this->get_parameter("robot_frame_id", robot_frame_id);
  this->get_parameter("gyro_frame_id", gyro_frame_id);
  this->get_parameter("odom_x_scale", odom_x_scale);
  this->get_parameter("odom_y_scale", odom_y_scale);
  this->get_parameter("odom_z_scale_positive", odom_z_scale_positive);
  this->get_parameter("odom_z_scale_negative", odom_z_scale_negative);

  odom_publisher = create_publisher<nav_msgs::msg::Odometry>("odom", 2);
  imu_publisher = create_publisher<sensor_msgs::msg::Imu>("imu/data_raw", 2); 
  voltage_publisher = create_publisher<std_msgs::msg::Float32>("PowerVoltage", 1);

  
  Charging_publisher = create_publisher<std_msgs::msg::Bool>("robot_charging_flag", 10);
  Charging_current_publisher = create_publisher<std_msgs::msg::Float32>("robot_charging_current", 10);
  RED_publisher = create_publisher<std_msgs::msg::UInt8>("robot_red_flag", 10);
  
  Red_Vel_Sub = create_subscription<geometry_msgs::msg::Twist>(
      "red_vel", 10, std::bind(&turn_on_robot::Red_Vel_Callback, this, std::placeholders::_1));
  Recharge_Flag_Sub = create_subscription<std_msgs::msg::Int8>(
      "robot_recharge_flag", 10, std::bind(&turn_on_robot::Recharge_Flag_Callback, this,std::placeholders::_1));
  
  SetCharge_Service=this->create_service<turtlesim::srv::Spawn>\
  ("/set_charge",std::bind(&turn_on_robot::Set_Charge_Callback,this, \ 
    std::placeholders::_1 ,std::placeholders::_2));
    
  
  
  Cmd_Vel_Sub = create_subscription<geometry_msgs::msg::Twist>(
      "cmd_vel", 2, std::bind(&turn_on_robot::Cmd_Vel_Callback, this, _1));

  RCLCPP_INFO(this->get_logger(),"rai_robot Data ready"); 

  try
  { 
    
    Stm32_Serial.setPort(usart_port_name); 
    Stm32_Serial.setBaudrate(serial_baud_rate); 
    serial::Timeout _time = serial::Timeout::simpleTimeout(2000); 
    Stm32_Serial.setTimeout(_time);
    Stm32_Serial.open(); 
    Stm32_Serial.flushInput();
  }
  catch (serial::IOException& e)
  {
    RCLCPP_ERROR(this->get_logger(),"rai_robot can not open serial port,Please check the serial port cable! "); 
  }
  if(Stm32_Serial.isOpen())
  {
    RCLCPP_INFO(this->get_logger(),"rai_robot serial port opened"); 
  }
}





turn_on_robot::~turn_on_robot()
{
  
  
  Send_Data.tx[0]=FRAME_HEADER;
  Send_Data.tx[1] = 0;  
  Send_Data.tx[2] = 0; 

  
  Send_Data.tx[4] = 0;     
  Send_Data.tx[3] = 0;  

  
  Send_Data.tx[6] = 0;
  Send_Data.tx[5] = 0;  

  
  Send_Data.tx[8] = 0;  
  Send_Data.tx[7] = 0;    
  Send_Data.tx[9]=Check_Sum(9,SEND_DATA_CHECK); 
  Send_Data.tx[10]=FRAME_TAIL; 
  try
  {
    Stm32_Serial.write(Send_Data.tx,sizeof (Send_Data.tx)); 
  }
  catch (serial::IOException& e)   
  {
    RCLCPP_ERROR(this->get_logger(),"Unable to send data through serial port"); 
  }
  Stm32_Serial.close(); 
  RCLCPP_INFO(this->get_logger(),"Shutting down"); 
}

