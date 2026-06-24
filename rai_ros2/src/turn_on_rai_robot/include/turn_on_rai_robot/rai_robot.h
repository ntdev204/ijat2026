
#ifndef __RAI_ROBOT_H_
#define __RAI_ROBOT_H_

#include <iostream>
#include <string.h>
#include <string> 
#include <iostream>
#include <math.h> 
#include <stdlib.h>    
#include <unistd.h> 

#include "rclcpp/rclcpp.hpp"
#include <rcl/types.h>
#include <sys/stat.h>
#include <fcntl.h>          
#include <stdbool.h>
      
#include <sys/types.h>

#include <serial/serial.h>


#include <tf2_ros/transform_broadcaster.h>
#include "tf2_geometry_msgs/tf2_geometry_msgs.hpp"

#include "std_msgs/msg/string.hpp"
#include <std_msgs/msg/float32.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <geometry_msgs/msg/pose.hpp>
#include <geometry_msgs/msg/vector3.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include "rai_robot_msg/msg/data.hpp" 
#include "rai_robot_msg/msg/supersonic.hpp"


#include <std_msgs/msg/int8.hpp>
#include <std_msgs/msg/bool.hpp>
#include <std_msgs/msg/u_int8.hpp>
#include <turtlesim/srv/spawn.hpp>

using namespace std;

#define RESET   string("\033[0m")
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define PURPLE  "\033[35m"
#define CYAN    "\033[36m"



#define SEND_DATA_CHECK   1          
#define READ_DATA_CHECK   0          
#define FRAME_HEADER      0X7B       
#define FRAME_TAIL        0X7D       
#define RECEIVE_DATA_SIZE 24         
#define SEND_DATA_SIZE    11         
#define PI 				  3.1415926f 

#define Distance_DATA_size 19
#define Distance_HEADER    0XFA 
#define Distance_TAIL      0XFC 


#define AutoCharge_HEADER      0X7C 
#define AutoCharge_TAIL        0X7F 
#define AutoCharge_DATA_SIZE    8   





#define GYROSCOPE_RATIO   0.00026644f




#define ACCEl_RATIO 	  1671.84f  	

extern sensor_msgs::msg::Imu Mpu6050;  



const double odom_pose_covariance[36]   = {1e-3,    0,    0,   0,   0,    0, 
										      0, 1e-3,    0,   0,   0,    0,
										      0,    0,  1e6,   0,   0,    0,
										      0,    0,    0, 1e6,   0,    0,
										      0,    0,    0,   0, 1e6,    0,
										      0,    0,    0,   0,   0,  1e3 };

const double odom_pose_covariance2[36]  = {1e-9,    0,    0,   0,   0,    0, 
										      0, 1e-3, 1e-9,   0,   0,    0,
										      0,    0,  1e6,   0,   0,    0,
										      0,    0,    0, 1e6,   0,    0,
										      0,    0,    0,   0, 1e6,    0,
										      0,    0,    0,   0,   0, 1e-9 };

const double odom_twist_covariance[36]  = {1e-3,    0,    0,   0,   0,    0, 
										      0, 1e-3,    0,   0,   0,    0,
										      0,    0,  1e6,   0,   0,    0,
										      0,    0,    0, 1e6,   0,    0,
										      0,    0,    0,   0, 1e6,    0,
										      0,    0,    0,   0,   0,  1e3 };
										      
const double odom_twist_covariance2[36] = {1e-9,    0,    0,   0,   0,    0, 
										      0, 1e-3, 1e-9,   0,   0,    0,
										      0,    0,  1e6,   0,   0,    0,
										      0,    0,    0, 1e6,   0,    0,
										      0,    0,    0,   0, 1e6,    0,
										      0,    0,    0,   0,   0, 1e-9} ;



typedef struct __Vel_Pos_Data_
{
	float X;
	float Y;
	float Z;
}Vel_Pos_Data;



typedef struct __MPU6050_DATA_
{
	short accele_x_data; 
	short accele_y_data; 	
	short accele_z_data; 
    short gyros_x_data; 
	short gyros_y_data; 	
	short gyros_z_data; 

}MPU6050_DATA;



typedef struct _SEND_DATA_  
{
	    uint8_t tx[SEND_DATA_SIZE];
		float X_speed;	       
		float Y_speed;           
		float Z_speed;         
		unsigned char Frame_Tail; 
}SEND_DATA;

typedef struct _RECEIVE_AutoCharge_DATA_     
{
	    uint8_t rx[AutoCharge_HEADER];  
		unsigned char Frame_Header;     
		unsigned char Frame_Tail;		
}RECEIVE_AutoCharge_DATA;


typedef struct _DISTANCE_DATA_     
{
	    uint8_t rx[Distance_DATA_size];
		unsigned char Frame_Header;
		unsigned char Frame_Tail;
}DISTANCE_DATA;

typedef struct _Distance_     
{
	float A;  
	float B;  
	float C;  
	float D;
	float E;  
	float F;
 	float G;  
	float H;
}Supersonic_data;



typedef struct _RECEIVE_DATA_     
{
	    uint8_t rx[RECEIVE_DATA_SIZE];
	    uint8_t Flag_Stop;
		unsigned char Frame_Header;
		float X_speed;  
		float Y_speed;  
		float Z_speed;  
		float Power_Voltage;	
		unsigned char Frame_Tail;
}RECEIVE_DATA;



class turn_on_robot : public rclcpp::Node
{
	public:
		turn_on_robot();  
		~turn_on_robot(); 
		void Control();   
		serial::Serial Stm32_Serial; 
	private:
		
		rclcpp::Time _Now, _Last_Time;  
		float Sampling_Time;         

		rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr Cmd_Vel_Sub;

        
        rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odom_publisher;       
        rclcpp::Publisher<std_msgs::msg::Float32>::SharedPtr voltage_publisher;        
        rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr imu_publisher;
        rclcpp::Publisher<rai_robot_msg::msg::Supersonic>::SharedPtr distance_publisher;         

		
		rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr Charging_publisher;
		rclcpp::Publisher<std_msgs::msg::Float32>::SharedPtr Charging_current_publisher;
		rclcpp::Publisher<std_msgs::msg::UInt8>::SharedPtr RED_publisher;
		
		rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr Red_Vel_Sub;
		rclcpp::Subscription<std_msgs::msg::Int8>::SharedPtr Recharge_Flag_Sub;
		
		rclcpp::Service<turtlesim::srv::Spawn>::SharedPtr SetCharge_Service;

		
		
        void Cmd_Vel_Callback(const geometry_msgs::msg::Twist::SharedPtr twist_aux);
        
		
		void Red_Vel_Callback(const geometry_msgs::msg::Twist::SharedPtr twist_aux); 
		void Recharge_Flag_Callback(const std_msgs::msg::Int8::SharedPtr Recharge_Flag); 
		void Set_Charge_Callback(const shared_ptr<turtlesim::srv::Spawn::Request> req,shared_ptr<turtlesim::srv::Spawn::Response> res);
		
		void Publish_Charging();       
		void Publish_ChargingCurrent();
		void Publish_RED();

		void Publish_Odom();      
		void Publish_ImuSensor(); 
		void Publish_Voltage();   
		void Publish_distance();
        
        
        bool Get_Sensor_Data();   
		bool Get_Sensor_Data_New();
        unsigned char Check_Sum(unsigned char Count_Number,unsigned char mode); 
        unsigned char Check_Sum_AutoCharge(unsigned char Count_Number,unsigned char mode); 
        short IMU_Trans(uint8_t Data_High,uint8_t Data_Low);  
		float Odom_Trans(uint8_t Data_High,uint8_t Data_Low); 

        string usart_port_name, robot_frame_id, gyro_frame_id, odom_frame_id; 
        int serial_baud_rate;      
        RECEIVE_DATA Receive_Data; 
        SEND_DATA Send_Data;       
        DISTANCE_DATA Distance_Data; 
        RECEIVE_AutoCharge_DATA Receive_AutoCharge_Data;  
        Supersonic_data distance;  
        Vel_Pos_Data Robot_Pos;    
        Vel_Pos_Data Robot_Vel;    
        MPU6050_DATA Mpu6050_Data; 

		int8_t AutoRecharge=0;
        float Power_voltage;       
        bool Charging=0;           
        float Charging_Current=0;  
        uint8_t Red=0;                
        float odom_x_scale,odom_y_scale,odom_z_scale_positive,odom_z_scale_negative; 
};
#endif
