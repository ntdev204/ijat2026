/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        show.c
 *
 * Description:  show.c module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#include "show.h"
int Voltage_Show;
unsigned char i;
unsigned char Send_Count;
extern SEND_DATA Send_Data;
extern int Time_count;
u8 oled_refresh_flag;
u8 oled_page=0;

int Buzzer_count=25;
void show_task(void *pvParameters)
{
   u32 lastWakeTime = getSysTickCnt();
   while(1)
   {
		int i=0;
		static int LowVoltage_1=0, LowVoltage_2=0;
		vTaskDelayUntil(&lastWakeTime, F2T(RATE_10_HZ));

		if(Time_count<50)Buzzer=1;
		else if(Time_count>=51 && Time_count<100)Buzzer=0;

		if(LowVoltage_1==1 || LowVoltage_2==1)Buzzer_count=0;
		if(Buzzer_count<5)Buzzer_count++;
		if(Buzzer_count<5)Buzzer=1;
		else if(Buzzer_count==5)Buzzer=0;

		for(i=0;i<100;i++)
		{
			Voltage_All+=Get_battery_volt();
		}
		Voltage=Voltage_All/100;
		Voltage = VolMean_Filter(Voltage);
		Voltage_All=0;

		if(Get_Charging_HardWare==1)
		{
			if(Voltage<20&&RED_STATE) Allow_Recharge=1;
		}

		if(LowVoltage_1==1)LowVoltage_1++;
		if(LowVoltage_2==1)LowVoltage_2++;
		APP_Show();

		if(oled_refresh_flag) OLED_Clear(),oled_refresh_flag=0;
		else oled_show();
   }
}

extern u8 start_clear;
extern u8 clear_state;
void oled_show(void)
{
	int Car_Mode_Show;

	if(oled_page==0)
	{

		Car_Mode_Show=(int) (Get_adc_Average(CAR_MODE_ADC,10)/Divisor_Mode);

		Voltage_Show=Voltage*100;

		if(Allow_Recharge) OLED_ShowString(0,0,"RCM :");
		else OLED_ShowString(0,0,"TYPE:");
		if (robot_mode_check_flag==0) OLED_ShowNumber(40,0,Car_Mode_Show,2,12);
		else if (robot_mode_check_flag==1) OLED_ShowString(38,0," X");

		if(Charging)
		{
			OLED_ShowString(80,0," ");
			OLED_ShowString(85,0,"   ");
			OLED_ShowString(60,0,"Cur:");
			oled_showfloat(Charging_Current/1000.0f,90,0,1,2);
		}
		else
		{

			OLED_ShowString(76,0,"    ");
			OLED_ShowString(120,0," ");
			OLED_ShowString(60,0,"GZ");
			if( gyro[2]<0)  OLED_ShowString(80,0,"-"),OLED_ShowNumber(90,0,-gyro[2],5,12);
			else            OLED_ShowString(80,0,"+"),OLED_ShowNumber(90,0, gyro[2],5,12);
		}

		OLED_ShowString(0,10,"A");
		if( MOTOR_A.Target<0)	OLED_ShowString(15,10,"-"),
													OLED_ShowNumber(20,10,-MOTOR_A.Target*1000,5,12);
		else                 	OLED_ShowString(15,10,"+"),
													OLED_ShowNumber(20,10, MOTOR_A.Target*1000,5,12);

		if( MOTOR_A.Encoder<0)OLED_ShowString(60,10,"-"),
													OLED_ShowNumber(75,10,-MOTOR_A.Encoder*1000,5,12);
		else                 	OLED_ShowString(60,10,"+"),
													OLED_ShowNumber(75,10, MOTOR_A.Encoder*1000,5,12);

		OLED_ShowString(0,20,"B");
		if( MOTOR_B.Target<0)	OLED_ShowString(15,20,"-"),
													OLED_ShowNumber(20,20,-MOTOR_B.Target*1000,5,12);
		else                 	OLED_ShowString(15,20,"+"),
													OLED_ShowNumber(20,20, MOTOR_B.Target*1000,5,12);

		if( MOTOR_B.Encoder<0)OLED_ShowString(60,20,"-"),
													OLED_ShowNumber(75,20,-MOTOR_B.Encoder*1000,5,12);
		else                 	OLED_ShowString(60,20,"+"),
													OLED_ShowNumber(75,20, MOTOR_B.Encoder*1000,5,12);

		OLED_ShowString(0,30,"C");
		if( MOTOR_C.Target<0)	OLED_ShowString(15,30,"-"),
													OLED_ShowNumber(20,30,- MOTOR_C.Target*1000,5,12);
		else                 	OLED_ShowString(15,30,"+"),
													OLED_ShowNumber(20,30,  MOTOR_C.Target*1000,5,12);

		if( MOTOR_C.Encoder<0)OLED_ShowString(60,30,"-"),
													OLED_ShowNumber(75,30,-MOTOR_C.Encoder*1000,5,12);
		else                 	OLED_ShowString(60,30,"+"),
													OLED_ShowNumber(75,30, MOTOR_C.Encoder*1000,5,12);

		#if Mec

		OLED_ShowString(0,40,"D");
		if( MOTOR_D.Target<0)	OLED_ShowString(15,40,"-"),
													OLED_ShowNumber(20,40,- MOTOR_D.Target*1000,5,12);
		else                 	OLED_ShowString(15,40,"+"),
													OLED_ShowNumber(20,40,  MOTOR_D.Target*1000,5,12);

		if( MOTOR_D.Encoder<0)OLED_ShowString(60,40,"-"),
													OLED_ShowNumber(75,40,-MOTOR_D.Encoder*1000,5,12);
		else                 	OLED_ShowString(60,40,"+"),
													OLED_ShowNumber(75,40, MOTOR_D.Encoder*1000,5,12);

		#elif Omni

		OLED_ShowString(0,40,"MOVE_Z");
		if( Send_Data.Sensor_Str.Z_speed<0)	OLED_ShowString(60,40,"-"),
													OLED_ShowNumber(75,40,-Send_Data.Sensor_Str.Z_speed,5,12);
		else                 	OLED_ShowString(60,40,"+"),
													OLED_ShowNumber(75,40, Send_Data.Sensor_Str.Z_speed,5,12);
		#endif

		if      (PS2_ON_Flag==1)   OLED_ShowString(0,50,"PS2  ");
		else if (APP_ON_Flag==1)   OLED_ShowString(0,50,"APP  ");
		else if (Remote_ON_Flag==1)OLED_ShowString(0,50,"R-C  ");
		else if (CAN_ON_Flag==1)   OLED_ShowString(0,50,"CAN  ");
		else if (Usart_ON_Flag==1) OLED_ShowString(0,50,"USART");
		else if (nav_walk==1)      OLED_ShowString(0,50,"nav");
		else                       OLED_ShowString(0,50,"ROS  ");

		if(EN==1&&Flag_Stop==0)   OLED_ShowString(45,50,"O N");
		else                      OLED_ShowString(45,50,"OFF");

															OLED_ShowString(88,50,".");
															OLED_ShowString(110,50,"V");
															OLED_ShowNumber(75,50,Voltage_Show/100,2,12);
															OLED_ShowNumber(98,50,Voltage_Show%100,2,12);
		if(Voltage_Show%100<10) 	OLED_ShowNumber(92,50,0,2,12);

		OLED_Refresh_Gram();
	}
	else if(oled_page==1)
	{

		OLED_ShowString(07,00,"LA  LB  RB  RA");
		OLED_ShowNumber(0+9,10,L_A,1,12);
		OLED_ShowNumber(30+9,10,L_B,1,12);
		OLED_ShowNumber(60+9,10,R_B,1,12);
		OLED_ShowNumber(90+9,10,R_A,1,12);
		OLED_ShowString(0,30,"cur:");
		OLED_ShowString(75,30,"A");
		oled_showfloat(Charging_Current/1000.0f,30,30,2,2);

		OLED_Refresh_Gram();
	}
	else if( oled_page==2 )
	{

		OLED_ShowString(80,0,"RC:");
		OLED_ShowNumber(104,0,Allow_Recharge,1,12);

		OLED_ShowString(80,10,"sp:");
		if( smooth_control.VX!=0 || smooth_control.VY!=0 ||smooth_control.VZ!=0  )
			OLED_ShowNumber(104,10,1,1,12);
		else
			OLED_ShowNumber(104,10,0,1,12);

		OLED_ShowString(80,20,"CG:");
		OLED_ShowNumber(104,20,Charging,1,12);

		OLED_ShowString(0,0,"A:");
		OLED_ShowNumber(12,0,int_abs(MOTOR_A.Motor_Pwm),5,12);
		OLED_ShowString(0,10,"B:");
		OLED_ShowNumber(12,10,int_abs(MOTOR_B.Motor_Pwm),5,12);
		OLED_ShowString(0,20,"C:");
		OLED_ShowNumber(12,20,int_abs(MOTOR_C.Motor_Pwm),5,12);
		OLED_ShowString(0,30,"D:");
		OLED_ShowNumber(12,30,int_abs(MOTOR_D.Motor_Pwm),5,12);

		OLED_ShowString(0,50,"clear:");
		OLED_ShowNumber(50,50,start_clear,1,12);

		OLED_Refresh_Gram();
	}
}

void APP_Show(void)
{
	 static u8 flag_show;
	 int Left_Figure,Right_Figure,Voltage_Show;

	 Voltage_Show=(Voltage*100-2000)*5/26;
	 if(Voltage_Show>100)Voltage_Show=100;

	 Left_Figure=MOTOR_A.Encoder*100;  if(Left_Figure<0)Left_Figure=-Left_Figure;
	 Right_Figure=MOTOR_B.Encoder*100; if(Right_Figure<0)Right_Figure=-Right_Figure;

	 flag_show=!flag_show;

 if(PID_Send==1)
	 {

		 printf("{C%d:%d:%d}$",(int)RC_Velocity,(int)Velocity_KP,(int)Velocity_KI);
		 PID_Send=0;
	 }
	 else	if(flag_show==0)
	 {

	   printf("{A%d:%d:%d:%d}$",(u8)Left_Figure,(u8)Right_Figure,Voltage_Show,(int)gyro[2]);
	 }
	 else
	 {

	   printf("{B%d:%d:%d}$",(int)gyro[0],(int)gyro[1],(int)gyro[2]);
	 }

}

float base_vol=22.5f;
#define VOL_COUNT 100
float VolMean_Filter(float data)
{
    u8 i;
    double Sum_Speed = 0;
    float Filter_Speed;
    static  float Speed_Buf[VOL_COUNT]= {0};

	static u8 once=1;
	if(once)
	{
		once=0;
		for(i=0;i<VOL_COUNT;i++)
			Speed_Buf[i]=base_vol;
	}

    for(i = 1 ; i<VOL_COUNT; i++)
    {
        Speed_Buf[i - 1] = Speed_Buf[i];
    }
    Speed_Buf[VOL_COUNT - 1] =data;

    for(i = 0 ; i < VOL_COUNT; i++)
    {
        Sum_Speed += Speed_Buf[i];
    }
    Filter_Speed = (float)(Sum_Speed / VOL_COUNT);
    return Filter_Speed;
}
