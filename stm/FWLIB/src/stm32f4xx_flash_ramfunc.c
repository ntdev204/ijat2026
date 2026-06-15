/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        stm32f4xx_flash_ramfunc.c
 *
 * Description:  stm32f4xx_flash_ramfunc.c module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#include "stm32f4xx_flash_ramfunc.h"


__RAM_FUNC FLASH_FlashInterfaceCmd(FunctionalState NewState)
{
  if (NewState != DISABLE)
  {

    CLEAR_BIT(PWR->CR, PWR_CR_FISSR);
  }
  else
  {

    SET_BIT(PWR->CR, PWR_CR_FISSR);
  }
}


__RAM_FUNC FLASH_FlashSleepModeCmd(FunctionalState NewState)
{
  if (NewState != DISABLE)
  {

    SET_BIT(PWR->CR, PWR_CR_FMSSR);
  }
  else
  {

    CLEAR_BIT(PWR->CR, PWR_CR_FMSSR);
  }
}
