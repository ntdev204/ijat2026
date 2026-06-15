/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        stm32f4xx_wwdg.c
 *
 * Description:  stm32f4xx_wwdg.c module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#include "stm32f4xx_wwdg.h"
#include "stm32f4xx_rcc.h"


#define WWDG_OFFSET       (WWDG_BASE - PERIPH_BASE)

#define CFR_OFFSET        (WWDG_OFFSET + 0x04)
#define EWI_BitNumber     0x09
#define CFR_EWI_BB        (PERIPH_BB_BASE + (CFR_OFFSET * 32) + (EWI_BitNumber * 4))


#define CFR_WDGTB_MASK    ((uint32_t)0xFFFFFE7F)
#define CFR_W_MASK        ((uint32_t)0xFFFFFF80)
#define BIT_MASK          ((uint8_t)0x7F)


void WWDG_DeInit(void)
{
  RCC_APB1PeriphResetCmd(RCC_APB1Periph_WWDG, ENABLE);
  RCC_APB1PeriphResetCmd(RCC_APB1Periph_WWDG, DISABLE);
}


void WWDG_SetPrescaler(uint32_t WWDG_Prescaler)
{
  uint32_t tmpreg = 0;

  assert_param(IS_WWDG_PRESCALER(WWDG_Prescaler));

  tmpreg = WWDG->CFR & CFR_WDGTB_MASK;

  tmpreg |= WWDG_Prescaler;

  WWDG->CFR = tmpreg;
}


void WWDG_SetWindowValue(uint8_t WindowValue)
{
  __IO uint32_t tmpreg = 0;


  assert_param(IS_WWDG_WINDOW_VALUE(WindowValue));


  tmpreg = WWDG->CFR & CFR_W_MASK;


  tmpreg |= WindowValue & (uint32_t) BIT_MASK;


  WWDG->CFR = tmpreg;
}


void WWDG_EnableIT(void)
{
  *(__IO uint32_t *) CFR_EWI_BB = (uint32_t)ENABLE;
}


void WWDG_SetCounter(uint8_t Counter)
{

  assert_param(IS_WWDG_COUNTER(Counter));

  WWDG->CR = Counter & BIT_MASK;
}


void WWDG_Enable(uint8_t Counter)
{

  assert_param(IS_WWDG_COUNTER(Counter));
  WWDG->CR = WWDG_CR_WDGA | Counter;
}


FlagStatus WWDG_GetFlagStatus(void)
{
  FlagStatus bitstatus = RESET;

  if ((WWDG->SR) != (uint32_t)RESET)
  {
    bitstatus = SET;
  }
  else
  {
    bitstatus = RESET;
  }
  return bitstatus;
}


void WWDG_ClearFlag(void)
{
  WWDG->SR = (uint32_t)RESET;
}
