/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        stm32f4xx_i2c.c
 *
 * Description:  stm32f4xx_i2c.c module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#include "stm32f4xx_i2c.h"
#include "stm32f4xx_rcc.h"


#define CR1_CLEAR_MASK    ((uint16_t)0xFBF5)
#define FLAG_MASK         ((uint32_t)0x00FFFFFF)
#define ITEN_MASK         ((uint32_t)0x07000000)


void I2C_DeInit(I2C_TypeDef* I2Cx)
{

  assert_param(IS_I2C_ALL_PERIPH(I2Cx));

  if (I2Cx == I2C1)
  {

    RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C1, ENABLE);

    RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C1, DISABLE);
  }
  else if (I2Cx == I2C2)
  {

    RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C2, ENABLE);

    RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C2, DISABLE);
  }
  else
  {
    if (I2Cx == I2C3)
    {

      RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C3, ENABLE);

      RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C3, DISABLE);
    }
  }
}


void I2C_Init(I2C_TypeDef* I2Cx, I2C_InitTypeDef* I2C_InitStruct)
{
  uint16_t tmpreg = 0, freqrange = 0;
  uint16_t result = 0x04;
  uint32_t pclk1 = 8000000;
  RCC_ClocksTypeDef  rcc_clocks;

  assert_param(IS_I2C_ALL_PERIPH(I2Cx));
  assert_param(IS_I2C_CLOCK_SPEED(I2C_InitStruct->I2C_ClockSpeed));
  assert_param(IS_I2C_MODE(I2C_InitStruct->I2C_Mode));
  assert_param(IS_I2C_DUTY_CYCLE(I2C_InitStruct->I2C_DutyCycle));
  assert_param(IS_I2C_OWN_ADDRESS1(I2C_InitStruct->I2C_OwnAddress1));
  assert_param(IS_I2C_ACK_STATE(I2C_InitStruct->I2C_Ack));
  assert_param(IS_I2C_ACKNOWLEDGE_ADDRESS(I2C_InitStruct->I2C_AcknowledgedAddress));


  tmpreg = I2Cx->CR2;

  tmpreg &= (uint16_t)~((uint16_t)I2C_CR2_FREQ);

  RCC_GetClocksFreq(&rcc_clocks);
  pclk1 = rcc_clocks.PCLK1_Frequency;

  freqrange = (uint16_t)(pclk1 / 1000000);
  tmpreg |= freqrange;

  I2Cx->CR2 = tmpreg;


  I2Cx->CR1 &= (uint16_t)~((uint16_t)I2C_CR1_PE);


  tmpreg = 0;


  if (I2C_InitStruct->I2C_ClockSpeed <= 100000)
  {

    result = (uint16_t)(pclk1 / (I2C_InitStruct->I2C_ClockSpeed << 1));

    if (result < 0x04)
    {

      result = 0x04;
    }

    tmpreg |= result;

    I2Cx->TRISE = freqrange + 1;
  }


  else
  {
    if (I2C_InitStruct->I2C_DutyCycle == I2C_DutyCycle_2)
    {

      result = (uint16_t)(pclk1 / (I2C_InitStruct->I2C_ClockSpeed * 3));
    }
    else
    {

      result = (uint16_t)(pclk1 / (I2C_InitStruct->I2C_ClockSpeed * 25));

      result |= I2C_DutyCycle_16_9;
    }


    if ((result & I2C_CCR_CCR) == 0)
    {

      result |= (uint16_t)0x0001;
    }

    tmpreg |= (uint16_t)(result | I2C_CCR_FS);

    I2Cx->TRISE = (uint16_t)(((freqrange * (uint16_t)300) / (uint16_t)1000) + (uint16_t)1);
  }


  I2Cx->CCR = tmpreg;

  I2Cx->CR1 |= I2C_CR1_PE;


  tmpreg = I2Cx->CR1;

  tmpreg &= CR1_CLEAR_MASK;


  tmpreg |= (uint16_t)((uint32_t)I2C_InitStruct->I2C_Mode | I2C_InitStruct->I2C_Ack);

  I2Cx->CR1 = tmpreg;


  I2Cx->OAR1 = (I2C_InitStruct->I2C_AcknowledgedAddress | I2C_InitStruct->I2C_OwnAddress1);
}


void I2C_StructInit(I2C_InitTypeDef* I2C_InitStruct)
{


  I2C_InitStruct->I2C_ClockSpeed = 5000;

  I2C_InitStruct->I2C_Mode = I2C_Mode_I2C;

  I2C_InitStruct->I2C_DutyCycle = I2C_DutyCycle_2;

  I2C_InitStruct->I2C_OwnAddress1 = 0;

  I2C_InitStruct->I2C_Ack = I2C_Ack_Disable;

  I2C_InitStruct->I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
}


void I2C_Cmd(I2C_TypeDef* I2Cx, FunctionalState NewState)
{

  assert_param(IS_I2C_ALL_PERIPH(I2Cx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {

    I2Cx->CR1 |= I2C_CR1_PE;
  }
  else
  {

    I2Cx->CR1 &= (uint16_t)~((uint16_t)I2C_CR1_PE);
  }
}


void I2C_AnalogFilterCmd(I2C_TypeDef* I2Cx, FunctionalState NewState)
{

  assert_param(IS_I2C_ALL_PERIPH(I2Cx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {

    I2Cx->FLTR &= (uint16_t)~((uint16_t)I2C_FLTR_ANOFF);
  }
  else
  {

    I2Cx->FLTR |= I2C_FLTR_ANOFF;
  }
}


void I2C_DigitalFilterConfig(I2C_TypeDef* I2Cx, uint16_t I2C_DigitalFilter)
{
  uint16_t tmpreg = 0;


  assert_param(IS_I2C_ALL_PERIPH(I2Cx));
  assert_param(IS_I2C_DIGITAL_FILTER(I2C_DigitalFilter));


  tmpreg = I2Cx->FLTR;


  tmpreg &= (uint16_t)~((uint16_t)I2C_FLTR_DNF);


  tmpreg |= (uint16_t)((uint16_t)I2C_DigitalFilter & I2C_FLTR_DNF);


  I2Cx->FLTR = tmpreg;
}


void I2C_GenerateSTART(I2C_TypeDef* I2Cx, FunctionalState NewState)
{

  assert_param(IS_I2C_ALL_PERIPH(I2Cx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {

    I2Cx->CR1 |= I2C_CR1_START;
  }
  else
  {

    I2Cx->CR1 &= (uint16_t)~((uint16_t)I2C_CR1_START);
  }
}


void I2C_GenerateSTOP(I2C_TypeDef* I2Cx, FunctionalState NewState)
{

  assert_param(IS_I2C_ALL_PERIPH(I2Cx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {

    I2Cx->CR1 |= I2C_CR1_STOP;
  }
  else
  {

    I2Cx->CR1 &= (uint16_t)~((uint16_t)I2C_CR1_STOP);
  }
}


void I2C_Send7bitAddress(I2C_TypeDef* I2Cx, uint8_t Address, uint8_t I2C_Direction)
{

  assert_param(IS_I2C_ALL_PERIPH(I2Cx));
  assert_param(IS_I2C_DIRECTION(I2C_Direction));

  if (I2C_Direction != I2C_Direction_Transmitter)
  {

    Address |= I2C_OAR1_ADD0;
  }
  else
  {

    Address &= (uint8_t)~((uint8_t)I2C_OAR1_ADD0);
  }

  I2Cx->DR = Address;
}


void I2C_AcknowledgeConfig(I2C_TypeDef* I2Cx, FunctionalState NewState)
{

  assert_param(IS_I2C_ALL_PERIPH(I2Cx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {

    I2Cx->CR1 |= I2C_CR1_ACK;
  }
  else
  {

    I2Cx->CR1 &= (uint16_t)~((uint16_t)I2C_CR1_ACK);
  }
}


void I2C_OwnAddress2Config(I2C_TypeDef* I2Cx, uint8_t Address)
{
  uint16_t tmpreg = 0;


  assert_param(IS_I2C_ALL_PERIPH(I2Cx));


  tmpreg = I2Cx->OAR2;


  tmpreg &= (uint16_t)~((uint16_t)I2C_OAR2_ADD2);


  tmpreg |= (uint16_t)((uint16_t)Address & (uint16_t)0x00FE);


  I2Cx->OAR2 = tmpreg;
}


void I2C_DualAddressCmd(I2C_TypeDef* I2Cx, FunctionalState NewState)
{

  assert_param(IS_I2C_ALL_PERIPH(I2Cx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {

    I2Cx->OAR2 |= I2C_OAR2_ENDUAL;
  }
  else
  {

    I2Cx->OAR2 &= (uint16_t)~((uint16_t)I2C_OAR2_ENDUAL);
  }
}


void I2C_GeneralCallCmd(I2C_TypeDef* I2Cx, FunctionalState NewState)
{

  assert_param(IS_I2C_ALL_PERIPH(I2Cx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {

    I2Cx->CR1 |= I2C_CR1_ENGC;
  }
  else
  {

    I2Cx->CR1 &= (uint16_t)~((uint16_t)I2C_CR1_ENGC);
  }
}


void I2C_SoftwareResetCmd(I2C_TypeDef* I2Cx, FunctionalState NewState)
{

  assert_param(IS_I2C_ALL_PERIPH(I2Cx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {

    I2Cx->CR1 |= I2C_CR1_SWRST;
  }
  else
  {

    I2Cx->CR1 &= (uint16_t)~((uint16_t)I2C_CR1_SWRST);
  }
}


void I2C_StretchClockCmd(I2C_TypeDef* I2Cx, FunctionalState NewState)
{

  assert_param(IS_I2C_ALL_PERIPH(I2Cx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState == DISABLE)
  {

    I2Cx->CR1 |= I2C_CR1_NOSTRETCH;
  }
  else
  {

    I2Cx->CR1 &= (uint16_t)~((uint16_t)I2C_CR1_NOSTRETCH);
  }
}


void I2C_FastModeDutyCycleConfig(I2C_TypeDef* I2Cx, uint16_t I2C_DutyCycle)
{

  assert_param(IS_I2C_ALL_PERIPH(I2Cx));
  assert_param(IS_I2C_DUTY_CYCLE(I2C_DutyCycle));
  if (I2C_DutyCycle != I2C_DutyCycle_16_9)
  {

    I2Cx->CCR &= I2C_DutyCycle_2;
  }
  else
  {

    I2Cx->CCR |= I2C_DutyCycle_16_9;
  }
}


void I2C_NACKPositionConfig(I2C_TypeDef* I2Cx, uint16_t I2C_NACKPosition)
{

  assert_param(IS_I2C_ALL_PERIPH(I2Cx));
  assert_param(IS_I2C_NACK_POSITION(I2C_NACKPosition));


  if (I2C_NACKPosition == I2C_NACKPosition_Next)
  {

    I2Cx->CR1 |= I2C_NACKPosition_Next;
  }
  else
  {

    I2Cx->CR1 &= I2C_NACKPosition_Current;
  }
}


void I2C_SMBusAlertConfig(I2C_TypeDef* I2Cx, uint16_t I2C_SMBusAlert)
{

  assert_param(IS_I2C_ALL_PERIPH(I2Cx));
  assert_param(IS_I2C_SMBUS_ALERT(I2C_SMBusAlert));
  if (I2C_SMBusAlert == I2C_SMBusAlert_Low)
  {

    I2Cx->CR1 |= I2C_SMBusAlert_Low;
  }
  else
  {

    I2Cx->CR1 &= I2C_SMBusAlert_High;
  }
}


void I2C_ARPCmd(I2C_TypeDef* I2Cx, FunctionalState NewState)
{

  assert_param(IS_I2C_ALL_PERIPH(I2Cx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {

    I2Cx->CR1 |= I2C_CR1_ENARP;
  }
  else
  {

    I2Cx->CR1 &= (uint16_t)~((uint16_t)I2C_CR1_ENARP);
  }
}


void I2C_SendData(I2C_TypeDef* I2Cx, uint8_t Data)
{

  assert_param(IS_I2C_ALL_PERIPH(I2Cx));

  I2Cx->DR = Data;
}


uint8_t I2C_ReceiveData(I2C_TypeDef* I2Cx)
{

  assert_param(IS_I2C_ALL_PERIPH(I2Cx));

  return (uint8_t)I2Cx->DR;
}


void I2C_TransmitPEC(I2C_TypeDef* I2Cx, FunctionalState NewState)
{

  assert_param(IS_I2C_ALL_PERIPH(I2Cx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {

    I2Cx->CR1 |= I2C_CR1_PEC;
  }
  else
  {

    I2Cx->CR1 &= (uint16_t)~((uint16_t)I2C_CR1_PEC);
  }
}


void I2C_PECPositionConfig(I2C_TypeDef* I2Cx, uint16_t I2C_PECPosition)
{

  assert_param(IS_I2C_ALL_PERIPH(I2Cx));
  assert_param(IS_I2C_PEC_POSITION(I2C_PECPosition));
  if (I2C_PECPosition == I2C_PECPosition_Next)
  {

    I2Cx->CR1 |= I2C_PECPosition_Next;
  }
  else
  {

    I2Cx->CR1 &= I2C_PECPosition_Current;
  }
}


void I2C_CalculatePEC(I2C_TypeDef* I2Cx, FunctionalState NewState)
{

  assert_param(IS_I2C_ALL_PERIPH(I2Cx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {

    I2Cx->CR1 |= I2C_CR1_ENPEC;
  }
  else
  {

    I2Cx->CR1 &= (uint16_t)~((uint16_t)I2C_CR1_ENPEC);
  }
}


uint8_t I2C_GetPEC(I2C_TypeDef* I2Cx)
{

  assert_param(IS_I2C_ALL_PERIPH(I2Cx));

  return ((I2Cx->SR2) >> 8);
}


void I2C_DMACmd(I2C_TypeDef* I2Cx, FunctionalState NewState)
{

  assert_param(IS_I2C_ALL_PERIPH(I2Cx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {

    I2Cx->CR2 |= I2C_CR2_DMAEN;
  }
  else
  {

    I2Cx->CR2 &= (uint16_t)~((uint16_t)I2C_CR2_DMAEN);
  }
}


void I2C_DMALastTransferCmd(I2C_TypeDef* I2Cx, FunctionalState NewState)
{

  assert_param(IS_I2C_ALL_PERIPH(I2Cx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {

    I2Cx->CR2 |= I2C_CR2_LAST;
  }
  else
  {

    I2Cx->CR2 &= (uint16_t)~((uint16_t)I2C_CR2_LAST);
  }
}


uint16_t I2C_ReadRegister(I2C_TypeDef* I2Cx, uint8_t I2C_Register)
{
  __IO uint32_t tmp = 0;


  assert_param(IS_I2C_ALL_PERIPH(I2Cx));
  assert_param(IS_I2C_REGISTER(I2C_Register));

  tmp = (uint32_t) I2Cx;
  tmp += I2C_Register;


  return (*(__IO uint16_t *) tmp);
}


void I2C_ITConfig(I2C_TypeDef* I2Cx, uint16_t I2C_IT, FunctionalState NewState)
{

  assert_param(IS_I2C_ALL_PERIPH(I2Cx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  assert_param(IS_I2C_CONFIG_IT(I2C_IT));

  if (NewState != DISABLE)
  {

    I2Cx->CR2 |= I2C_IT;
  }
  else
  {

    I2Cx->CR2 &= (uint16_t)~I2C_IT;
  }
}


ErrorStatus I2C_CheckEvent(I2C_TypeDef* I2Cx, uint32_t I2C_EVENT)
{
  uint32_t lastevent = 0;
  uint32_t flag1 = 0, flag2 = 0;
  ErrorStatus status = ERROR;


  assert_param(IS_I2C_ALL_PERIPH(I2Cx));
  assert_param(IS_I2C_EVENT(I2C_EVENT));


  flag1 = I2Cx->SR1;
  flag2 = I2Cx->SR2;
  flag2 = flag2 << 16;


  lastevent = (flag1 | flag2) & FLAG_MASK;


  if ((lastevent & I2C_EVENT) == I2C_EVENT)
  {

    status = SUCCESS;
  }
  else
  {

    status = ERROR;
  }

  return status;
}


uint32_t I2C_GetLastEvent(I2C_TypeDef* I2Cx)
{
  uint32_t lastevent = 0;
  uint32_t flag1 = 0, flag2 = 0;


  assert_param(IS_I2C_ALL_PERIPH(I2Cx));


  flag1 = I2Cx->SR1;
  flag2 = I2Cx->SR2;
  flag2 = flag2 << 16;


  lastevent = (flag1 | flag2) & FLAG_MASK;


  return lastevent;
}


FlagStatus I2C_GetFlagStatus(I2C_TypeDef* I2Cx, uint32_t I2C_FLAG)
{
  FlagStatus bitstatus = RESET;
  __IO uint32_t i2creg = 0, i2cxbase = 0;


  assert_param(IS_I2C_ALL_PERIPH(I2Cx));
  assert_param(IS_I2C_GET_FLAG(I2C_FLAG));


  i2cxbase = (uint32_t)I2Cx;


  i2creg = I2C_FLAG >> 28;


  I2C_FLAG &= FLAG_MASK;

  if(i2creg != 0)
  {

    i2cxbase += 0x14;
  }
  else
  {

    I2C_FLAG = (uint32_t)(I2C_FLAG >> 16);

    i2cxbase += 0x18;
  }

  if(((*(__IO uint32_t *)i2cxbase) & I2C_FLAG) != (uint32_t)RESET)
  {

    bitstatus = SET;
  }
  else
  {

    bitstatus = RESET;
  }


  return  bitstatus;
}


void I2C_ClearFlag(I2C_TypeDef* I2Cx, uint32_t I2C_FLAG)
{
  uint32_t flagpos = 0;

  assert_param(IS_I2C_ALL_PERIPH(I2Cx));
  assert_param(IS_I2C_CLEAR_FLAG(I2C_FLAG));

  flagpos = I2C_FLAG & FLAG_MASK;

  I2Cx->SR1 = (uint16_t)~flagpos;
}


ITStatus I2C_GetITStatus(I2C_TypeDef* I2Cx, uint32_t I2C_IT)
{
  ITStatus bitstatus = RESET;
  uint32_t enablestatus = 0;


  assert_param(IS_I2C_ALL_PERIPH(I2Cx));
  assert_param(IS_I2C_GET_IT(I2C_IT));


  enablestatus = (uint32_t)(((I2C_IT & ITEN_MASK) >> 16) & (I2Cx->CR2)) ;


  I2C_IT &= FLAG_MASK;


  if (((I2Cx->SR1 & I2C_IT) != (uint32_t)RESET) && enablestatus)
  {

    bitstatus = SET;
  }
  else
  {

    bitstatus = RESET;
  }

  return  bitstatus;
}


void I2C_ClearITPendingBit(I2C_TypeDef* I2Cx, uint32_t I2C_IT)
{
  uint32_t flagpos = 0;

  assert_param(IS_I2C_ALL_PERIPH(I2Cx));
  assert_param(IS_I2C_CLEAR_IT(I2C_IT));


  flagpos = I2C_IT & FLAG_MASK;


  I2Cx->SR1 = (uint16_t)~flagpos;
}
