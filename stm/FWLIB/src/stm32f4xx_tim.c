/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        stm32f4xx_tim.c
 *
 * Description:  stm32f4xx_tim.c module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#include "stm32f4xx_tim.h"
#include "stm32f4xx_rcc.h"


#define SMCR_ETR_MASK      ((uint16_t)0x00FF)
#define CCMR_OFFSET        ((uint16_t)0x0018)
#define CCER_CCE_SET       ((uint16_t)0x0001)
#define	CCER_CCNE_SET      ((uint16_t)0x0004)
#define CCMR_OC13M_MASK    ((uint16_t)0xFF8F)
#define CCMR_OC24M_MASK    ((uint16_t)0x8FFF)


static void TI1_Config(TIM_TypeDef* TIMx, uint16_t TIM_ICPolarity, uint16_t TIM_ICSelection,
                       uint16_t TIM_ICFilter);
static void TI2_Config(TIM_TypeDef* TIMx, uint16_t TIM_ICPolarity, uint16_t TIM_ICSelection,
                       uint16_t TIM_ICFilter);
static void TI3_Config(TIM_TypeDef* TIMx, uint16_t TIM_ICPolarity, uint16_t TIM_ICSelection,
                       uint16_t TIM_ICFilter);
static void TI4_Config(TIM_TypeDef* TIMx, uint16_t TIM_ICPolarity, uint16_t TIM_ICSelection,
                       uint16_t TIM_ICFilter);


void TIM_DeInit(TIM_TypeDef* TIMx)
{

  assert_param(IS_TIM_ALL_PERIPH(TIMx));

  if (TIMx == TIM1)
  {
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_TIM1, ENABLE);
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_TIM1, DISABLE);
  }
  else if (TIMx == TIM2)
  {
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_TIM2, ENABLE);
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_TIM2, DISABLE);
  }
  else if (TIMx == TIM3)
  {
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_TIM3, ENABLE);
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_TIM3, DISABLE);
  }
  else if (TIMx == TIM4)
  {
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_TIM4, ENABLE);
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_TIM4, DISABLE);
  }
  else if (TIMx == TIM5)
  {
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_TIM5, ENABLE);
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_TIM5, DISABLE);
  }
  else if (TIMx == TIM6)
  {
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_TIM6, ENABLE);
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_TIM6, DISABLE);
  }
  else if (TIMx == TIM7)
  {
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_TIM7, ENABLE);
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_TIM7, DISABLE);
  }
  else if (TIMx == TIM8)
  {
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_TIM8, ENABLE);
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_TIM8, DISABLE);
  }
  else if (TIMx == TIM9)
  {
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_TIM9, ENABLE);
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_TIM9, DISABLE);
   }
  else if (TIMx == TIM10)
  {
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_TIM10, ENABLE);
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_TIM10, DISABLE);
  }
  else if (TIMx == TIM11)
  {
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_TIM11, ENABLE);
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_TIM11, DISABLE);
  }
  else if (TIMx == TIM12)
  {
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_TIM12, ENABLE);
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_TIM12, DISABLE);
  }
  else if (TIMx == TIM13)
  {
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_TIM13, ENABLE);
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_TIM13, DISABLE);
  }
  else
  {
    if (TIMx == TIM14)
    {
      RCC_APB1PeriphResetCmd(RCC_APB1Periph_TIM14, ENABLE);
      RCC_APB1PeriphResetCmd(RCC_APB1Periph_TIM14, DISABLE);
    }
  }
}


void TIM_TimeBaseInit(TIM_TypeDef* TIMx, TIM_TimeBaseInitTypeDef* TIM_TimeBaseInitStruct)
{
  uint16_t tmpcr1 = 0;


  assert_param(IS_TIM_ALL_PERIPH(TIMx));
  assert_param(IS_TIM_COUNTER_MODE(TIM_TimeBaseInitStruct->TIM_CounterMode));
  assert_param(IS_TIM_CKD_DIV(TIM_TimeBaseInitStruct->TIM_ClockDivision));

  tmpcr1 = TIMx->CR1;

  if((TIMx == TIM1) || (TIMx == TIM8)||
     (TIMx == TIM2) || (TIMx == TIM3)||
     (TIMx == TIM4) || (TIMx == TIM5))
  {

    tmpcr1 &= (uint16_t)(~(TIM_CR1_DIR | TIM_CR1_CMS));
    tmpcr1 |= (uint32_t)TIM_TimeBaseInitStruct->TIM_CounterMode;
  }

  if((TIMx != TIM6) && (TIMx != TIM7))
  {

    tmpcr1 &=  (uint16_t)(~TIM_CR1_CKD);
    tmpcr1 |= (uint32_t)TIM_TimeBaseInitStruct->TIM_ClockDivision;
  }

  TIMx->CR1 = tmpcr1;


  TIMx->ARR = TIM_TimeBaseInitStruct->TIM_Period ;


  TIMx->PSC = TIM_TimeBaseInitStruct->TIM_Prescaler;

  if ((TIMx == TIM1) || (TIMx == TIM8))
  {

    TIMx->RCR = TIM_TimeBaseInitStruct->TIM_RepetitionCounter;
  }


  TIMx->EGR = TIM_PSCReloadMode_Immediate;
}


void TIM_TimeBaseStructInit(TIM_TimeBaseInitTypeDef* TIM_TimeBaseInitStruct)
{

  TIM_TimeBaseInitStruct->TIM_Period = 0xFFFFFFFF;
  TIM_TimeBaseInitStruct->TIM_Prescaler = 0x0000;
  TIM_TimeBaseInitStruct->TIM_ClockDivision = TIM_CKD_DIV1;
  TIM_TimeBaseInitStruct->TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInitStruct->TIM_RepetitionCounter = 0x0000;
}


void TIM_PrescalerConfig(TIM_TypeDef* TIMx, uint16_t Prescaler, uint16_t TIM_PSCReloadMode)
{

  assert_param(IS_TIM_ALL_PERIPH(TIMx));
  assert_param(IS_TIM_PRESCALER_RELOAD(TIM_PSCReloadMode));

  TIMx->PSC = Prescaler;

  TIMx->EGR = TIM_PSCReloadMode;
}


void TIM_CounterModeConfig(TIM_TypeDef* TIMx, uint16_t TIM_CounterMode)
{
  uint16_t tmpcr1 = 0;


  assert_param(IS_TIM_LIST3_PERIPH(TIMx));
  assert_param(IS_TIM_COUNTER_MODE(TIM_CounterMode));

  tmpcr1 = TIMx->CR1;


  tmpcr1 &= (uint16_t)~(TIM_CR1_DIR | TIM_CR1_CMS);


  tmpcr1 |= TIM_CounterMode;


  TIMx->CR1 = tmpcr1;
}


void TIM_SetCounter(TIM_TypeDef* TIMx, uint32_t Counter)
{

   assert_param(IS_TIM_ALL_PERIPH(TIMx));


  TIMx->CNT = Counter;
}


void TIM_SetAutoreload(TIM_TypeDef* TIMx, uint32_t Autoreload)
{

  assert_param(IS_TIM_ALL_PERIPH(TIMx));


  TIMx->ARR = Autoreload;
}


uint32_t TIM_GetCounter(TIM_TypeDef* TIMx)
{

  assert_param(IS_TIM_ALL_PERIPH(TIMx));


  return TIMx->CNT;
}


uint16_t TIM_GetPrescaler(TIM_TypeDef* TIMx)
{

  assert_param(IS_TIM_ALL_PERIPH(TIMx));


  return TIMx->PSC;
}


void TIM_UpdateDisableConfig(TIM_TypeDef* TIMx, FunctionalState NewState)
{

  assert_param(IS_TIM_ALL_PERIPH(TIMx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    TIMx->CR1 |= TIM_CR1_UDIS;
  }
  else
  {

    TIMx->CR1 &= (uint16_t)~TIM_CR1_UDIS;
  }
}


void TIM_UpdateRequestConfig(TIM_TypeDef* TIMx, uint16_t TIM_UpdateSource)
{

  assert_param(IS_TIM_ALL_PERIPH(TIMx));
  assert_param(IS_TIM_UPDATE_SOURCE(TIM_UpdateSource));

  if (TIM_UpdateSource != TIM_UpdateSource_Global)
  {

    TIMx->CR1 |= TIM_CR1_URS;
  }
  else
  {

    TIMx->CR1 &= (uint16_t)~TIM_CR1_URS;
  }
}


void TIM_ARRPreloadConfig(TIM_TypeDef* TIMx, FunctionalState NewState)
{

  assert_param(IS_TIM_ALL_PERIPH(TIMx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    TIMx->CR1 |= TIM_CR1_ARPE;
  }
  else
  {

    TIMx->CR1 &= (uint16_t)~TIM_CR1_ARPE;
  }
}


void TIM_SelectOnePulseMode(TIM_TypeDef* TIMx, uint16_t TIM_OPMode)
{

  assert_param(IS_TIM_ALL_PERIPH(TIMx));
  assert_param(IS_TIM_OPM_MODE(TIM_OPMode));


  TIMx->CR1 &= (uint16_t)~TIM_CR1_OPM;


  TIMx->CR1 |= TIM_OPMode;
}


void TIM_SetClockDivision(TIM_TypeDef* TIMx, uint16_t TIM_CKD)
{

  assert_param(IS_TIM_LIST1_PERIPH(TIMx));
  assert_param(IS_TIM_CKD_DIV(TIM_CKD));


  TIMx->CR1 &= (uint16_t)(~TIM_CR1_CKD);


  TIMx->CR1 |= TIM_CKD;
}


void TIM_Cmd(TIM_TypeDef* TIMx, FunctionalState NewState)
{

  assert_param(IS_TIM_ALL_PERIPH(TIMx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    TIMx->CR1 |= TIM_CR1_CEN;
  }
  else
  {

    TIMx->CR1 &= (uint16_t)~TIM_CR1_CEN;
  }
}


void TIM_OC1Init(TIM_TypeDef* TIMx, TIM_OCInitTypeDef* TIM_OCInitStruct)
{
  uint16_t tmpccmrx = 0, tmpccer = 0, tmpcr2 = 0;


  assert_param(IS_TIM_LIST1_PERIPH(TIMx));
  assert_param(IS_TIM_OC_MODE(TIM_OCInitStruct->TIM_OCMode));
  assert_param(IS_TIM_OUTPUT_STATE(TIM_OCInitStruct->TIM_OutputState));
  assert_param(IS_TIM_OC_POLARITY(TIM_OCInitStruct->TIM_OCPolarity));


  TIMx->CCER &= (uint16_t)~TIM_CCER_CC1E;


  tmpccer = TIMx->CCER;

  tmpcr2 =  TIMx->CR2;


  tmpccmrx = TIMx->CCMR1;


  tmpccmrx &= (uint16_t)~TIM_CCMR1_OC1M;
  tmpccmrx &= (uint16_t)~TIM_CCMR1_CC1S;

  tmpccmrx |= TIM_OCInitStruct->TIM_OCMode;


  tmpccer &= (uint16_t)~TIM_CCER_CC1P;

  tmpccer |= TIM_OCInitStruct->TIM_OCPolarity;


  tmpccer |= TIM_OCInitStruct->TIM_OutputState;

  if((TIMx == TIM1) || (TIMx == TIM8))
  {
    assert_param(IS_TIM_OUTPUTN_STATE(TIM_OCInitStruct->TIM_OutputNState));
    assert_param(IS_TIM_OCN_POLARITY(TIM_OCInitStruct->TIM_OCNPolarity));
    assert_param(IS_TIM_OCNIDLE_STATE(TIM_OCInitStruct->TIM_OCNIdleState));
    assert_param(IS_TIM_OCIDLE_STATE(TIM_OCInitStruct->TIM_OCIdleState));


    tmpccer &= (uint16_t)~TIM_CCER_CC1NP;

    tmpccer |= TIM_OCInitStruct->TIM_OCNPolarity;

    tmpccer &= (uint16_t)~TIM_CCER_CC1NE;


    tmpccer |= TIM_OCInitStruct->TIM_OutputNState;

    tmpcr2 &= (uint16_t)~TIM_CR2_OIS1;
    tmpcr2 &= (uint16_t)~TIM_CR2_OIS1N;

    tmpcr2 |= TIM_OCInitStruct->TIM_OCIdleState;

    tmpcr2 |= TIM_OCInitStruct->TIM_OCNIdleState;
  }

  TIMx->CR2 = tmpcr2;


  TIMx->CCMR1 = tmpccmrx;


  TIMx->CCR1 = TIM_OCInitStruct->TIM_Pulse;


  TIMx->CCER = tmpccer;
}


void TIM_OC2Init(TIM_TypeDef* TIMx, TIM_OCInitTypeDef* TIM_OCInitStruct)
{
  uint16_t tmpccmrx = 0, tmpccer = 0, tmpcr2 = 0;


  assert_param(IS_TIM_LIST2_PERIPH(TIMx));
  assert_param(IS_TIM_OC_MODE(TIM_OCInitStruct->TIM_OCMode));
  assert_param(IS_TIM_OUTPUT_STATE(TIM_OCInitStruct->TIM_OutputState));
  assert_param(IS_TIM_OC_POLARITY(TIM_OCInitStruct->TIM_OCPolarity));


  TIMx->CCER &= (uint16_t)~TIM_CCER_CC2E;


  tmpccer = TIMx->CCER;

  tmpcr2 =  TIMx->CR2;


  tmpccmrx = TIMx->CCMR1;


  tmpccmrx &= (uint16_t)~TIM_CCMR1_OC2M;
  tmpccmrx &= (uint16_t)~TIM_CCMR1_CC2S;


  tmpccmrx |= (uint16_t)(TIM_OCInitStruct->TIM_OCMode << 8);


  tmpccer &= (uint16_t)~TIM_CCER_CC2P;

  tmpccer |= (uint16_t)(TIM_OCInitStruct->TIM_OCPolarity << 4);


  tmpccer |= (uint16_t)(TIM_OCInitStruct->TIM_OutputState << 4);

  if((TIMx == TIM1) || (TIMx == TIM8))
  {
    assert_param(IS_TIM_OUTPUTN_STATE(TIM_OCInitStruct->TIM_OutputNState));
    assert_param(IS_TIM_OCN_POLARITY(TIM_OCInitStruct->TIM_OCNPolarity));
    assert_param(IS_TIM_OCNIDLE_STATE(TIM_OCInitStruct->TIM_OCNIdleState));
    assert_param(IS_TIM_OCIDLE_STATE(TIM_OCInitStruct->TIM_OCIdleState));


    tmpccer &= (uint16_t)~TIM_CCER_CC2NP;

    tmpccer |= (uint16_t)(TIM_OCInitStruct->TIM_OCNPolarity << 4);

    tmpccer &= (uint16_t)~TIM_CCER_CC2NE;


    tmpccer |= (uint16_t)(TIM_OCInitStruct->TIM_OutputNState << 4);

    tmpcr2 &= (uint16_t)~TIM_CR2_OIS2;
    tmpcr2 &= (uint16_t)~TIM_CR2_OIS2N;

    tmpcr2 |= (uint16_t)(TIM_OCInitStruct->TIM_OCIdleState << 2);

    tmpcr2 |= (uint16_t)(TIM_OCInitStruct->TIM_OCNIdleState << 2);
  }

  TIMx->CR2 = tmpcr2;


  TIMx->CCMR1 = tmpccmrx;


  TIMx->CCR2 = TIM_OCInitStruct->TIM_Pulse;


  TIMx->CCER = tmpccer;
}


void TIM_OC3Init(TIM_TypeDef* TIMx, TIM_OCInitTypeDef* TIM_OCInitStruct)
{
  uint16_t tmpccmrx = 0, tmpccer = 0, tmpcr2 = 0;


  assert_param(IS_TIM_LIST3_PERIPH(TIMx));
  assert_param(IS_TIM_OC_MODE(TIM_OCInitStruct->TIM_OCMode));
  assert_param(IS_TIM_OUTPUT_STATE(TIM_OCInitStruct->TIM_OutputState));
  assert_param(IS_TIM_OC_POLARITY(TIM_OCInitStruct->TIM_OCPolarity));


  TIMx->CCER &= (uint16_t)~TIM_CCER_CC3E;


  tmpccer = TIMx->CCER;

  tmpcr2 =  TIMx->CR2;


  tmpccmrx = TIMx->CCMR2;


  tmpccmrx &= (uint16_t)~TIM_CCMR2_OC3M;
  tmpccmrx &= (uint16_t)~TIM_CCMR2_CC3S;

  tmpccmrx |= TIM_OCInitStruct->TIM_OCMode;


  tmpccer &= (uint16_t)~TIM_CCER_CC3P;

  tmpccer |= (uint16_t)(TIM_OCInitStruct->TIM_OCPolarity << 8);


  tmpccer |= (uint16_t)(TIM_OCInitStruct->TIM_OutputState << 8);

  if((TIMx == TIM1) || (TIMx == TIM8))
  {
    assert_param(IS_TIM_OUTPUTN_STATE(TIM_OCInitStruct->TIM_OutputNState));
    assert_param(IS_TIM_OCN_POLARITY(TIM_OCInitStruct->TIM_OCNPolarity));
    assert_param(IS_TIM_OCNIDLE_STATE(TIM_OCInitStruct->TIM_OCNIdleState));
    assert_param(IS_TIM_OCIDLE_STATE(TIM_OCInitStruct->TIM_OCIdleState));


    tmpccer &= (uint16_t)~TIM_CCER_CC3NP;

    tmpccer |= (uint16_t)(TIM_OCInitStruct->TIM_OCNPolarity << 8);

    tmpccer &= (uint16_t)~TIM_CCER_CC3NE;


    tmpccer |= (uint16_t)(TIM_OCInitStruct->TIM_OutputNState << 8);

    tmpcr2 &= (uint16_t)~TIM_CR2_OIS3;
    tmpcr2 &= (uint16_t)~TIM_CR2_OIS3N;

    tmpcr2 |= (uint16_t)(TIM_OCInitStruct->TIM_OCIdleState << 4);

    tmpcr2 |= (uint16_t)(TIM_OCInitStruct->TIM_OCNIdleState << 4);
  }

  TIMx->CR2 = tmpcr2;


  TIMx->CCMR2 = tmpccmrx;


  TIMx->CCR3 = TIM_OCInitStruct->TIM_Pulse;


  TIMx->CCER = tmpccer;
}


void TIM_OC4Init(TIM_TypeDef* TIMx, TIM_OCInitTypeDef* TIM_OCInitStruct)
{
  uint16_t tmpccmrx = 0, tmpccer = 0, tmpcr2 = 0;


  assert_param(IS_TIM_LIST3_PERIPH(TIMx));
  assert_param(IS_TIM_OC_MODE(TIM_OCInitStruct->TIM_OCMode));
  assert_param(IS_TIM_OUTPUT_STATE(TIM_OCInitStruct->TIM_OutputState));
  assert_param(IS_TIM_OC_POLARITY(TIM_OCInitStruct->TIM_OCPolarity));


  TIMx->CCER &= (uint16_t)~TIM_CCER_CC4E;


  tmpccer = TIMx->CCER;

  tmpcr2 =  TIMx->CR2;


  tmpccmrx = TIMx->CCMR2;


  tmpccmrx &= (uint16_t)~TIM_CCMR2_OC4M;
  tmpccmrx &= (uint16_t)~TIM_CCMR2_CC4S;


  tmpccmrx |= (uint16_t)(TIM_OCInitStruct->TIM_OCMode << 8);


  tmpccer &= (uint16_t)~TIM_CCER_CC4P;

  tmpccer |= (uint16_t)(TIM_OCInitStruct->TIM_OCPolarity << 12);


  tmpccer |= (uint16_t)(TIM_OCInitStruct->TIM_OutputState << 12);

  if((TIMx == TIM1) || (TIMx == TIM8))
  {
    assert_param(IS_TIM_OCIDLE_STATE(TIM_OCInitStruct->TIM_OCIdleState));

    tmpcr2 &=(uint16_t) ~TIM_CR2_OIS4;

    tmpcr2 |= (uint16_t)(TIM_OCInitStruct->TIM_OCIdleState << 6);
  }

  TIMx->CR2 = tmpcr2;


  TIMx->CCMR2 = tmpccmrx;


  TIMx->CCR4 = TIM_OCInitStruct->TIM_Pulse;


  TIMx->CCER = tmpccer;
}


void TIM_OCStructInit(TIM_OCInitTypeDef* TIM_OCInitStruct)
{

  TIM_OCInitStruct->TIM_OCMode = TIM_OCMode_Timing;
  TIM_OCInitStruct->TIM_OutputState = TIM_OutputState_Disable;
  TIM_OCInitStruct->TIM_OutputNState = TIM_OutputNState_Disable;
  TIM_OCInitStruct->TIM_Pulse = 0x00000000;
  TIM_OCInitStruct->TIM_OCPolarity = TIM_OCPolarity_High;
  TIM_OCInitStruct->TIM_OCNPolarity = TIM_OCPolarity_High;
  TIM_OCInitStruct->TIM_OCIdleState = TIM_OCIdleState_Reset;
  TIM_OCInitStruct->TIM_OCNIdleState = TIM_OCNIdleState_Reset;
}


void TIM_SelectOCxM(TIM_TypeDef* TIMx, uint16_t TIM_Channel, uint16_t TIM_OCMode)
{
  uint32_t tmp = 0;
  uint16_t tmp1 = 0;


  assert_param(IS_TIM_LIST1_PERIPH(TIMx));
  assert_param(IS_TIM_CHANNEL(TIM_Channel));
  assert_param(IS_TIM_OCM(TIM_OCMode));

  tmp = (uint32_t) TIMx;
  tmp += CCMR_OFFSET;

  tmp1 = CCER_CCE_SET << (uint16_t)TIM_Channel;


  TIMx->CCER &= (uint16_t) ~tmp1;

  if((TIM_Channel == TIM_Channel_1) ||(TIM_Channel == TIM_Channel_3))
  {
    tmp += (TIM_Channel>>1);


    *(__IO uint32_t *) tmp &= CCMR_OC13M_MASK;


    *(__IO uint32_t *) tmp |= TIM_OCMode;
  }
  else
  {
    tmp += (uint16_t)(TIM_Channel - (uint16_t)4)>> (uint16_t)1;


    *(__IO uint32_t *) tmp &= CCMR_OC24M_MASK;


    *(__IO uint32_t *) tmp |= (uint16_t)(TIM_OCMode << 8);
  }
}


void TIM_SetCompare1(TIM_TypeDef* TIMx, uint32_t Compare1)
{

  assert_param(IS_TIM_LIST1_PERIPH(TIMx));


  TIMx->CCR1 = Compare1;
}


void TIM_SetCompare2(TIM_TypeDef* TIMx, uint32_t Compare2)
{

  assert_param(IS_TIM_LIST2_PERIPH(TIMx));


  TIMx->CCR2 = Compare2;
}


void TIM_SetCompare3(TIM_TypeDef* TIMx, uint32_t Compare3)
{

  assert_param(IS_TIM_LIST3_PERIPH(TIMx));


  TIMx->CCR3 = Compare3;
}


void TIM_SetCompare4(TIM_TypeDef* TIMx, uint32_t Compare4)
{

  assert_param(IS_TIM_LIST3_PERIPH(TIMx));


  TIMx->CCR4 = Compare4;
}


void TIM_ForcedOC1Config(TIM_TypeDef* TIMx, uint16_t TIM_ForcedAction)
{
  uint16_t tmpccmr1 = 0;


  assert_param(IS_TIM_LIST1_PERIPH(TIMx));
  assert_param(IS_TIM_FORCED_ACTION(TIM_ForcedAction));
  tmpccmr1 = TIMx->CCMR1;


  tmpccmr1 &= (uint16_t)~TIM_CCMR1_OC1M;


  tmpccmr1 |= TIM_ForcedAction;


  TIMx->CCMR1 = tmpccmr1;
}


void TIM_ForcedOC2Config(TIM_TypeDef* TIMx, uint16_t TIM_ForcedAction)
{
  uint16_t tmpccmr1 = 0;


  assert_param(IS_TIM_LIST2_PERIPH(TIMx));
  assert_param(IS_TIM_FORCED_ACTION(TIM_ForcedAction));
  tmpccmr1 = TIMx->CCMR1;


  tmpccmr1 &= (uint16_t)~TIM_CCMR1_OC2M;


  tmpccmr1 |= (uint16_t)(TIM_ForcedAction << 8);


  TIMx->CCMR1 = tmpccmr1;
}


void TIM_ForcedOC3Config(TIM_TypeDef* TIMx, uint16_t TIM_ForcedAction)
{
  uint16_t tmpccmr2 = 0;


  assert_param(IS_TIM_LIST3_PERIPH(TIMx));
  assert_param(IS_TIM_FORCED_ACTION(TIM_ForcedAction));

  tmpccmr2 = TIMx->CCMR2;


  tmpccmr2 &= (uint16_t)~TIM_CCMR2_OC3M;


  tmpccmr2 |= TIM_ForcedAction;


  TIMx->CCMR2 = tmpccmr2;
}


void TIM_ForcedOC4Config(TIM_TypeDef* TIMx, uint16_t TIM_ForcedAction)
{
  uint16_t tmpccmr2 = 0;


  assert_param(IS_TIM_LIST3_PERIPH(TIMx));
  assert_param(IS_TIM_FORCED_ACTION(TIM_ForcedAction));
  tmpccmr2 = TIMx->CCMR2;


  tmpccmr2 &= (uint16_t)~TIM_CCMR2_OC4M;


  tmpccmr2 |= (uint16_t)(TIM_ForcedAction << 8);


  TIMx->CCMR2 = tmpccmr2;
}


void TIM_OC1PreloadConfig(TIM_TypeDef* TIMx, uint16_t TIM_OCPreload)
{
  uint16_t tmpccmr1 = 0;


  assert_param(IS_TIM_LIST1_PERIPH(TIMx));
  assert_param(IS_TIM_OCPRELOAD_STATE(TIM_OCPreload));

  tmpccmr1 = TIMx->CCMR1;


  tmpccmr1 &= (uint16_t)(~TIM_CCMR1_OC1PE);


  tmpccmr1 |= TIM_OCPreload;


  TIMx->CCMR1 = tmpccmr1;
}


void TIM_OC2PreloadConfig(TIM_TypeDef* TIMx, uint16_t TIM_OCPreload)
{
  uint16_t tmpccmr1 = 0;


  assert_param(IS_TIM_LIST2_PERIPH(TIMx));
  assert_param(IS_TIM_OCPRELOAD_STATE(TIM_OCPreload));

  tmpccmr1 = TIMx->CCMR1;


  tmpccmr1 &= (uint16_t)(~TIM_CCMR1_OC2PE);


  tmpccmr1 |= (uint16_t)(TIM_OCPreload << 8);


  TIMx->CCMR1 = tmpccmr1;
}


void TIM_OC3PreloadConfig(TIM_TypeDef* TIMx, uint16_t TIM_OCPreload)
{
  uint16_t tmpccmr2 = 0;


  assert_param(IS_TIM_LIST3_PERIPH(TIMx));
  assert_param(IS_TIM_OCPRELOAD_STATE(TIM_OCPreload));

  tmpccmr2 = TIMx->CCMR2;


  tmpccmr2 &= (uint16_t)(~TIM_CCMR2_OC3PE);


  tmpccmr2 |= TIM_OCPreload;


  TIMx->CCMR2 = tmpccmr2;
}


void TIM_OC4PreloadConfig(TIM_TypeDef* TIMx, uint16_t TIM_OCPreload)
{
  uint16_t tmpccmr2 = 0;


  assert_param(IS_TIM_LIST3_PERIPH(TIMx));
  assert_param(IS_TIM_OCPRELOAD_STATE(TIM_OCPreload));

  tmpccmr2 = TIMx->CCMR2;


  tmpccmr2 &= (uint16_t)(~TIM_CCMR2_OC4PE);


  tmpccmr2 |= (uint16_t)(TIM_OCPreload << 8);


  TIMx->CCMR2 = tmpccmr2;
}


void TIM_OC1FastConfig(TIM_TypeDef* TIMx, uint16_t TIM_OCFast)
{
  uint16_t tmpccmr1 = 0;


  assert_param(IS_TIM_LIST1_PERIPH(TIMx));
  assert_param(IS_TIM_OCFAST_STATE(TIM_OCFast));


  tmpccmr1 = TIMx->CCMR1;


  tmpccmr1 &= (uint16_t)~TIM_CCMR1_OC1FE;


  tmpccmr1 |= TIM_OCFast;


  TIMx->CCMR1 = tmpccmr1;
}


void TIM_OC2FastConfig(TIM_TypeDef* TIMx, uint16_t TIM_OCFast)
{
  uint16_t tmpccmr1 = 0;


  assert_param(IS_TIM_LIST2_PERIPH(TIMx));
  assert_param(IS_TIM_OCFAST_STATE(TIM_OCFast));


  tmpccmr1 = TIMx->CCMR1;


  tmpccmr1 &= (uint16_t)(~TIM_CCMR1_OC2FE);


  tmpccmr1 |= (uint16_t)(TIM_OCFast << 8);


  TIMx->CCMR1 = tmpccmr1;
}


void TIM_OC3FastConfig(TIM_TypeDef* TIMx, uint16_t TIM_OCFast)
{
  uint16_t tmpccmr2 = 0;


  assert_param(IS_TIM_LIST3_PERIPH(TIMx));
  assert_param(IS_TIM_OCFAST_STATE(TIM_OCFast));


  tmpccmr2 = TIMx->CCMR2;


  tmpccmr2 &= (uint16_t)~TIM_CCMR2_OC3FE;


  tmpccmr2 |= TIM_OCFast;


  TIMx->CCMR2 = tmpccmr2;
}


void TIM_OC4FastConfig(TIM_TypeDef* TIMx, uint16_t TIM_OCFast)
{
  uint16_t tmpccmr2 = 0;


  assert_param(IS_TIM_LIST3_PERIPH(TIMx));
  assert_param(IS_TIM_OCFAST_STATE(TIM_OCFast));


  tmpccmr2 = TIMx->CCMR2;


  tmpccmr2 &= (uint16_t)(~TIM_CCMR2_OC4FE);


  tmpccmr2 |= (uint16_t)(TIM_OCFast << 8);


  TIMx->CCMR2 = tmpccmr2;
}


void TIM_ClearOC1Ref(TIM_TypeDef* TIMx, uint16_t TIM_OCClear)
{
  uint16_t tmpccmr1 = 0;


  assert_param(IS_TIM_LIST1_PERIPH(TIMx));
  assert_param(IS_TIM_OCCLEAR_STATE(TIM_OCClear));

  tmpccmr1 = TIMx->CCMR1;


  tmpccmr1 &= (uint16_t)~TIM_CCMR1_OC1CE;


  tmpccmr1 |= TIM_OCClear;


  TIMx->CCMR1 = tmpccmr1;
}


void TIM_ClearOC2Ref(TIM_TypeDef* TIMx, uint16_t TIM_OCClear)
{
  uint16_t tmpccmr1 = 0;


  assert_param(IS_TIM_LIST2_PERIPH(TIMx));
  assert_param(IS_TIM_OCCLEAR_STATE(TIM_OCClear));

  tmpccmr1 = TIMx->CCMR1;


  tmpccmr1 &= (uint16_t)~TIM_CCMR1_OC2CE;


  tmpccmr1 |= (uint16_t)(TIM_OCClear << 8);


  TIMx->CCMR1 = tmpccmr1;
}


void TIM_ClearOC3Ref(TIM_TypeDef* TIMx, uint16_t TIM_OCClear)
{
  uint16_t tmpccmr2 = 0;


  assert_param(IS_TIM_LIST3_PERIPH(TIMx));
  assert_param(IS_TIM_OCCLEAR_STATE(TIM_OCClear));

  tmpccmr2 = TIMx->CCMR2;


  tmpccmr2 &= (uint16_t)~TIM_CCMR2_OC3CE;


  tmpccmr2 |= TIM_OCClear;


  TIMx->CCMR2 = tmpccmr2;
}


void TIM_ClearOC4Ref(TIM_TypeDef* TIMx, uint16_t TIM_OCClear)
{
  uint16_t tmpccmr2 = 0;


  assert_param(IS_TIM_LIST3_PERIPH(TIMx));
  assert_param(IS_TIM_OCCLEAR_STATE(TIM_OCClear));

  tmpccmr2 = TIMx->CCMR2;


  tmpccmr2 &= (uint16_t)~TIM_CCMR2_OC4CE;


  tmpccmr2 |= (uint16_t)(TIM_OCClear << 8);


  TIMx->CCMR2 = tmpccmr2;
}


void TIM_OC1PolarityConfig(TIM_TypeDef* TIMx, uint16_t TIM_OCPolarity)
{
  uint16_t tmpccer = 0;


  assert_param(IS_TIM_LIST1_PERIPH(TIMx));
  assert_param(IS_TIM_OC_POLARITY(TIM_OCPolarity));

  tmpccer = TIMx->CCER;


  tmpccer &= (uint16_t)(~TIM_CCER_CC1P);
  tmpccer |= TIM_OCPolarity;


  TIMx->CCER = tmpccer;
}


void TIM_OC1NPolarityConfig(TIM_TypeDef* TIMx, uint16_t TIM_OCNPolarity)
{
  uint16_t tmpccer = 0;

  assert_param(IS_TIM_LIST4_PERIPH(TIMx));
  assert_param(IS_TIM_OCN_POLARITY(TIM_OCNPolarity));

  tmpccer = TIMx->CCER;


  tmpccer &= (uint16_t)~TIM_CCER_CC1NP;
  tmpccer |= TIM_OCNPolarity;


  TIMx->CCER = tmpccer;
}


void TIM_OC2PolarityConfig(TIM_TypeDef* TIMx, uint16_t TIM_OCPolarity)
{
  uint16_t tmpccer = 0;


  assert_param(IS_TIM_LIST2_PERIPH(TIMx));
  assert_param(IS_TIM_OC_POLARITY(TIM_OCPolarity));

  tmpccer = TIMx->CCER;


  tmpccer &= (uint16_t)(~TIM_CCER_CC2P);
  tmpccer |= (uint16_t)(TIM_OCPolarity << 4);


  TIMx->CCER = tmpccer;
}


void TIM_OC2NPolarityConfig(TIM_TypeDef* TIMx, uint16_t TIM_OCNPolarity)
{
  uint16_t tmpccer = 0;


  assert_param(IS_TIM_LIST4_PERIPH(TIMx));
  assert_param(IS_TIM_OCN_POLARITY(TIM_OCNPolarity));

  tmpccer = TIMx->CCER;


  tmpccer &= (uint16_t)~TIM_CCER_CC2NP;
  tmpccer |= (uint16_t)(TIM_OCNPolarity << 4);


  TIMx->CCER = tmpccer;
}


void TIM_OC3PolarityConfig(TIM_TypeDef* TIMx, uint16_t TIM_OCPolarity)
{
  uint16_t tmpccer = 0;


  assert_param(IS_TIM_LIST3_PERIPH(TIMx));
  assert_param(IS_TIM_OC_POLARITY(TIM_OCPolarity));

  tmpccer = TIMx->CCER;


  tmpccer &= (uint16_t)~TIM_CCER_CC3P;
  tmpccer |= (uint16_t)(TIM_OCPolarity << 8);


  TIMx->CCER = tmpccer;
}


void TIM_OC3NPolarityConfig(TIM_TypeDef* TIMx, uint16_t TIM_OCNPolarity)
{
  uint16_t tmpccer = 0;


  assert_param(IS_TIM_LIST4_PERIPH(TIMx));
  assert_param(IS_TIM_OCN_POLARITY(TIM_OCNPolarity));

  tmpccer = TIMx->CCER;


  tmpccer &= (uint16_t)~TIM_CCER_CC3NP;
  tmpccer |= (uint16_t)(TIM_OCNPolarity << 8);


  TIMx->CCER = tmpccer;
}


void TIM_OC4PolarityConfig(TIM_TypeDef* TIMx, uint16_t TIM_OCPolarity)
{
  uint16_t tmpccer = 0;


  assert_param(IS_TIM_LIST3_PERIPH(TIMx));
  assert_param(IS_TIM_OC_POLARITY(TIM_OCPolarity));

  tmpccer = TIMx->CCER;


  tmpccer &= (uint16_t)~TIM_CCER_CC4P;
  tmpccer |= (uint16_t)(TIM_OCPolarity << 12);


  TIMx->CCER = tmpccer;
}


void TIM_CCxCmd(TIM_TypeDef* TIMx, uint16_t TIM_Channel, uint16_t TIM_CCx)
{
  uint16_t tmp = 0;


  assert_param(IS_TIM_LIST1_PERIPH(TIMx));
  assert_param(IS_TIM_CHANNEL(TIM_Channel));
  assert_param(IS_TIM_CCX(TIM_CCx));

  tmp = CCER_CCE_SET << TIM_Channel;


  TIMx->CCER &= (uint16_t)~ tmp;


  TIMx->CCER |=  (uint16_t)(TIM_CCx << TIM_Channel);
}


void TIM_CCxNCmd(TIM_TypeDef* TIMx, uint16_t TIM_Channel, uint16_t TIM_CCxN)
{
  uint16_t tmp = 0;


  assert_param(IS_TIM_LIST4_PERIPH(TIMx));
  assert_param(IS_TIM_COMPLEMENTARY_CHANNEL(TIM_Channel));
  assert_param(IS_TIM_CCXN(TIM_CCxN));

  tmp = CCER_CCNE_SET << TIM_Channel;


  TIMx->CCER &= (uint16_t) ~tmp;


  TIMx->CCER |=  (uint16_t)(TIM_CCxN << TIM_Channel);
}


void TIM_ICInit(TIM_TypeDef* TIMx, TIM_ICInitTypeDef* TIM_ICInitStruct)
{

  assert_param(IS_TIM_LIST1_PERIPH(TIMx));
  assert_param(IS_TIM_IC_POLARITY(TIM_ICInitStruct->TIM_ICPolarity));
  assert_param(IS_TIM_IC_SELECTION(TIM_ICInitStruct->TIM_ICSelection));
  assert_param(IS_TIM_IC_PRESCALER(TIM_ICInitStruct->TIM_ICPrescaler));
  assert_param(IS_TIM_IC_FILTER(TIM_ICInitStruct->TIM_ICFilter));

  if (TIM_ICInitStruct->TIM_Channel == TIM_Channel_1)
  {

    TI1_Config(TIMx, TIM_ICInitStruct->TIM_ICPolarity,
               TIM_ICInitStruct->TIM_ICSelection,
               TIM_ICInitStruct->TIM_ICFilter);

    TIM_SetIC1Prescaler(TIMx, TIM_ICInitStruct->TIM_ICPrescaler);
  }
  else if (TIM_ICInitStruct->TIM_Channel == TIM_Channel_2)
  {

    assert_param(IS_TIM_LIST2_PERIPH(TIMx));
    TI2_Config(TIMx, TIM_ICInitStruct->TIM_ICPolarity,
               TIM_ICInitStruct->TIM_ICSelection,
               TIM_ICInitStruct->TIM_ICFilter);

    TIM_SetIC2Prescaler(TIMx, TIM_ICInitStruct->TIM_ICPrescaler);
  }
  else if (TIM_ICInitStruct->TIM_Channel == TIM_Channel_3)
  {

    assert_param(IS_TIM_LIST3_PERIPH(TIMx));
    TI3_Config(TIMx,  TIM_ICInitStruct->TIM_ICPolarity,
               TIM_ICInitStruct->TIM_ICSelection,
               TIM_ICInitStruct->TIM_ICFilter);

    TIM_SetIC3Prescaler(TIMx, TIM_ICInitStruct->TIM_ICPrescaler);
  }
  else
  {

    assert_param(IS_TIM_LIST3_PERIPH(TIMx));
    TI4_Config(TIMx, TIM_ICInitStruct->TIM_ICPolarity,
               TIM_ICInitStruct->TIM_ICSelection,
               TIM_ICInitStruct->TIM_ICFilter);

    TIM_SetIC4Prescaler(TIMx, TIM_ICInitStruct->TIM_ICPrescaler);
  }
}


void TIM_ICStructInit(TIM_ICInitTypeDef* TIM_ICInitStruct)
{

  TIM_ICInitStruct->TIM_Channel = TIM_Channel_1;
  TIM_ICInitStruct->TIM_ICPolarity = TIM_ICPolarity_Rising;
  TIM_ICInitStruct->TIM_ICSelection = TIM_ICSelection_DirectTI;
  TIM_ICInitStruct->TIM_ICPrescaler = TIM_ICPSC_DIV1;
  TIM_ICInitStruct->TIM_ICFilter = 0x00;
}


void TIM_PWMIConfig(TIM_TypeDef* TIMx, TIM_ICInitTypeDef* TIM_ICInitStruct)
{
  uint16_t icoppositepolarity = TIM_ICPolarity_Rising;
  uint16_t icoppositeselection = TIM_ICSelection_DirectTI;


  assert_param(IS_TIM_LIST2_PERIPH(TIMx));


  if (TIM_ICInitStruct->TIM_ICPolarity == TIM_ICPolarity_Rising)
  {
    icoppositepolarity = TIM_ICPolarity_Falling;
  }
  else
  {
    icoppositepolarity = TIM_ICPolarity_Rising;
  }

  if (TIM_ICInitStruct->TIM_ICSelection == TIM_ICSelection_DirectTI)
  {
    icoppositeselection = TIM_ICSelection_IndirectTI;
  }
  else
  {
    icoppositeselection = TIM_ICSelection_DirectTI;
  }
  if (TIM_ICInitStruct->TIM_Channel == TIM_Channel_1)
  {

    TI1_Config(TIMx, TIM_ICInitStruct->TIM_ICPolarity, TIM_ICInitStruct->TIM_ICSelection,
               TIM_ICInitStruct->TIM_ICFilter);

    TIM_SetIC1Prescaler(TIMx, TIM_ICInitStruct->TIM_ICPrescaler);

    TI2_Config(TIMx, icoppositepolarity, icoppositeselection, TIM_ICInitStruct->TIM_ICFilter);

    TIM_SetIC2Prescaler(TIMx, TIM_ICInitStruct->TIM_ICPrescaler);
  }
  else
  {

    TI2_Config(TIMx, TIM_ICInitStruct->TIM_ICPolarity, TIM_ICInitStruct->TIM_ICSelection,
               TIM_ICInitStruct->TIM_ICFilter);

    TIM_SetIC2Prescaler(TIMx, TIM_ICInitStruct->TIM_ICPrescaler);

    TI1_Config(TIMx, icoppositepolarity, icoppositeselection, TIM_ICInitStruct->TIM_ICFilter);

    TIM_SetIC1Prescaler(TIMx, TIM_ICInitStruct->TIM_ICPrescaler);
  }
}


uint32_t TIM_GetCapture1(TIM_TypeDef* TIMx)
{

  assert_param(IS_TIM_LIST1_PERIPH(TIMx));


  return TIMx->CCR1;
}


uint32_t TIM_GetCapture2(TIM_TypeDef* TIMx)
{

  assert_param(IS_TIM_LIST2_PERIPH(TIMx));


  return TIMx->CCR2;
}


uint32_t TIM_GetCapture3(TIM_TypeDef* TIMx)
{

  assert_param(IS_TIM_LIST3_PERIPH(TIMx));


  return TIMx->CCR3;
}


uint32_t TIM_GetCapture4(TIM_TypeDef* TIMx)
{

  assert_param(IS_TIM_LIST3_PERIPH(TIMx));


  return TIMx->CCR4;
}


void TIM_SetIC1Prescaler(TIM_TypeDef* TIMx, uint16_t TIM_ICPSC)
{

  assert_param(IS_TIM_LIST1_PERIPH(TIMx));
  assert_param(IS_TIM_IC_PRESCALER(TIM_ICPSC));


  TIMx->CCMR1 &= (uint16_t)~TIM_CCMR1_IC1PSC;


  TIMx->CCMR1 |= TIM_ICPSC;
}


void TIM_SetIC2Prescaler(TIM_TypeDef* TIMx, uint16_t TIM_ICPSC)
{

  assert_param(IS_TIM_LIST2_PERIPH(TIMx));
  assert_param(IS_TIM_IC_PRESCALER(TIM_ICPSC));


  TIMx->CCMR1 &= (uint16_t)~TIM_CCMR1_IC2PSC;


  TIMx->CCMR1 |= (uint16_t)(TIM_ICPSC << 8);
}


void TIM_SetIC3Prescaler(TIM_TypeDef* TIMx, uint16_t TIM_ICPSC)
{

  assert_param(IS_TIM_LIST3_PERIPH(TIMx));
  assert_param(IS_TIM_IC_PRESCALER(TIM_ICPSC));


  TIMx->CCMR2 &= (uint16_t)~TIM_CCMR2_IC3PSC;


  TIMx->CCMR2 |= TIM_ICPSC;
}


void TIM_SetIC4Prescaler(TIM_TypeDef* TIMx, uint16_t TIM_ICPSC)
{

  assert_param(IS_TIM_LIST3_PERIPH(TIMx));
  assert_param(IS_TIM_IC_PRESCALER(TIM_ICPSC));


  TIMx->CCMR2 &= (uint16_t)~TIM_CCMR2_IC4PSC;


  TIMx->CCMR2 |= (uint16_t)(TIM_ICPSC << 8);
}


void TIM_BDTRConfig(TIM_TypeDef* TIMx, TIM_BDTRInitTypeDef *TIM_BDTRInitStruct)
{

  assert_param(IS_TIM_LIST4_PERIPH(TIMx));
  assert_param(IS_TIM_OSSR_STATE(TIM_BDTRInitStruct->TIM_OSSRState));
  assert_param(IS_TIM_OSSI_STATE(TIM_BDTRInitStruct->TIM_OSSIState));
  assert_param(IS_TIM_LOCK_LEVEL(TIM_BDTRInitStruct->TIM_LOCKLevel));
  assert_param(IS_TIM_BREAK_STATE(TIM_BDTRInitStruct->TIM_Break));
  assert_param(IS_TIM_BREAK_POLARITY(TIM_BDTRInitStruct->TIM_BreakPolarity));
  assert_param(IS_TIM_AUTOMATIC_OUTPUT_STATE(TIM_BDTRInitStruct->TIM_AutomaticOutput));


  TIMx->BDTR = (uint32_t)TIM_BDTRInitStruct->TIM_OSSRState | TIM_BDTRInitStruct->TIM_OSSIState |
             TIM_BDTRInitStruct->TIM_LOCKLevel | TIM_BDTRInitStruct->TIM_DeadTime |
             TIM_BDTRInitStruct->TIM_Break | TIM_BDTRInitStruct->TIM_BreakPolarity |
             TIM_BDTRInitStruct->TIM_AutomaticOutput;
}


void TIM_BDTRStructInit(TIM_BDTRInitTypeDef* TIM_BDTRInitStruct)
{

  TIM_BDTRInitStruct->TIM_OSSRState = TIM_OSSRState_Disable;
  TIM_BDTRInitStruct->TIM_OSSIState = TIM_OSSIState_Disable;
  TIM_BDTRInitStruct->TIM_LOCKLevel = TIM_LOCKLevel_OFF;
  TIM_BDTRInitStruct->TIM_DeadTime = 0x00;
  TIM_BDTRInitStruct->TIM_Break = TIM_Break_Disable;
  TIM_BDTRInitStruct->TIM_BreakPolarity = TIM_BreakPolarity_Low;
  TIM_BDTRInitStruct->TIM_AutomaticOutput = TIM_AutomaticOutput_Disable;
}


void TIM_CtrlPWMOutputs(TIM_TypeDef* TIMx, FunctionalState NewState)
{

  assert_param(IS_TIM_LIST4_PERIPH(TIMx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    TIMx->BDTR |= TIM_BDTR_MOE;
  }
  else
  {

    TIMx->BDTR &= (uint16_t)~TIM_BDTR_MOE;
  }
}


void TIM_SelectCOM(TIM_TypeDef* TIMx, FunctionalState NewState)
{

  assert_param(IS_TIM_LIST4_PERIPH(TIMx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    TIMx->CR2 |= TIM_CR2_CCUS;
  }
  else
  {

    TIMx->CR2 &= (uint16_t)~TIM_CR2_CCUS;
  }
}


void TIM_CCPreloadControl(TIM_TypeDef* TIMx, FunctionalState NewState)
{

  assert_param(IS_TIM_LIST4_PERIPH(TIMx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {

    TIMx->CR2 |= TIM_CR2_CCPC;
  }
  else
  {

    TIMx->CR2 &= (uint16_t)~TIM_CR2_CCPC;
  }
}


void TIM_ITConfig(TIM_TypeDef* TIMx, uint16_t TIM_IT, FunctionalState NewState)
{

  assert_param(IS_TIM_ALL_PERIPH(TIMx));
  assert_param(IS_TIM_IT(TIM_IT));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    TIMx->DIER |= TIM_IT;
  }
  else
  {

    TIMx->DIER &= (uint16_t)~TIM_IT;
  }
}


void TIM_GenerateEvent(TIM_TypeDef* TIMx, uint16_t TIM_EventSource)
{

  assert_param(IS_TIM_ALL_PERIPH(TIMx));
  assert_param(IS_TIM_EVENT_SOURCE(TIM_EventSource));


  TIMx->EGR = TIM_EventSource;
}


FlagStatus TIM_GetFlagStatus(TIM_TypeDef* TIMx, uint16_t TIM_FLAG)
{
  ITStatus bitstatus = RESET;

  assert_param(IS_TIM_ALL_PERIPH(TIMx));
  assert_param(IS_TIM_GET_FLAG(TIM_FLAG));


  if ((TIMx->SR & TIM_FLAG) != (uint16_t)RESET)
  {
    bitstatus = SET;
  }
  else
  {
    bitstatus = RESET;
  }
  return bitstatus;
}


void TIM_ClearFlag(TIM_TypeDef* TIMx, uint16_t TIM_FLAG)
{

  assert_param(IS_TIM_ALL_PERIPH(TIMx));


  TIMx->SR = (uint16_t)~TIM_FLAG;
}


ITStatus TIM_GetITStatus(TIM_TypeDef* TIMx, uint16_t TIM_IT)
{
  ITStatus bitstatus = RESET;
  uint16_t itstatus = 0x0, itenable = 0x0;

  assert_param(IS_TIM_ALL_PERIPH(TIMx));
  assert_param(IS_TIM_GET_IT(TIM_IT));

  itstatus = TIMx->SR & TIM_IT;

  itenable = TIMx->DIER & TIM_IT;
  if ((itstatus != (uint16_t)RESET) && (itenable != (uint16_t)RESET))
  {
    bitstatus = SET;
  }
  else
  {
    bitstatus = RESET;
  }
  return bitstatus;
}


void TIM_ClearITPendingBit(TIM_TypeDef* TIMx, uint16_t TIM_IT)
{

  assert_param(IS_TIM_ALL_PERIPH(TIMx));


  TIMx->SR = (uint16_t)~TIM_IT;
}


void TIM_DMAConfig(TIM_TypeDef* TIMx, uint16_t TIM_DMABase, uint16_t TIM_DMABurstLength)
{

  assert_param(IS_TIM_LIST3_PERIPH(TIMx));
  assert_param(IS_TIM_DMA_BASE(TIM_DMABase));
  assert_param(IS_TIM_DMA_LENGTH(TIM_DMABurstLength));


  TIMx->DCR = TIM_DMABase | TIM_DMABurstLength;
}


void TIM_DMACmd(TIM_TypeDef* TIMx, uint16_t TIM_DMASource, FunctionalState NewState)
{

  assert_param(IS_TIM_LIST5_PERIPH(TIMx));
  assert_param(IS_TIM_DMA_SOURCE(TIM_DMASource));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    TIMx->DIER |= TIM_DMASource;
  }
  else
  {

    TIMx->DIER &= (uint16_t)~TIM_DMASource;
  }
}


void TIM_SelectCCDMA(TIM_TypeDef* TIMx, FunctionalState NewState)
{

  assert_param(IS_TIM_LIST3_PERIPH(TIMx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    TIMx->CR2 |= TIM_CR2_CCDS;
  }
  else
  {

    TIMx->CR2 &= (uint16_t)~TIM_CR2_CCDS;
  }
}


void TIM_InternalClockConfig(TIM_TypeDef* TIMx)
{

  assert_param(IS_TIM_LIST2_PERIPH(TIMx));


  TIMx->SMCR &=  (uint16_t)~TIM_SMCR_SMS;
}


void TIM_ITRxExternalClockConfig(TIM_TypeDef* TIMx, uint16_t TIM_InputTriggerSource)
{

  assert_param(IS_TIM_LIST2_PERIPH(TIMx));
  assert_param(IS_TIM_INTERNAL_TRIGGER_SELECTION(TIM_InputTriggerSource));


  TIM_SelectInputTrigger(TIMx, TIM_InputTriggerSource);


  TIMx->SMCR |= TIM_SlaveMode_External1;
}


void TIM_TIxExternalClockConfig(TIM_TypeDef* TIMx, uint16_t TIM_TIxExternalCLKSource,
                                uint16_t TIM_ICPolarity, uint16_t ICFilter)
{

  assert_param(IS_TIM_LIST1_PERIPH(TIMx));
  assert_param(IS_TIM_IC_POLARITY(TIM_ICPolarity));
  assert_param(IS_TIM_IC_FILTER(ICFilter));


  if (TIM_TIxExternalCLKSource == TIM_TIxExternalCLK1Source_TI2)
  {
    TI2_Config(TIMx, TIM_ICPolarity, TIM_ICSelection_DirectTI, ICFilter);
  }
  else
  {
    TI1_Config(TIMx, TIM_ICPolarity, TIM_ICSelection_DirectTI, ICFilter);
  }

  TIM_SelectInputTrigger(TIMx, TIM_TIxExternalCLKSource);

  TIMx->SMCR |= TIM_SlaveMode_External1;
}


void TIM_ETRClockMode1Config(TIM_TypeDef* TIMx, uint16_t TIM_ExtTRGPrescaler,
                            uint16_t TIM_ExtTRGPolarity, uint16_t ExtTRGFilter)
{
  uint16_t tmpsmcr = 0;


  assert_param(IS_TIM_LIST3_PERIPH(TIMx));
  assert_param(IS_TIM_EXT_PRESCALER(TIM_ExtTRGPrescaler));
  assert_param(IS_TIM_EXT_POLARITY(TIM_ExtTRGPolarity));
  assert_param(IS_TIM_EXT_FILTER(ExtTRGFilter));

  TIM_ETRConfig(TIMx, TIM_ExtTRGPrescaler, TIM_ExtTRGPolarity, ExtTRGFilter);


  tmpsmcr = TIMx->SMCR;


  tmpsmcr &= (uint16_t)~TIM_SMCR_SMS;


  tmpsmcr |= TIM_SlaveMode_External1;


  tmpsmcr &= (uint16_t)~TIM_SMCR_TS;
  tmpsmcr |= TIM_TS_ETRF;


  TIMx->SMCR = tmpsmcr;
}


void TIM_ETRClockMode2Config(TIM_TypeDef* TIMx, uint16_t TIM_ExtTRGPrescaler,
                             uint16_t TIM_ExtTRGPolarity, uint16_t ExtTRGFilter)
{

  assert_param(IS_TIM_LIST3_PERIPH(TIMx));
  assert_param(IS_TIM_EXT_PRESCALER(TIM_ExtTRGPrescaler));
  assert_param(IS_TIM_EXT_POLARITY(TIM_ExtTRGPolarity));
  assert_param(IS_TIM_EXT_FILTER(ExtTRGFilter));


  TIM_ETRConfig(TIMx, TIM_ExtTRGPrescaler, TIM_ExtTRGPolarity, ExtTRGFilter);


  TIMx->SMCR |= TIM_SMCR_ECE;
}


void TIM_SelectInputTrigger(TIM_TypeDef* TIMx, uint16_t TIM_InputTriggerSource)
{
  uint16_t tmpsmcr = 0;


  assert_param(IS_TIM_LIST1_PERIPH(TIMx));
  assert_param(IS_TIM_TRIGGER_SELECTION(TIM_InputTriggerSource));


  tmpsmcr = TIMx->SMCR;


  tmpsmcr &= (uint16_t)~TIM_SMCR_TS;


  tmpsmcr |= TIM_InputTriggerSource;


  TIMx->SMCR = tmpsmcr;
}


void TIM_SelectOutputTrigger(TIM_TypeDef* TIMx, uint16_t TIM_TRGOSource)
{

  assert_param(IS_TIM_LIST5_PERIPH(TIMx));
  assert_param(IS_TIM_TRGO_SOURCE(TIM_TRGOSource));


  TIMx->CR2 &= (uint16_t)~TIM_CR2_MMS;

  TIMx->CR2 |=  TIM_TRGOSource;
}


void TIM_SelectSlaveMode(TIM_TypeDef* TIMx, uint16_t TIM_SlaveMode)
{

  assert_param(IS_TIM_LIST2_PERIPH(TIMx));
  assert_param(IS_TIM_SLAVE_MODE(TIM_SlaveMode));


  TIMx->SMCR &= (uint16_t)~TIM_SMCR_SMS;


  TIMx->SMCR |= TIM_SlaveMode;
}


void TIM_SelectMasterSlaveMode(TIM_TypeDef* TIMx, uint16_t TIM_MasterSlaveMode)
{

  assert_param(IS_TIM_LIST2_PERIPH(TIMx));
  assert_param(IS_TIM_MSM_STATE(TIM_MasterSlaveMode));


  TIMx->SMCR &= (uint16_t)~TIM_SMCR_MSM;


  TIMx->SMCR |= TIM_MasterSlaveMode;
}


void TIM_ETRConfig(TIM_TypeDef* TIMx, uint16_t TIM_ExtTRGPrescaler,
                   uint16_t TIM_ExtTRGPolarity, uint16_t ExtTRGFilter)
{
  uint16_t tmpsmcr = 0;


  assert_param(IS_TIM_LIST3_PERIPH(TIMx));
  assert_param(IS_TIM_EXT_PRESCALER(TIM_ExtTRGPrescaler));
  assert_param(IS_TIM_EXT_POLARITY(TIM_ExtTRGPolarity));
  assert_param(IS_TIM_EXT_FILTER(ExtTRGFilter));

  tmpsmcr = TIMx->SMCR;


  tmpsmcr &= SMCR_ETR_MASK;


  tmpsmcr |= (uint16_t)(TIM_ExtTRGPrescaler | (uint16_t)(TIM_ExtTRGPolarity | (uint16_t)(ExtTRGFilter << (uint16_t)8)));


  TIMx->SMCR = tmpsmcr;
}


void TIM_EncoderInterfaceConfig(TIM_TypeDef* TIMx, uint16_t TIM_EncoderMode,
                                uint16_t TIM_IC1Polarity, uint16_t TIM_IC2Polarity)
{
  uint16_t tmpsmcr = 0;
  uint16_t tmpccmr1 = 0;
  uint16_t tmpccer = 0;


  assert_param(IS_TIM_LIST2_PERIPH(TIMx));
  assert_param(IS_TIM_ENCODER_MODE(TIM_EncoderMode));
  assert_param(IS_TIM_IC_POLARITY(TIM_IC1Polarity));
  assert_param(IS_TIM_IC_POLARITY(TIM_IC2Polarity));


  tmpsmcr = TIMx->SMCR;


  tmpccmr1 = TIMx->CCMR1;


  tmpccer = TIMx->CCER;


  tmpsmcr &= (uint16_t)~TIM_SMCR_SMS;
  tmpsmcr |= TIM_EncoderMode;


  tmpccmr1 &= ((uint16_t)~TIM_CCMR1_CC1S) & ((uint16_t)~TIM_CCMR1_CC2S);
  tmpccmr1 |= TIM_CCMR1_CC1S_0 | TIM_CCMR1_CC2S_0;


  tmpccer &= ((uint16_t)~TIM_CCER_CC1P) & ((uint16_t)~TIM_CCER_CC2P);
  tmpccer |= (uint16_t)(TIM_IC1Polarity | (uint16_t)(TIM_IC2Polarity << (uint16_t)4));


  TIMx->SMCR = tmpsmcr;


  TIMx->CCMR1 = tmpccmr1;


  TIMx->CCER = tmpccer;
}


void TIM_SelectHallSensor(TIM_TypeDef* TIMx, FunctionalState NewState)
{

  assert_param(IS_TIM_LIST2_PERIPH(TIMx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    TIMx->CR2 |= TIM_CR2_TI1S;
  }
  else
  {

    TIMx->CR2 &= (uint16_t)~TIM_CR2_TI1S;
  }
}


void TIM_RemapConfig(TIM_TypeDef* TIMx, uint16_t TIM_Remap)
{

  assert_param(IS_TIM_LIST6_PERIPH(TIMx));
  assert_param(IS_TIM_REMAP(TIM_Remap));


  TIMx->OR =  TIM_Remap;
}


static void TI1_Config(TIM_TypeDef* TIMx, uint16_t TIM_ICPolarity, uint16_t TIM_ICSelection,
                       uint16_t TIM_ICFilter)
{
  uint16_t tmpccmr1 = 0, tmpccer = 0;


  TIMx->CCER &= (uint16_t)~TIM_CCER_CC1E;
  tmpccmr1 = TIMx->CCMR1;
  tmpccer = TIMx->CCER;


  tmpccmr1 &= ((uint16_t)~TIM_CCMR1_CC1S) & ((uint16_t)~TIM_CCMR1_IC1F);
  tmpccmr1 |= (uint16_t)(TIM_ICSelection | (uint16_t)(TIM_ICFilter << (uint16_t)4));


  tmpccer &= (uint16_t)~(TIM_CCER_CC1P | TIM_CCER_CC1NP);
  tmpccer |= (uint16_t)(TIM_ICPolarity | (uint16_t)TIM_CCER_CC1E);


  TIMx->CCMR1 = tmpccmr1;
  TIMx->CCER = tmpccer;
}


static void TI2_Config(TIM_TypeDef* TIMx, uint16_t TIM_ICPolarity, uint16_t TIM_ICSelection,
                       uint16_t TIM_ICFilter)
{
  uint16_t tmpccmr1 = 0, tmpccer = 0, tmp = 0;


  TIMx->CCER &= (uint16_t)~TIM_CCER_CC2E;
  tmpccmr1 = TIMx->CCMR1;
  tmpccer = TIMx->CCER;
  tmp = (uint16_t)(TIM_ICPolarity << 4);


  tmpccmr1 &= ((uint16_t)~TIM_CCMR1_CC2S) & ((uint16_t)~TIM_CCMR1_IC2F);
  tmpccmr1 |= (uint16_t)(TIM_ICFilter << 12);
  tmpccmr1 |= (uint16_t)(TIM_ICSelection << 8);


  tmpccer &= (uint16_t)~(TIM_CCER_CC2P | TIM_CCER_CC2NP);
  tmpccer |=  (uint16_t)(tmp | (uint16_t)TIM_CCER_CC2E);


  TIMx->CCMR1 = tmpccmr1 ;
  TIMx->CCER = tmpccer;
}


static void TI3_Config(TIM_TypeDef* TIMx, uint16_t TIM_ICPolarity, uint16_t TIM_ICSelection,
                       uint16_t TIM_ICFilter)
{
  uint16_t tmpccmr2 = 0, tmpccer = 0, tmp = 0;


  TIMx->CCER &= (uint16_t)~TIM_CCER_CC3E;
  tmpccmr2 = TIMx->CCMR2;
  tmpccer = TIMx->CCER;
  tmp = (uint16_t)(TIM_ICPolarity << 8);


  tmpccmr2 &= ((uint16_t)~TIM_CCMR1_CC1S) & ((uint16_t)~TIM_CCMR2_IC3F);
  tmpccmr2 |= (uint16_t)(TIM_ICSelection | (uint16_t)(TIM_ICFilter << (uint16_t)4));


  tmpccer &= (uint16_t)~(TIM_CCER_CC3P | TIM_CCER_CC3NP);
  tmpccer |= (uint16_t)(tmp | (uint16_t)TIM_CCER_CC3E);


  TIMx->CCMR2 = tmpccmr2;
  TIMx->CCER = tmpccer;
}


static void TI4_Config(TIM_TypeDef* TIMx, uint16_t TIM_ICPolarity, uint16_t TIM_ICSelection,
                       uint16_t TIM_ICFilter)
{
  uint16_t tmpccmr2 = 0, tmpccer = 0, tmp = 0;


  TIMx->CCER &= (uint16_t)~TIM_CCER_CC4E;
  tmpccmr2 = TIMx->CCMR2;
  tmpccer = TIMx->CCER;
  tmp = (uint16_t)(TIM_ICPolarity << 12);


  tmpccmr2 &= ((uint16_t)~TIM_CCMR1_CC2S) & ((uint16_t)~TIM_CCMR1_IC2F);
  tmpccmr2 |= (uint16_t)(TIM_ICSelection << 8);
  tmpccmr2 |= (uint16_t)(TIM_ICFilter << 12);


  tmpccer &= (uint16_t)~(TIM_CCER_CC4P | TIM_CCER_CC4NP);
  tmpccer |= (uint16_t)(tmp | (uint16_t)TIM_CCER_CC4E);


  TIMx->CCMR2 = tmpccmr2;
  TIMx->CCER = tmpccer ;
}
