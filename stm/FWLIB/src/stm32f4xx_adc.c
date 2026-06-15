/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        stm32f4xx_adc.c
 *
 * Description:  stm32f4xx_adc.c module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#include "stm32f4xx_adc.h"
#include "stm32f4xx_rcc.h"


#define CR1_DISCNUM_RESET         ((uint32_t)0xFFFF1FFF)


#define CR1_AWDCH_RESET           ((uint32_t)0xFFFFFFE0)


#define CR1_AWDMode_RESET         ((uint32_t)0xFF3FFDFF)


#define CR1_CLEAR_MASK            ((uint32_t)0xFCFFFEFF)


#define CR2_EXTEN_RESET           ((uint32_t)0xCFFFFFFF)


#define CR2_JEXTEN_RESET          ((uint32_t)0xFFCFFFFF)


#define CR2_JEXTSEL_RESET         ((uint32_t)0xFFF0FFFF)


#define CR2_CLEAR_MASK            ((uint32_t)0xC0FFF7FD)


#define SQR3_SQ_SET               ((uint32_t)0x0000001F)
#define SQR2_SQ_SET               ((uint32_t)0x0000001F)
#define SQR1_SQ_SET               ((uint32_t)0x0000001F)


#define SQR1_L_RESET              ((uint32_t)0xFF0FFFFF)


#define JSQR_JSQ_SET              ((uint32_t)0x0000001F)


#define JSQR_JL_SET               ((uint32_t)0x00300000)
#define JSQR_JL_RESET             ((uint32_t)0xFFCFFFFF)


#define SMPR1_SMP_SET             ((uint32_t)0x00000007)
#define SMPR2_SMP_SET             ((uint32_t)0x00000007)


#define JDR_OFFSET                ((uint8_t)0x28)


#define CDR_ADDRESS               ((uint32_t)0x40012308)


#define CR_CLEAR_MASK             ((uint32_t)0xFFFC30E0)


void ADC_DeInit(void)
{

  RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC, ENABLE);


  RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC, DISABLE);
}


void ADC_Init(ADC_TypeDef* ADCx, ADC_InitTypeDef* ADC_InitStruct)
{
  uint32_t tmpreg1 = 0;
  uint8_t tmpreg2 = 0;

  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_ADC_RESOLUTION(ADC_InitStruct->ADC_Resolution));
  assert_param(IS_FUNCTIONAL_STATE(ADC_InitStruct->ADC_ScanConvMode));
  assert_param(IS_FUNCTIONAL_STATE(ADC_InitStruct->ADC_ContinuousConvMode));
  assert_param(IS_ADC_EXT_TRIG_EDGE(ADC_InitStruct->ADC_ExternalTrigConvEdge));
  assert_param(IS_ADC_EXT_TRIG(ADC_InitStruct->ADC_ExternalTrigConv));
  assert_param(IS_ADC_DATA_ALIGN(ADC_InitStruct->ADC_DataAlign));
  assert_param(IS_ADC_REGULAR_LENGTH(ADC_InitStruct->ADC_NbrOfConversion));


  tmpreg1 = ADCx->CR1;


  tmpreg1 &= CR1_CLEAR_MASK;


  tmpreg1 |= (uint32_t)(((uint32_t)ADC_InitStruct->ADC_ScanConvMode << 8) | \
                                   ADC_InitStruct->ADC_Resolution);

  ADCx->CR1 = tmpreg1;


  tmpreg1 = ADCx->CR2;


  tmpreg1 &= CR2_CLEAR_MASK;


  tmpreg1 |= (uint32_t)(ADC_InitStruct->ADC_DataAlign | \
                        ADC_InitStruct->ADC_ExternalTrigConv |
                        ADC_InitStruct->ADC_ExternalTrigConvEdge | \
                        ((uint32_t)ADC_InitStruct->ADC_ContinuousConvMode << 1));


  ADCx->CR2 = tmpreg1;


  tmpreg1 = ADCx->SQR1;


  tmpreg1 &= SQR1_L_RESET;


  tmpreg2 |= (uint8_t)(ADC_InitStruct->ADC_NbrOfConversion - (uint8_t)1);
  tmpreg1 |= ((uint32_t)tmpreg2 << 20);


  ADCx->SQR1 = tmpreg1;
}


void ADC_StructInit(ADC_InitTypeDef* ADC_InitStruct)
{

  ADC_InitStruct->ADC_Resolution = ADC_Resolution_12b;


  ADC_InitStruct->ADC_ScanConvMode = DISABLE;


  ADC_InitStruct->ADC_ContinuousConvMode = DISABLE;


  ADC_InitStruct->ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;


  ADC_InitStruct->ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;


  ADC_InitStruct->ADC_DataAlign = ADC_DataAlign_Right;


  ADC_InitStruct->ADC_NbrOfConversion = 1;
}


void ADC_CommonInit(ADC_CommonInitTypeDef* ADC_CommonInitStruct)
{
  uint32_t tmpreg1 = 0;

  assert_param(IS_ADC_MODE(ADC_CommonInitStruct->ADC_Mode));
  assert_param(IS_ADC_PRESCALER(ADC_CommonInitStruct->ADC_Prescaler));
  assert_param(IS_ADC_DMA_ACCESS_MODE(ADC_CommonInitStruct->ADC_DMAAccessMode));
  assert_param(IS_ADC_SAMPLING_DELAY(ADC_CommonInitStruct->ADC_TwoSamplingDelay));


  tmpreg1 = ADC->CCR;


  tmpreg1 &= CR_CLEAR_MASK;


  tmpreg1 |= (uint32_t)(ADC_CommonInitStruct->ADC_Mode |
                        ADC_CommonInitStruct->ADC_Prescaler |
                        ADC_CommonInitStruct->ADC_DMAAccessMode |
                        ADC_CommonInitStruct->ADC_TwoSamplingDelay);


  ADC->CCR = tmpreg1;
}


void ADC_CommonStructInit(ADC_CommonInitTypeDef* ADC_CommonInitStruct)
{

  ADC_CommonInitStruct->ADC_Mode = ADC_Mode_Independent;


  ADC_CommonInitStruct->ADC_Prescaler = ADC_Prescaler_Div2;


  ADC_CommonInitStruct->ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;


  ADC_CommonInitStruct->ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
}


void ADC_Cmd(ADC_TypeDef* ADCx, FunctionalState NewState)
{

  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {

    ADCx->CR2 |= (uint32_t)ADC_CR2_ADON;
  }
  else
  {

    ADCx->CR2 &= (uint32_t)(~ADC_CR2_ADON);
  }
}


void ADC_AnalogWatchdogCmd(ADC_TypeDef* ADCx, uint32_t ADC_AnalogWatchdog)
{
  uint32_t tmpreg = 0;

  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_ADC_ANALOG_WATCHDOG(ADC_AnalogWatchdog));


  tmpreg = ADCx->CR1;


  tmpreg &= CR1_AWDMode_RESET;


  tmpreg |= ADC_AnalogWatchdog;


  ADCx->CR1 = tmpreg;
}


void ADC_AnalogWatchdogThresholdsConfig(ADC_TypeDef* ADCx, uint16_t HighThreshold,
                                        uint16_t LowThreshold)
{

  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_ADC_THRESHOLD(HighThreshold));
  assert_param(IS_ADC_THRESHOLD(LowThreshold));


  ADCx->HTR = HighThreshold;


  ADCx->LTR = LowThreshold;
}


void ADC_AnalogWatchdogSingleChannelConfig(ADC_TypeDef* ADCx, uint8_t ADC_Channel)
{
  uint32_t tmpreg = 0;

  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_ADC_CHANNEL(ADC_Channel));


  tmpreg = ADCx->CR1;


  tmpreg &= CR1_AWDCH_RESET;


  tmpreg |= ADC_Channel;


  ADCx->CR1 = tmpreg;
}


void ADC_TempSensorVrefintCmd(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {

    ADC->CCR |= (uint32_t)ADC_CCR_TSVREFE;
  }
  else
  {

    ADC->CCR &= (uint32_t)(~ADC_CCR_TSVREFE);
  }
}


void ADC_VBATCmd(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {

    ADC->CCR |= (uint32_t)ADC_CCR_VBATE;
  }
  else
  {

    ADC->CCR &= (uint32_t)(~ADC_CCR_VBATE);
  }
}


void ADC_RegularChannelConfig(ADC_TypeDef* ADCx, uint8_t ADC_Channel, uint8_t Rank, uint8_t ADC_SampleTime)
{
  uint32_t tmpreg1 = 0, tmpreg2 = 0;

  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_ADC_CHANNEL(ADC_Channel));
  assert_param(IS_ADC_REGULAR_RANK(Rank));
  assert_param(IS_ADC_SAMPLE_TIME(ADC_SampleTime));


  if (ADC_Channel > ADC_Channel_9)
  {

    tmpreg1 = ADCx->SMPR1;


    tmpreg2 = SMPR1_SMP_SET << (3 * (ADC_Channel - 10));


    tmpreg1 &= ~tmpreg2;


    tmpreg2 = (uint32_t)ADC_SampleTime << (3 * (ADC_Channel - 10));


    tmpreg1 |= tmpreg2;


    ADCx->SMPR1 = tmpreg1;
  }
  else
  {

    tmpreg1 = ADCx->SMPR2;


    tmpreg2 = SMPR2_SMP_SET << (3 * ADC_Channel);


    tmpreg1 &= ~tmpreg2;


    tmpreg2 = (uint32_t)ADC_SampleTime << (3 * ADC_Channel);


    tmpreg1 |= tmpreg2;


    ADCx->SMPR2 = tmpreg1;
  }

  if (Rank < 7)
  {

    tmpreg1 = ADCx->SQR3;


    tmpreg2 = SQR3_SQ_SET << (5 * (Rank - 1));


    tmpreg1 &= ~tmpreg2;


    tmpreg2 = (uint32_t)ADC_Channel << (5 * (Rank - 1));


    tmpreg1 |= tmpreg2;


    ADCx->SQR3 = tmpreg1;
  }

  else if (Rank < 13)
  {

    tmpreg1 = ADCx->SQR2;


    tmpreg2 = SQR2_SQ_SET << (5 * (Rank - 7));


    tmpreg1 &= ~tmpreg2;


    tmpreg2 = (uint32_t)ADC_Channel << (5 * (Rank - 7));


    tmpreg1 |= tmpreg2;


    ADCx->SQR2 = tmpreg1;
  }

  else
  {

    tmpreg1 = ADCx->SQR1;


    tmpreg2 = SQR1_SQ_SET << (5 * (Rank - 13));


    tmpreg1 &= ~tmpreg2;


    tmpreg2 = (uint32_t)ADC_Channel << (5 * (Rank - 13));


    tmpreg1 |= tmpreg2;


    ADCx->SQR1 = tmpreg1;
  }
}


void ADC_SoftwareStartConv(ADC_TypeDef* ADCx)
{

  assert_param(IS_ADC_ALL_PERIPH(ADCx));


  ADCx->CR2 |= (uint32_t)ADC_CR2_SWSTART;
}


FlagStatus ADC_GetSoftwareStartConvStatus(ADC_TypeDef* ADCx)
{
  FlagStatus bitstatus = RESET;

  assert_param(IS_ADC_ALL_PERIPH(ADCx));


  if ((ADCx->CR2 & ADC_CR2_SWSTART) != (uint32_t)RESET)
  {

    bitstatus = SET;
  }
  else
  {

    bitstatus = RESET;
  }


  return  bitstatus;
}


void ADC_EOCOnEachRegularChannelCmd(ADC_TypeDef* ADCx, FunctionalState NewState)
{

  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    ADCx->CR2 |= (uint32_t)ADC_CR2_EOCS;
  }
  else
  {

    ADCx->CR2 &= (uint32_t)(~ADC_CR2_EOCS);
  }
}


void ADC_ContinuousModeCmd(ADC_TypeDef* ADCx, FunctionalState NewState)
{

  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    ADCx->CR2 |= (uint32_t)ADC_CR2_CONT;
  }
  else
  {

    ADCx->CR2 &= (uint32_t)(~ADC_CR2_CONT);
  }
}


void ADC_DiscModeChannelCountConfig(ADC_TypeDef* ADCx, uint8_t Number)
{
  uint32_t tmpreg1 = 0;
  uint32_t tmpreg2 = 0;


  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_ADC_REGULAR_DISC_NUMBER(Number));


  tmpreg1 = ADCx->CR1;


  tmpreg1 &= CR1_DISCNUM_RESET;


  tmpreg2 = Number - 1;
  tmpreg1 |= tmpreg2 << 13;


  ADCx->CR1 = tmpreg1;
}


void ADC_DiscModeCmd(ADC_TypeDef* ADCx, FunctionalState NewState)
{

  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    ADCx->CR1 |= (uint32_t)ADC_CR1_DISCEN;
  }
  else
  {

    ADCx->CR1 &= (uint32_t)(~ADC_CR1_DISCEN);
  }
}


uint16_t ADC_GetConversionValue(ADC_TypeDef* ADCx)
{

  assert_param(IS_ADC_ALL_PERIPH(ADCx));


  return (uint16_t) ADCx->DR;
}


uint32_t ADC_GetMultiModeConversionValue(void)
{

  return (*(__IO uint32_t *) CDR_ADDRESS);
}


void ADC_DMACmd(ADC_TypeDef* ADCx, FunctionalState NewState)
{

  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {

    ADCx->CR2 |= (uint32_t)ADC_CR2_DMA;
  }
  else
  {

    ADCx->CR2 &= (uint32_t)(~ADC_CR2_DMA);
  }
}


void ADC_DMARequestAfterLastTransferCmd(ADC_TypeDef* ADCx, FunctionalState NewState)
{

  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {

    ADCx->CR2 |= (uint32_t)ADC_CR2_DDS;
  }
  else
  {

    ADCx->CR2 &= (uint32_t)(~ADC_CR2_DDS);
  }
}


void ADC_MultiModeDMARequestAfterLastTransferCmd(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {

    ADC->CCR |= (uint32_t)ADC_CCR_DDS;
  }
  else
  {

    ADC->CCR &= (uint32_t)(~ADC_CCR_DDS);
  }
}


void ADC_InjectedChannelConfig(ADC_TypeDef* ADCx, uint8_t ADC_Channel, uint8_t Rank, uint8_t ADC_SampleTime)
{
  uint32_t tmpreg1 = 0, tmpreg2 = 0, tmpreg3 = 0;

  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_ADC_CHANNEL(ADC_Channel));
  assert_param(IS_ADC_INJECTED_RANK(Rank));
  assert_param(IS_ADC_SAMPLE_TIME(ADC_SampleTime));

  if (ADC_Channel > ADC_Channel_9)
  {

    tmpreg1 = ADCx->SMPR1;

    tmpreg2 = SMPR1_SMP_SET << (3*(ADC_Channel - 10));

    tmpreg1 &= ~tmpreg2;

    tmpreg2 = (uint32_t)ADC_SampleTime << (3*(ADC_Channel - 10));

    tmpreg1 |= tmpreg2;

    ADCx->SMPR1 = tmpreg1;
  }
  else
  {

    tmpreg1 = ADCx->SMPR2;

    tmpreg2 = SMPR2_SMP_SET << (3 * ADC_Channel);

    tmpreg1 &= ~tmpreg2;

    tmpreg2 = (uint32_t)ADC_SampleTime << (3 * ADC_Channel);

    tmpreg1 |= tmpreg2;

    ADCx->SMPR2 = tmpreg1;
  }


  tmpreg1 = ADCx->JSQR;

  tmpreg3 =  (tmpreg1 & JSQR_JL_SET)>> 20;

  tmpreg2 = JSQR_JSQ_SET << (5 * (uint8_t)((Rank + 3) - (tmpreg3 + 1)));

  tmpreg1 &= ~tmpreg2;

  tmpreg2 = (uint32_t)ADC_Channel << (5 * (uint8_t)((Rank + 3) - (tmpreg3 + 1)));

  tmpreg1 |= tmpreg2;

  ADCx->JSQR = tmpreg1;
}


void ADC_InjectedSequencerLengthConfig(ADC_TypeDef* ADCx, uint8_t Length)
{
  uint32_t tmpreg1 = 0;
  uint32_t tmpreg2 = 0;

  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_ADC_INJECTED_LENGTH(Length));


  tmpreg1 = ADCx->JSQR;


  tmpreg1 &= JSQR_JL_RESET;


  tmpreg2 = Length - 1;
  tmpreg1 |= tmpreg2 << 20;


  ADCx->JSQR = tmpreg1;
}


void ADC_SetInjectedOffset(ADC_TypeDef* ADCx, uint8_t ADC_InjectedChannel, uint16_t Offset)
{
    __IO uint32_t tmp = 0;

  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_ADC_INJECTED_CHANNEL(ADC_InjectedChannel));
  assert_param(IS_ADC_OFFSET(Offset));

  tmp = (uint32_t)ADCx;
  tmp += ADC_InjectedChannel;


 *(__IO uint32_t *) tmp = (uint32_t)Offset;
}


void ADC_ExternalTrigInjectedConvConfig(ADC_TypeDef* ADCx, uint32_t ADC_ExternalTrigInjecConv)
{
  uint32_t tmpreg = 0;

  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_ADC_EXT_INJEC_TRIG(ADC_ExternalTrigInjecConv));


  tmpreg = ADCx->CR2;


  tmpreg &= CR2_JEXTSEL_RESET;


  tmpreg |= ADC_ExternalTrigInjecConv;


  ADCx->CR2 = tmpreg;
}


void ADC_ExternalTrigInjectedConvEdgeConfig(ADC_TypeDef* ADCx, uint32_t ADC_ExternalTrigInjecConvEdge)
{
  uint32_t tmpreg = 0;

  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_ADC_EXT_INJEC_TRIG_EDGE(ADC_ExternalTrigInjecConvEdge));

  tmpreg = ADCx->CR2;

  tmpreg &= CR2_JEXTEN_RESET;

  tmpreg |= ADC_ExternalTrigInjecConvEdge;

  ADCx->CR2 = tmpreg;
}


void ADC_SoftwareStartInjectedConv(ADC_TypeDef* ADCx)
{

  assert_param(IS_ADC_ALL_PERIPH(ADCx));

  ADCx->CR2 |= (uint32_t)ADC_CR2_JSWSTART;
}


FlagStatus ADC_GetSoftwareStartInjectedConvCmdStatus(ADC_TypeDef* ADCx)
{
  FlagStatus bitstatus = RESET;

  assert_param(IS_ADC_ALL_PERIPH(ADCx));


  if ((ADCx->CR2 & ADC_CR2_JSWSTART) != (uint32_t)RESET)
  {

    bitstatus = SET;
  }
  else
  {

    bitstatus = RESET;
  }

  return  bitstatus;
}


void ADC_AutoInjectedConvCmd(ADC_TypeDef* ADCx, FunctionalState NewState)
{

  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {

    ADCx->CR1 |= (uint32_t)ADC_CR1_JAUTO;
  }
  else
  {

    ADCx->CR1 &= (uint32_t)(~ADC_CR1_JAUTO);
  }
}


void ADC_InjectedDiscModeCmd(ADC_TypeDef* ADCx, FunctionalState NewState)
{

  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {

    ADCx->CR1 |= (uint32_t)ADC_CR1_JDISCEN;
  }
  else
  {

    ADCx->CR1 &= (uint32_t)(~ADC_CR1_JDISCEN);
  }
}


uint16_t ADC_GetInjectedConversionValue(ADC_TypeDef* ADCx, uint8_t ADC_InjectedChannel)
{
  __IO uint32_t tmp = 0;


  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_ADC_INJECTED_CHANNEL(ADC_InjectedChannel));

  tmp = (uint32_t)ADCx;
  tmp += ADC_InjectedChannel + JDR_OFFSET;


  return (uint16_t) (*(__IO uint32_t*)  tmp);
}


void ADC_ITConfig(ADC_TypeDef* ADCx, uint16_t ADC_IT, FunctionalState NewState)
{
  uint32_t itmask = 0;

  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  assert_param(IS_ADC_IT(ADC_IT));


  itmask = (uint8_t)ADC_IT;
  itmask = (uint32_t)0x01 << itmask;

  if (NewState != DISABLE)
  {

    ADCx->CR1 |= itmask;
  }
  else
  {

    ADCx->CR1 &= (~(uint32_t)itmask);
  }
}


FlagStatus ADC_GetFlagStatus(ADC_TypeDef* ADCx, uint8_t ADC_FLAG)
{
  FlagStatus bitstatus = RESET;

  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_ADC_GET_FLAG(ADC_FLAG));


  if ((ADCx->SR & ADC_FLAG) != (uint8_t)RESET)
  {

    bitstatus = SET;
  }
  else
  {

    bitstatus = RESET;
  }

  return  bitstatus;
}


void ADC_ClearFlag(ADC_TypeDef* ADCx, uint8_t ADC_FLAG)
{

  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_ADC_CLEAR_FLAG(ADC_FLAG));


  ADCx->SR = ~(uint32_t)ADC_FLAG;
}


ITStatus ADC_GetITStatus(ADC_TypeDef* ADCx, uint16_t ADC_IT)
{
  ITStatus bitstatus = RESET;
  uint32_t itmask = 0, enablestatus = 0;


  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_ADC_IT(ADC_IT));


  itmask = ADC_IT >> 8;


  enablestatus = (ADCx->CR1 & ((uint32_t)0x01 << (uint8_t)ADC_IT)) ;


  if (((ADCx->SR & itmask) != (uint32_t)RESET) && enablestatus)
  {

    bitstatus = SET;
  }
  else
  {

    bitstatus = RESET;
  }

  return  bitstatus;
}


void ADC_ClearITPendingBit(ADC_TypeDef* ADCx, uint16_t ADC_IT)
{
  uint8_t itmask = 0;

  assert_param(IS_ADC_ALL_PERIPH(ADCx));
  assert_param(IS_ADC_IT(ADC_IT));

  itmask = (uint8_t)(ADC_IT >> 8);

  ADCx->SR = ~(uint32_t)itmask;
}
