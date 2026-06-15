/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        stm32f4xx_dac.c
 *
 * Description:  stm32f4xx_dac.c module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#include "stm32f4xx_dac.h"
#include "stm32f4xx_rcc.h"


#define CR_CLEAR_MASK              ((uint32_t)0x00000FFE)


#define DUAL_SWTRIG_SET            ((uint32_t)0x00000003)
#define DUAL_SWTRIG_RESET          ((uint32_t)0xFFFFFFFC)


#define DHR12R1_OFFSET             ((uint32_t)0x00000008)
#define DHR12R2_OFFSET             ((uint32_t)0x00000014)
#define DHR12RD_OFFSET             ((uint32_t)0x00000020)


#define DOR_OFFSET                 ((uint32_t)0x0000002C)


void DAC_DeInit(void)
{

  RCC_APB1PeriphResetCmd(RCC_APB1Periph_DAC, ENABLE);

  RCC_APB1PeriphResetCmd(RCC_APB1Periph_DAC, DISABLE);
}


void DAC_Init(uint32_t DAC_Channel, DAC_InitTypeDef* DAC_InitStruct)
{
  uint32_t tmpreg1 = 0, tmpreg2 = 0;


  assert_param(IS_DAC_TRIGGER(DAC_InitStruct->DAC_Trigger));
  assert_param(IS_DAC_GENERATE_WAVE(DAC_InitStruct->DAC_WaveGeneration));
  assert_param(IS_DAC_LFSR_UNMASK_TRIANGLE_AMPLITUDE(DAC_InitStruct->DAC_LFSRUnmask_TriangleAmplitude));
  assert_param(IS_DAC_OUTPUT_BUFFER_STATE(DAC_InitStruct->DAC_OutputBuffer));


  tmpreg1 = DAC->CR;

  tmpreg1 &= ~(CR_CLEAR_MASK << DAC_Channel);


  tmpreg2 = (DAC_InitStruct->DAC_Trigger | DAC_InitStruct->DAC_WaveGeneration |
             DAC_InitStruct->DAC_LFSRUnmask_TriangleAmplitude | \
             DAC_InitStruct->DAC_OutputBuffer);

  tmpreg1 |= tmpreg2 << DAC_Channel;

  DAC->CR = tmpreg1;
}


void DAC_StructInit(DAC_InitTypeDef* DAC_InitStruct)
{


  DAC_InitStruct->DAC_Trigger = DAC_Trigger_None;

  DAC_InitStruct->DAC_WaveGeneration = DAC_WaveGeneration_None;

  DAC_InitStruct->DAC_LFSRUnmask_TriangleAmplitude = DAC_LFSRUnmask_Bit0;

  DAC_InitStruct->DAC_OutputBuffer = DAC_OutputBuffer_Enable;
}


void DAC_Cmd(uint32_t DAC_Channel, FunctionalState NewState)
{

  assert_param(IS_DAC_CHANNEL(DAC_Channel));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    DAC->CR |= (DAC_CR_EN1 << DAC_Channel);
  }
  else
  {

    DAC->CR &= (~(DAC_CR_EN1 << DAC_Channel));
  }
}


void DAC_SoftwareTriggerCmd(uint32_t DAC_Channel, FunctionalState NewState)
{

  assert_param(IS_DAC_CHANNEL(DAC_Channel));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    DAC->SWTRIGR |= (uint32_t)DAC_SWTRIGR_SWTRIG1 << (DAC_Channel >> 4);
  }
  else
  {

    DAC->SWTRIGR &= ~((uint32_t)DAC_SWTRIGR_SWTRIG1 << (DAC_Channel >> 4));
  }
}


void DAC_DualSoftwareTriggerCmd(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    DAC->SWTRIGR |= DUAL_SWTRIG_SET;
  }
  else
  {

    DAC->SWTRIGR &= DUAL_SWTRIG_RESET;
  }
}


void DAC_WaveGenerationCmd(uint32_t DAC_Channel, uint32_t DAC_Wave, FunctionalState NewState)
{

  assert_param(IS_DAC_CHANNEL(DAC_Channel));
  assert_param(IS_DAC_WAVE(DAC_Wave));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    DAC->CR |= DAC_Wave << DAC_Channel;
  }
  else
  {

    DAC->CR &= ~(DAC_Wave << DAC_Channel);
  }
}


void DAC_SetChannel1Data(uint32_t DAC_Align, uint16_t Data)
{
  __IO uint32_t tmp = 0;


  assert_param(IS_DAC_ALIGN(DAC_Align));
  assert_param(IS_DAC_DATA(Data));

  tmp = (uint32_t)DAC_BASE;
  tmp += DHR12R1_OFFSET + DAC_Align;


  *(__IO uint32_t *) tmp = Data;
}


void DAC_SetChannel2Data(uint32_t DAC_Align, uint16_t Data)
{
  __IO uint32_t tmp = 0;


  assert_param(IS_DAC_ALIGN(DAC_Align));
  assert_param(IS_DAC_DATA(Data));

  tmp = (uint32_t)DAC_BASE;
  tmp += DHR12R2_OFFSET + DAC_Align;


  *(__IO uint32_t *)tmp = Data;
}


void DAC_SetDualChannelData(uint32_t DAC_Align, uint16_t Data2, uint16_t Data1)
{
  uint32_t data = 0, tmp = 0;


  assert_param(IS_DAC_ALIGN(DAC_Align));
  assert_param(IS_DAC_DATA(Data1));
  assert_param(IS_DAC_DATA(Data2));


  if (DAC_Align == DAC_Align_8b_R)
  {
    data = ((uint32_t)Data2 << 8) | Data1;
  }
  else
  {
    data = ((uint32_t)Data2 << 16) | Data1;
  }

  tmp = (uint32_t)DAC_BASE;
  tmp += DHR12RD_OFFSET + DAC_Align;


  *(__IO uint32_t *)tmp = data;
}


uint16_t DAC_GetDataOutputValue(uint32_t DAC_Channel)
{
  __IO uint32_t tmp = 0;


  assert_param(IS_DAC_CHANNEL(DAC_Channel));

  tmp = (uint32_t) DAC_BASE ;
  tmp += DOR_OFFSET + ((uint32_t)DAC_Channel >> 2);


  return (uint16_t) (*(__IO uint32_t*) tmp);
}


void DAC_DMACmd(uint32_t DAC_Channel, FunctionalState NewState)
{

  assert_param(IS_DAC_CHANNEL(DAC_Channel));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    DAC->CR |= (DAC_CR_DMAEN1 << DAC_Channel);
  }
  else
  {

    DAC->CR &= (~(DAC_CR_DMAEN1 << DAC_Channel));
  }
}


void DAC_ITConfig(uint32_t DAC_Channel, uint32_t DAC_IT, FunctionalState NewState)
{

  assert_param(IS_DAC_CHANNEL(DAC_Channel));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  assert_param(IS_DAC_IT(DAC_IT));

  if (NewState != DISABLE)
  {

    DAC->CR |=  (DAC_IT << DAC_Channel);
  }
  else
  {

    DAC->CR &= (~(uint32_t)(DAC_IT << DAC_Channel));
  }
}


FlagStatus DAC_GetFlagStatus(uint32_t DAC_Channel, uint32_t DAC_FLAG)
{
  FlagStatus bitstatus = RESET;

  assert_param(IS_DAC_CHANNEL(DAC_Channel));
  assert_param(IS_DAC_FLAG(DAC_FLAG));


  if ((DAC->SR & (DAC_FLAG << DAC_Channel)) != (uint8_t)RESET)
  {

    bitstatus = SET;
  }
  else
  {

    bitstatus = RESET;
  }

  return  bitstatus;
}


void DAC_ClearFlag(uint32_t DAC_Channel, uint32_t DAC_FLAG)
{

  assert_param(IS_DAC_CHANNEL(DAC_Channel));
  assert_param(IS_DAC_FLAG(DAC_FLAG));


  DAC->SR = (DAC_FLAG << DAC_Channel);
}


ITStatus DAC_GetITStatus(uint32_t DAC_Channel, uint32_t DAC_IT)
{
  ITStatus bitstatus = RESET;
  uint32_t enablestatus = 0;


  assert_param(IS_DAC_CHANNEL(DAC_Channel));
  assert_param(IS_DAC_IT(DAC_IT));


  enablestatus = (DAC->CR & (DAC_IT << DAC_Channel)) ;


  if (((DAC->SR & (DAC_IT << DAC_Channel)) != (uint32_t)RESET) && enablestatus)
  {

    bitstatus = SET;
  }
  else
  {

    bitstatus = RESET;
  }

  return  bitstatus;
}


void DAC_ClearITPendingBit(uint32_t DAC_Channel, uint32_t DAC_IT)
{

  assert_param(IS_DAC_CHANNEL(DAC_Channel));
  assert_param(IS_DAC_IT(DAC_IT));


  DAC->SR = (DAC_IT << DAC_Channel);
}
