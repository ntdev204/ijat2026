/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        MPU6050.c
 *
 * Description:  MPU6050.c module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#include "MPU6050.h"
#include "I2C.h"

#define PRINT_ACCEL     (0x01)
#define PRINT_GYRO      (0x02)
#define PRINT_QUAT      (0x04)
#define ACCEL_ON        (0x01)
#define GYRO_ON         (0x02)
#define MOTION          (0)
#define NO_MOTION       (1)
#define DEFAULT_MPU_HZ  (200)
#define FLASH_SIZE      (512)
#define FLASH_MEM_START ((void*)0x1800)
#define q30  1073741824.0f

short gyro[3], accel[3];
short Deviation_gyro[3],Original_gyro[3];
short Deviation_accel[3],Original_accel[3];
int Deviation_Count;
float q0=1.0f,q1=0.0f,q2=0.0f,q3=0.0f;

uint8_t buffer[14];

int16_t  MPU6050_FIFO[6][11];
int16_t Gx_offset=0,Gy_offset=0,Gz_offset=0;

void MPU6050_task(void *pvParameters)
{

    u32 lastWakeTime = getSysTickCnt();
    while(1)
    {

			vTaskDelayUntil(&lastWakeTime, F2T(RATE_100_HZ));

		  if(Deviation_Count<CONTROL_DELAY)
		  {
		  	Deviation_Count++;
			   memcpy(Deviation_gyro,gyro,sizeof(gyro));
				 memcpy(Deviation_accel,accel,sizeof(accel));
		  }

			MPU_Get_Accelscope();

      MPU_Get_Gyroscope();

    }
}

void MPU_Get_Accelscope()
{
	int i=0;
		accel[0]=(I2C_ReadOneByte(devAddr,MPU6050_RA_ACCEL_XOUT_H)<<8)+I2C_ReadOneByte(devAddr,MPU6050_RA_ACCEL_XOUT_L);
		accel[1]=(I2C_ReadOneByte(devAddr,MPU6050_RA_ACCEL_YOUT_H)<<8)+I2C_ReadOneByte(devAddr,MPU6050_RA_ACCEL_YOUT_L);
		accel[2]=(I2C_ReadOneByte(devAddr,MPU6050_RA_ACCEL_ZOUT_H)<<8)+I2C_ReadOneByte(devAddr,MPU6050_RA_ACCEL_ZOUT_L);

	if(Deviation_Count<CONTROL_DELAY)
		{
     i=i+1;
		}
	else
		{

			Original_accel[0] =accel[0];
			Original_accel[1] =accel[1];
			Original_accel[2]= accel[2];

			accel[0] =Original_accel[0]-Deviation_accel[0];
			accel[1] =Original_accel[1]-Deviation_accel[1];
			accel[2]= Original_accel[2]-Deviation_accel[2]+16384;
		}

}

u8 MPU_Get_Gyroscope()
{
	  u8 res;
  	gyro[0]=(I2C_ReadOneByte(devAddr,MPU6050_RA_GYRO_XOUT_H)<<8)+I2C_ReadOneByte(devAddr,MPU6050_RA_GYRO_XOUT_L);
		gyro[1]=(I2C_ReadOneByte(devAddr,MPU6050_RA_GYRO_YOUT_H)<<8)+I2C_ReadOneByte(devAddr,MPU6050_RA_GYRO_YOUT_L);
		gyro[2]=(I2C_ReadOneByte(devAddr,MPU6050_RA_GYRO_ZOUT_H)<<8)+I2C_ReadOneByte(devAddr,MPU6050_RA_GYRO_ZOUT_L);

	if(Deviation_Count<CONTROL_DELAY)
		{
			Led_Count=1;
			Flag_Stop=1;

		}
	else
		{
			if(Deviation_Count==CONTROL_DELAY)
			{
				Flag_Stop=0;
			  Led_Count=300;
        Deviation_Count++;
			}

			Original_gyro[0] =gyro[0];
			Original_gyro[1] =gyro[1];
			Original_gyro[2]= gyro[2];

			gyro[0] =Original_gyro[0]-Deviation_gyro[0];
			gyro[1] =Original_gyro[1]-Deviation_gyro[1];
			gyro[2]= Original_gyro[2]-Deviation_gyro[2];
		}

  return res;

}

void  MPU6050_newValues(int16_t ax,int16_t ay,int16_t az,int16_t gx,int16_t gy,int16_t gz)
{
unsigned char i ;
int32_t sum=0;
for(i=1;i<10;i++){
MPU6050_FIFO[0][i-1]=MPU6050_FIFO[0][i];
MPU6050_FIFO[1][i-1]=MPU6050_FIFO[1][i];
MPU6050_FIFO[2][i-1]=MPU6050_FIFO[2][i];
MPU6050_FIFO[3][i-1]=MPU6050_FIFO[3][i];
MPU6050_FIFO[4][i-1]=MPU6050_FIFO[4][i];
MPU6050_FIFO[5][i-1]=MPU6050_FIFO[5][i];
}
MPU6050_FIFO[0][9]=ax;
MPU6050_FIFO[1][9]=ay;
MPU6050_FIFO[2][9]=az;
MPU6050_FIFO[3][9]=gx;
MPU6050_FIFO[4][9]=gy;
MPU6050_FIFO[5][9]=gz;

sum=0;
for(i=0;i<10;i++){
   sum+=MPU6050_FIFO[0][i];
}
MPU6050_FIFO[0][10]=sum/10;

sum=0;
for(i=0;i<10;i++){
   sum+=MPU6050_FIFO[1][i];
}
MPU6050_FIFO[1][10]=sum/10;

sum=0;
for(i=0;i<10;i++){
   sum+=MPU6050_FIFO[2][i];
}
MPU6050_FIFO[2][10]=sum/10;

sum=0;
for(i=0;i<10;i++){
   sum+=MPU6050_FIFO[3][i];
}
MPU6050_FIFO[3][10]=sum/10;

sum=0;
for(i=0;i<10;i++){
   sum+=MPU6050_FIFO[4][i];
}
MPU6050_FIFO[4][10]=sum/10;

sum=0;
for(i=0;i<10;i++){
   sum+=MPU6050_FIFO[5][i];
}
MPU6050_FIFO[5][10]=sum/10;
}

void MPU6050_setClockSource(uint8_t source){
    I2C_WriteBits(devAddr, MPU6050_RA_PWR_MGMT_1, MPU6050_PWR1_CLKSEL_BIT, MPU6050_PWR1_CLKSEL_LENGTH, source);

}

void MPU6050_setFullScaleGyroRange(uint8_t range) {
    I2C_WriteBits(devAddr, MPU6050_RA_GYRO_CONFIG, MPU6050_GCONFIG_FS_SEL_BIT, MPU6050_GCONFIG_FS_SEL_LENGTH, range);
}

void MPU6050_setFullScaleAccelRange(uint8_t range) {
    I2C_WriteBits(devAddr, MPU6050_RA_ACCEL_CONFIG, MPU6050_ACONFIG_AFS_SEL_BIT, MPU6050_ACONFIG_AFS_SEL_LENGTH, range);
}

void MPU6050_setSleepEnabled(uint8_t enabled) {
    I2C_WriteOneBit(devAddr, MPU6050_RA_PWR_MGMT_1, MPU6050_PWR1_SLEEP_BIT, enabled);
}

uint8_t MPU6050_getDeviceID(void) {

	return I2C_ReadOneByte(devAddr,MPU6050_RA_WHO_AM_I);

}

uint8_t MPU6050_testConnection(void) {
   if(MPU6050_getDeviceID() == 0x68)
   return 1;
   	else return 0;
}

void MPU6050_setI2CMasterModeEnabled(uint8_t enabled) {
    I2C_WriteOneBit(devAddr, MPU6050_RA_USER_CTRL, MPU6050_USERCTRL_I2C_MST_EN_BIT, enabled);
}

void MPU6050_setI2CBypassEnabled(uint8_t enabled) {
    I2C_WriteOneBit(devAddr, MPU6050_RA_INT_PIN_CFG, MPU6050_INTCFG_I2C_BYPASS_EN_BIT, enabled);
}

u8 MPU6050_initialize(void) {

		u8 res;

	I2C_WriteOneByte(devAddr,MPU6050_RA_PWR_MGMT_1,0X80);
  delay_ms(200);
	I2C_WriteOneByte(devAddr,MPU6050_RA_PWR_MGMT_1,0X00);

	MPU6050_setFullScaleGyroRange(MPU6050_GYRO_FS_500);

  MPU6050_setFullScaleAccelRange(MPU6050_ACCEL_FS_2);
	MPU6050_Set_Rate(50);

	I2C_WriteOneByte(devAddr,MPU6050_RA_INT_ENABLE,0X00);
	I2C_WriteOneByte(devAddr,MPU6050_RA_USER_CTRL,0X00);
	I2C_WriteOneByte(devAddr,MPU6050_RA_FIFO_EN,0X00);

	I2C_WriteOneByte(devAddr,MPU6050_RA_INT_PIN_CFG,0X80);

	res=I2C_ReadOneByte(devAddr,MPU6050_RA_WHO_AM_I);
	if(res==MPU6050_DEFAULT_ADDRESS)
	{
		I2C_WriteOneByte(devAddr,MPU6050_RA_PWR_MGMT_1,0X01);
		I2C_WriteOneByte(devAddr,MPU6050_RA_PWR_MGMT_2,0X00);
		MPU6050_Set_Rate(50);
 	}else return 1;
	return 0;

}

int Read_Temperature(void)
{
	  float Temp;
	  Temp=(I2C_ReadOneByte(devAddr,MPU6050_RA_TEMP_OUT_H)<<8)+I2C_ReadOneByte(devAddr,MPU6050_RA_TEMP_OUT_L);
		if(Temp>32768) Temp-=65536;
		Temp=(36.53f+Temp/340)*10;
	  return (int)Temp;
}

unsigned char MPU6050_Set_LPF(u16 lpf)
{
	u8 data=0;
	if(lpf>=188)data=1;
	else if(lpf>=98)data=2;
	else if(lpf>=42)data=3;
	else if(lpf>=20)data=4;
	else if(lpf>=10)data=5;
	else data=6;
	return I2C_WriteOneByte(devAddr,MPU6050_RA_CONFIG,data);
}

unsigned char MPU6050_Set_Rate(u16 rate)
{
	u8 data;
	if(rate>1000)rate=1000;
	if(rate<4)rate=4;
	data=1000/rate-1;
	data=I2C_WriteOneByte(devAddr,MPU6050_RA_SMPLRT_DIV,data);
 	return MPU6050_Set_LPF(rate/2);
}
