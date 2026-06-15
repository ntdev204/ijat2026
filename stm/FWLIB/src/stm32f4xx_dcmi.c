/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        stm32f4xx_dcmi.c
 *
 * Description:  stm32f4xx_dcmi.c module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#include "stm32f4xx_dcmi.h"
#include "stm32f4xx_rcc.h"


void DCMI_DeInit(void)
{
  DCMI->CR = 0x0;
  DCMI->IER = 0x0;
  DCMI->ICR = 0x1F;
  DCMI->ESCR = 0x0;
  DCMI->ESUR = 0x0;
  DCMI->CWSTRTR = 0x0;
  DCMI->CWSIZER = 0x0;
}


void DCMI_Init(DCMI_InitTypeDef* DCMI_InitStruct)
{
  uint32_t temp = 0x0;


  assert_param(IS_DCMI_CAPTURE_MODE(DCMI_InitStruct->DCMI_CaptureMode));
  assert_param(IS_DCMI_SYNCHRO(DCMI_InitStruct->DCMI_SynchroMode));
  assert_param(IS_DCMI_PCKPOLARITY(DCMI_InitStruct->DCMI_PCKPolarity));
  assert_param(IS_DCMI_VSPOLARITY(DCMI_InitStruct->DCMI_VSPolarity));
  assert_param(IS_DCMI_HSPOLARITY(DCMI_InitStruct->DCMI_HSPolarity));
  assert_param(IS_DCMI_CAPTURE_RATE(DCMI_InitStruct->DCMI_CaptureRate));
  assert_param(IS_DCMI_EXTENDED_DATA(DCMI_InitStruct->DCMI_ExtendedDataMode));


  DCMI->CR &= ~(DCMI_CR_ENABLE | DCMI_CR_CAPTURE);


  temp = DCMI->CR;

  temp &= ~((uint32_t)DCMI_CR_CM     | DCMI_CR_ESS   | DCMI_CR_PCKPOL |
                      DCMI_CR_HSPOL  | DCMI_CR_VSPOL | DCMI_CR_FCRC_0 |
                      DCMI_CR_FCRC_1 | DCMI_CR_EDM_0 | DCMI_CR_EDM_1);


  temp |= ((uint32_t)DCMI_InitStruct->DCMI_CaptureMode |
                     DCMI_InitStruct->DCMI_SynchroMode |
                     DCMI_InitStruct->DCMI_PCKPolarity |
                     DCMI_InitStruct->DCMI_VSPolarity |
                     DCMI_InitStruct->DCMI_HSPolarity |
                     DCMI_InitStruct->DCMI_CaptureRate |
                     DCMI_InitStruct->DCMI_ExtendedDataMode);

  DCMI->CR = temp;
}


void DCMI_StructInit(DCMI_InitTypeDef* DCMI_InitStruct)
{

  DCMI_InitStruct->DCMI_CaptureMode = DCMI_CaptureMode_Continuous;
  DCMI_InitStruct->DCMI_SynchroMode = DCMI_SynchroMode_Hardware;
  DCMI_InitStruct->DCMI_PCKPolarity = DCMI_PCKPolarity_Falling;
  DCMI_InitStruct->DCMI_VSPolarity = DCMI_VSPolarity_Low;
  DCMI_InitStruct->DCMI_HSPolarity = DCMI_HSPolarity_Low;
  DCMI_InitStruct->DCMI_CaptureRate = DCMI_CaptureRate_All_Frame;
  DCMI_InitStruct->DCMI_ExtendedDataMode = DCMI_ExtendedDataMode_8b;
}


void DCMI_CROPConfig(DCMI_CROPInitTypeDef* DCMI_CROPInitStruct)
{

  DCMI->CWSTRTR = (uint32_t)((uint32_t)DCMI_CROPInitStruct->DCMI_HorizontalOffsetCount |
                  ((uint32_t)DCMI_CROPInitStruct->DCMI_VerticalStartLine << 16));


  DCMI->CWSIZER = (uint32_t)(DCMI_CROPInitStruct->DCMI_CaptureCount |
                  ((uint32_t)DCMI_CROPInitStruct->DCMI_VerticalLineCount << 16));
}


void DCMI_CROPCmd(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    DCMI->CR |= (uint32_t)DCMI_CR_CROP;
  }
  else
  {

    DCMI->CR &= ~(uint32_t)DCMI_CR_CROP;
  }
}


void DCMI_SetEmbeddedSynchroCodes(DCMI_CodesInitTypeDef* DCMI_CodesInitStruct)
{
  DCMI->ESCR = (uint32_t)(DCMI_CodesInitStruct->DCMI_FrameStartCode |
                          ((uint32_t)DCMI_CodesInitStruct->DCMI_LineStartCode << 8)|
                          ((uint32_t)DCMI_CodesInitStruct->DCMI_LineEndCode << 16)|
                          ((uint32_t)DCMI_CodesInitStruct->DCMI_FrameEndCode << 24));
}


void DCMI_JPEGCmd(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    DCMI->CR |= (uint32_t)DCMI_CR_JPEG;
  }
  else
  {

    DCMI->CR &= ~(uint32_t)DCMI_CR_JPEG;
  }
}


void DCMI_Cmd(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    DCMI->CR |= (uint32_t)DCMI_CR_ENABLE;
  }
  else
  {

    DCMI->CR &= ~(uint32_t)DCMI_CR_ENABLE;
  }
}


void DCMI_CaptureCmd(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    DCMI->CR |= (uint32_t)DCMI_CR_CAPTURE;
  }
  else
  {

    DCMI->CR &= ~(uint32_t)DCMI_CR_CAPTURE;
  }
}


uint32_t DCMI_ReadData(void)
{
  return DCMI->DR;
}


void DCMI_ITConfig(uint16_t DCMI_IT, FunctionalState NewState)
{

  assert_param(IS_DCMI_CONFIG_IT(DCMI_IT));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    DCMI->IER |= DCMI_IT;
  }
  else
  {

    DCMI->IER &= (uint16_t)(~DCMI_IT);
  }
}


FlagStatus DCMI_GetFlagStatus(uint16_t DCMI_FLAG)
{
  FlagStatus bitstatus = RESET;
  uint32_t dcmireg, tempreg = 0;


  assert_param(IS_DCMI_GET_FLAG(DCMI_FLAG));


  dcmireg = (((uint16_t)DCMI_FLAG) >> 12);

  if (dcmireg == 0x00)
  {
    tempreg= DCMI->RISR;
  }
  else if (dcmireg == 0x02)
  {
    tempreg = DCMI->SR;
  }
  else
  {
    tempreg = DCMI->MISR;
  }

  if ((tempreg & DCMI_FLAG) != (uint16_t)RESET )
  {
    bitstatus = SET;
  }
  else
  {
    bitstatus = RESET;
  }

  return  bitstatus;
}


void DCMI_ClearFlag(uint16_t DCMI_FLAG)
{

  assert_param(IS_DCMI_CLEAR_FLAG(DCMI_FLAG));


  DCMI->ICR = DCMI_FLAG;
}


ITStatus DCMI_GetITStatus(uint16_t DCMI_IT)
{
  ITStatus bitstatus = RESET;
  uint32_t itstatus = 0;


  assert_param(IS_DCMI_GET_IT(DCMI_IT));

  itstatus = DCMI->MISR & DCMI_IT;

  if ((itstatus != (uint16_t)RESET))
  {
    bitstatus = SET;
  }
  else
  {
    bitstatus = RESET;
  }
  return bitstatus;
}


void DCMI_ClearITPendingBit(uint16_t DCMI_IT)
{


  DCMI->ICR = DCMI_IT;
}
