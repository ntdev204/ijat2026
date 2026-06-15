/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        stm32f4xx_rng.c
 *
 * Description:  stm32f4xx_rng.c module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#include "stm32f4xx_rng.h"
#include "stm32f4xx_rcc.h"


void RNG_DeInit(void)
{

  RCC_AHB2PeriphResetCmd(RCC_AHB2Periph_RNG, ENABLE);


  RCC_AHB2PeriphResetCmd(RCC_AHB2Periph_RNG, DISABLE);
}


void RNG_Cmd(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    RNG->CR |= RNG_CR_RNGEN;
  }
  else
  {

    RNG->CR &= ~RNG_CR_RNGEN;
  }
}


uint32_t RNG_GetRandomNumber(void)
{

  return RNG->DR;
}


void RNG_ITConfig(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    RNG->CR |= RNG_CR_IE;
  }
  else
  {

    RNG->CR &= ~RNG_CR_IE;
  }
}


FlagStatus RNG_GetFlagStatus(uint8_t RNG_FLAG)
{
  FlagStatus bitstatus = RESET;

  assert_param(IS_RNG_GET_FLAG(RNG_FLAG));


  if ((RNG->SR & RNG_FLAG) != (uint8_t)RESET)
  {

    bitstatus = SET;
  }
  else
  {

    bitstatus = RESET;
  }

  return  bitstatus;
}


void RNG_ClearFlag(uint8_t RNG_FLAG)
{

  assert_param(IS_RNG_CLEAR_FLAG(RNG_FLAG));

  RNG->SR = ~(uint32_t)(((uint32_t)RNG_FLAG) << 4);
}


ITStatus RNG_GetITStatus(uint8_t RNG_IT)
{
  ITStatus bitstatus = RESET;

  assert_param(IS_RNG_GET_IT(RNG_IT));


  if ((RNG->SR & RNG_IT) != (uint8_t)RESET)
  {

    bitstatus = SET;
  }
  else
  {

    bitstatus = RESET;
  }

  return bitstatus;
}


void RNG_ClearITPendingBit(uint8_t RNG_IT)
{

  assert_param(IS_RNG_IT(RNG_IT));


  RNG->SR = (uint8_t)~RNG_IT;
}
