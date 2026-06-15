/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        stm32f4xx_usart.c
 *
 * Description:  stm32f4xx_usart.c module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#include "stm32f4xx_usart.h"
#include "stm32f4xx_rcc.h"


#define CR1_CLEAR_MASK            ((uint16_t)(USART_CR1_M | USART_CR1_PCE | \
                                              USART_CR1_PS | USART_CR1_TE | \
                                              USART_CR1_RE))


#define CR2_CLOCK_CLEAR_MASK      ((uint16_t)(USART_CR2_CLKEN | USART_CR2_CPOL | \
                                              USART_CR2_CPHA | USART_CR2_LBCL))


#define CR3_CLEAR_MASK            ((uint16_t)(USART_CR3_RTSE | USART_CR3_CTSE))


#define IT_MASK                   ((uint16_t)0x001F)


void USART_DeInit(USART_TypeDef* USARTx)
{

  assert_param(IS_USART_ALL_PERIPH(USARTx));

  if (USARTx == USART1)
  {
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_USART1, ENABLE);
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_USART1, DISABLE);
  }
  else if (USARTx == USART2)
  {
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_USART2, ENABLE);
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_USART2, DISABLE);
  }
  else if (USARTx == USART3)
  {
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_USART3, ENABLE);
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_USART3, DISABLE);
  }
  else if (USARTx == UART4)
  {
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_UART4, ENABLE);
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_UART4, DISABLE);
  }
  else if (USARTx == UART5)
  {
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_UART5, ENABLE);
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_UART5, DISABLE);
  }
  else if (USARTx == USART6)
  {
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_USART6, ENABLE);
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_USART6, DISABLE);
  }
  else if (USARTx == UART7)
  {
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_UART7, ENABLE);
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_UART7, DISABLE);
  }
  else
  {
    if (USARTx == UART8)
    {
      RCC_APB1PeriphResetCmd(RCC_APB1Periph_UART8, ENABLE);
      RCC_APB1PeriphResetCmd(RCC_APB1Periph_UART8, DISABLE);
    }
  }
}


void USART_Init(USART_TypeDef* USARTx, USART_InitTypeDef* USART_InitStruct)
{
  uint32_t tmpreg = 0x00, apbclock = 0x00;
  uint32_t integerdivider = 0x00;
  uint32_t fractionaldivider = 0x00;
  RCC_ClocksTypeDef RCC_ClocksStatus;


  assert_param(IS_USART_ALL_PERIPH(USARTx));
  assert_param(IS_USART_BAUDRATE(USART_InitStruct->USART_BaudRate));
  assert_param(IS_USART_WORD_LENGTH(USART_InitStruct->USART_WordLength));
  assert_param(IS_USART_STOPBITS(USART_InitStruct->USART_StopBits));
  assert_param(IS_USART_PARITY(USART_InitStruct->USART_Parity));
  assert_param(IS_USART_MODE(USART_InitStruct->USART_Mode));
  assert_param(IS_USART_HARDWARE_FLOW_CONTROL(USART_InitStruct->USART_HardwareFlowControl));


  if (USART_InitStruct->USART_HardwareFlowControl != USART_HardwareFlowControl_None)
  {
    assert_param(IS_USART_1236_PERIPH(USARTx));
  }


  tmpreg = USARTx->CR2;


  tmpreg &= (uint32_t)~((uint32_t)USART_CR2_STOP);


  tmpreg |= (uint32_t)USART_InitStruct->USART_StopBits;


  USARTx->CR2 = (uint16_t)tmpreg;


  tmpreg = USARTx->CR1;


  tmpreg &= (uint32_t)~((uint32_t)CR1_CLEAR_MASK);


  tmpreg |= (uint32_t)USART_InitStruct->USART_WordLength | USART_InitStruct->USART_Parity |
            USART_InitStruct->USART_Mode;


  USARTx->CR1 = (uint16_t)tmpreg;


  tmpreg = USARTx->CR3;


  tmpreg &= (uint32_t)~((uint32_t)CR3_CLEAR_MASK);


  tmpreg |= USART_InitStruct->USART_HardwareFlowControl;


  USARTx->CR3 = (uint16_t)tmpreg;


  RCC_GetClocksFreq(&RCC_ClocksStatus);

  if ((USARTx == USART1) || (USARTx == USART6))
  {
    apbclock = RCC_ClocksStatus.PCLK2_Frequency;
  }
  else
  {
    apbclock = RCC_ClocksStatus.PCLK1_Frequency;
  }


  if ((USARTx->CR1 & USART_CR1_OVER8) != 0)
  {

    integerdivider = ((25 * apbclock) / (2 * (USART_InitStruct->USART_BaudRate)));
  }
  else
  {

    integerdivider = ((25 * apbclock) / (4 * (USART_InitStruct->USART_BaudRate)));
  }
  tmpreg = (integerdivider / 100) << 4;


  fractionaldivider = integerdivider - (100 * (tmpreg >> 4));


  if ((USARTx->CR1 & USART_CR1_OVER8) != 0)
  {
    tmpreg |= ((((fractionaldivider * 8) + 50) / 100)) & ((uint8_t)0x07);
  }
  else
  {
    tmpreg |= ((((fractionaldivider * 16) + 50) / 100)) & ((uint8_t)0x0F);
  }


  USARTx->BRR = (uint16_t)tmpreg;
}


void USART_StructInit(USART_InitTypeDef* USART_InitStruct)
{

  USART_InitStruct->USART_BaudRate = 9600;
  USART_InitStruct->USART_WordLength = USART_WordLength_8b;
  USART_InitStruct->USART_StopBits = USART_StopBits_1;
  USART_InitStruct->USART_Parity = USART_Parity_No ;
  USART_InitStruct->USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  USART_InitStruct->USART_HardwareFlowControl = USART_HardwareFlowControl_None;
}


void USART_ClockInit(USART_TypeDef* USARTx, USART_ClockInitTypeDef* USART_ClockInitStruct)
{
  uint32_t tmpreg = 0x00;

  assert_param(IS_USART_1236_PERIPH(USARTx));
  assert_param(IS_USART_CLOCK(USART_ClockInitStruct->USART_Clock));
  assert_param(IS_USART_CPOL(USART_ClockInitStruct->USART_CPOL));
  assert_param(IS_USART_CPHA(USART_ClockInitStruct->USART_CPHA));
  assert_param(IS_USART_LASTBIT(USART_ClockInitStruct->USART_LastBit));


  tmpreg = USARTx->CR2;

  tmpreg &= (uint32_t)~((uint32_t)CR2_CLOCK_CLEAR_MASK);


  tmpreg |= (uint32_t)USART_ClockInitStruct->USART_Clock | USART_ClockInitStruct->USART_CPOL |
                 USART_ClockInitStruct->USART_CPHA | USART_ClockInitStruct->USART_LastBit;

  USARTx->CR2 = (uint16_t)tmpreg;
}


void USART_ClockStructInit(USART_ClockInitTypeDef* USART_ClockInitStruct)
{

  USART_ClockInitStruct->USART_Clock = USART_Clock_Disable;
  USART_ClockInitStruct->USART_CPOL = USART_CPOL_Low;
  USART_ClockInitStruct->USART_CPHA = USART_CPHA_1Edge;
  USART_ClockInitStruct->USART_LastBit = USART_LastBit_Disable;
}


void USART_Cmd(USART_TypeDef* USARTx, FunctionalState NewState)
{

  assert_param(IS_USART_ALL_PERIPH(USARTx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    USARTx->CR1 |= USART_CR1_UE;
  }
  else
  {

    USARTx->CR1 &= (uint16_t)~((uint16_t)USART_CR1_UE);
  }
}


void USART_SetPrescaler(USART_TypeDef* USARTx, uint8_t USART_Prescaler)
{

  assert_param(IS_USART_ALL_PERIPH(USARTx));


  USARTx->GTPR &= USART_GTPR_GT;

  USARTx->GTPR |= USART_Prescaler;
}


void USART_OverSampling8Cmd(USART_TypeDef* USARTx, FunctionalState NewState)
{

  assert_param(IS_USART_ALL_PERIPH(USARTx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    USARTx->CR1 |= USART_CR1_OVER8;
  }
  else
  {

    USARTx->CR1 &= (uint16_t)~((uint16_t)USART_CR1_OVER8);
  }
}


void USART_OneBitMethodCmd(USART_TypeDef* USARTx, FunctionalState NewState)
{

  assert_param(IS_USART_ALL_PERIPH(USARTx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    USARTx->CR3 |= USART_CR3_ONEBIT;
  }
  else
  {

    USARTx->CR3 &= (uint16_t)~((uint16_t)USART_CR3_ONEBIT);
  }
}


void USART_SendData(USART_TypeDef* USARTx, uint16_t Data)
{

  assert_param(IS_USART_ALL_PERIPH(USARTx));
  assert_param(IS_USART_DATA(Data));


  USARTx->DR = (Data & (uint16_t)0x01FF);
}


uint16_t USART_ReceiveData(USART_TypeDef* USARTx)
{

  assert_param(IS_USART_ALL_PERIPH(USARTx));


  return (uint16_t)(USARTx->DR & (uint16_t)0x01FF);
}


void USART_SetAddress(USART_TypeDef* USARTx, uint8_t USART_Address)
{

  assert_param(IS_USART_ALL_PERIPH(USARTx));
  assert_param(IS_USART_ADDRESS(USART_Address));


  USARTx->CR2 &= (uint16_t)~((uint16_t)USART_CR2_ADD);

  USARTx->CR2 |= USART_Address;
}


void USART_ReceiverWakeUpCmd(USART_TypeDef* USARTx, FunctionalState NewState)
{

  assert_param(IS_USART_ALL_PERIPH(USARTx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    USARTx->CR1 |= USART_CR1_RWU;
  }
  else
  {

    USARTx->CR1 &= (uint16_t)~((uint16_t)USART_CR1_RWU);
  }
}

void USART_WakeUpConfig(USART_TypeDef* USARTx, uint16_t USART_WakeUp)
{

  assert_param(IS_USART_ALL_PERIPH(USARTx));
  assert_param(IS_USART_WAKEUP(USART_WakeUp));

  USARTx->CR1 &= (uint16_t)~((uint16_t)USART_CR1_WAKE);
  USARTx->CR1 |= USART_WakeUp;
}


void USART_LINBreakDetectLengthConfig(USART_TypeDef* USARTx, uint16_t USART_LINBreakDetectLength)
{

  assert_param(IS_USART_ALL_PERIPH(USARTx));
  assert_param(IS_USART_LIN_BREAK_DETECT_LENGTH(USART_LINBreakDetectLength));

  USARTx->CR2 &= (uint16_t)~((uint16_t)USART_CR2_LBDL);
  USARTx->CR2 |= USART_LINBreakDetectLength;
}


void USART_LINCmd(USART_TypeDef* USARTx, FunctionalState NewState)
{

  assert_param(IS_USART_ALL_PERIPH(USARTx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    USARTx->CR2 |= USART_CR2_LINEN;
  }
  else
  {

    USARTx->CR2 &= (uint16_t)~((uint16_t)USART_CR2_LINEN);
  }
}


void USART_SendBreak(USART_TypeDef* USARTx)
{

  assert_param(IS_USART_ALL_PERIPH(USARTx));


  USARTx->CR1 |= USART_CR1_SBK;
}


void USART_HalfDuplexCmd(USART_TypeDef* USARTx, FunctionalState NewState)
{

  assert_param(IS_USART_ALL_PERIPH(USARTx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    USARTx->CR3 |= USART_CR3_HDSEL;
  }
  else
  {

    USARTx->CR3 &= (uint16_t)~((uint16_t)USART_CR3_HDSEL);
  }
}


void USART_SetGuardTime(USART_TypeDef* USARTx, uint8_t USART_GuardTime)
{

  assert_param(IS_USART_1236_PERIPH(USARTx));


  USARTx->GTPR &= USART_GTPR_PSC;

  USARTx->GTPR |= (uint16_t)((uint16_t)USART_GuardTime << 0x08);
}


void USART_SmartCardCmd(USART_TypeDef* USARTx, FunctionalState NewState)
{

  assert_param(IS_USART_1236_PERIPH(USARTx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {

    USARTx->CR3 |= USART_CR3_SCEN;
  }
  else
  {

    USARTx->CR3 &= (uint16_t)~((uint16_t)USART_CR3_SCEN);
  }
}


void USART_SmartCardNACKCmd(USART_TypeDef* USARTx, FunctionalState NewState)
{

  assert_param(IS_USART_1236_PERIPH(USARTx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {

    USARTx->CR3 |= USART_CR3_NACK;
  }
  else
  {

    USARTx->CR3 &= (uint16_t)~((uint16_t)USART_CR3_NACK);
  }
}


void USART_IrDAConfig(USART_TypeDef* USARTx, uint16_t USART_IrDAMode)
{

  assert_param(IS_USART_ALL_PERIPH(USARTx));
  assert_param(IS_USART_IRDA_MODE(USART_IrDAMode));

  USARTx->CR3 &= (uint16_t)~((uint16_t)USART_CR3_IRLP);
  USARTx->CR3 |= USART_IrDAMode;
}


void USART_IrDACmd(USART_TypeDef* USARTx, FunctionalState NewState)
{

  assert_param(IS_USART_ALL_PERIPH(USARTx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    USARTx->CR3 |= USART_CR3_IREN;
  }
  else
  {

    USARTx->CR3 &= (uint16_t)~((uint16_t)USART_CR3_IREN);
  }
}


void USART_DMACmd(USART_TypeDef* USARTx, uint16_t USART_DMAReq, FunctionalState NewState)
{

  assert_param(IS_USART_ALL_PERIPH(USARTx));
  assert_param(IS_USART_DMAREQ(USART_DMAReq));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    USARTx->CR3 |= USART_DMAReq;
  }
  else
  {

    USARTx->CR3 &= (uint16_t)~USART_DMAReq;
  }
}


void USART_ITConfig(USART_TypeDef* USARTx, uint16_t USART_IT, FunctionalState NewState)
{
  uint32_t usartreg = 0x00, itpos = 0x00, itmask = 0x00;
  uint32_t usartxbase = 0x00;

  assert_param(IS_USART_ALL_PERIPH(USARTx));
  assert_param(IS_USART_CONFIG_IT(USART_IT));
  assert_param(IS_FUNCTIONAL_STATE(NewState));


  if (USART_IT == USART_IT_CTS)
  {
    assert_param(IS_USART_1236_PERIPH(USARTx));
  }

  usartxbase = (uint32_t)USARTx;


  usartreg = (((uint8_t)USART_IT) >> 0x05);


  itpos = USART_IT & IT_MASK;
  itmask = (((uint32_t)0x01) << itpos);

  if (usartreg == 0x01)
  {
    usartxbase += 0x0C;
  }
  else if (usartreg == 0x02)
  {
    usartxbase += 0x10;
  }
  else
  {
    usartxbase += 0x14;
  }
  if (NewState != DISABLE)
  {
    *(__IO uint32_t*)usartxbase  |= itmask;
  }
  else
  {
    *(__IO uint32_t*)usartxbase &= ~itmask;
  }
}


FlagStatus USART_GetFlagStatus(USART_TypeDef* USARTx, uint16_t USART_FLAG)
{
  FlagStatus bitstatus = RESET;

  assert_param(IS_USART_ALL_PERIPH(USARTx));
  assert_param(IS_USART_FLAG(USART_FLAG));


  if (USART_FLAG == USART_FLAG_CTS)
  {
    assert_param(IS_USART_1236_PERIPH(USARTx));
  }

  if ((USARTx->SR & USART_FLAG) != (uint16_t)RESET)
  {
    bitstatus = SET;
  }
  else
  {
    bitstatus = RESET;
  }
  return bitstatus;
}


void USART_ClearFlag(USART_TypeDef* USARTx, uint16_t USART_FLAG)
{

  assert_param(IS_USART_ALL_PERIPH(USARTx));
  assert_param(IS_USART_CLEAR_FLAG(USART_FLAG));


  if ((USART_FLAG & USART_FLAG_CTS) == USART_FLAG_CTS)
  {
    assert_param(IS_USART_1236_PERIPH(USARTx));
  }

  USARTx->SR = (uint16_t)~USART_FLAG;
}


ITStatus USART_GetITStatus(USART_TypeDef* USARTx, uint16_t USART_IT)
{
  uint32_t bitpos = 0x00, itmask = 0x00, usartreg = 0x00;
  ITStatus bitstatus = RESET;

  assert_param(IS_USART_ALL_PERIPH(USARTx));
  assert_param(IS_USART_GET_IT(USART_IT));


  if (USART_IT == USART_IT_CTS)
  {
    assert_param(IS_USART_1236_PERIPH(USARTx));
  }


  usartreg = (((uint8_t)USART_IT) >> 0x05);

  itmask = USART_IT & IT_MASK;
  itmask = (uint32_t)0x01 << itmask;

  if (usartreg == 0x01)
  {
    itmask &= USARTx->CR1;
  }
  else if (usartreg == 0x02)
  {
    itmask &= USARTx->CR2;
  }
  else
  {
    itmask &= USARTx->CR3;
  }

  bitpos = USART_IT >> 0x08;
  bitpos = (uint32_t)0x01 << bitpos;
  bitpos &= USARTx->SR;
  if ((itmask != (uint16_t)RESET)&&(bitpos != (uint16_t)RESET))
  {
    bitstatus = SET;
  }
  else
  {
    bitstatus = RESET;
  }

  return bitstatus;
}


void USART_ClearITPendingBit(USART_TypeDef* USARTx, uint16_t USART_IT)
{
  uint16_t bitpos = 0x00, itmask = 0x00;

  assert_param(IS_USART_ALL_PERIPH(USARTx));
  assert_param(IS_USART_CLEAR_IT(USART_IT));


  if (USART_IT == USART_IT_CTS)
  {
    assert_param(IS_USART_1236_PERIPH(USARTx));
  }

  bitpos = USART_IT >> 0x08;
  itmask = ((uint16_t)0x01 << (uint16_t)bitpos);
  USARTx->SR = (uint16_t)~itmask;
}
