/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        stm32f4xx_pwr.c
 *
 * Description:  stm32f4xx_pwr.c module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#include "stm32f4xx_pwr.h"
#include "stm32f4xx_rcc.h"


#define PWR_OFFSET               (PWR_BASE - PERIPH_BASE)


#define CR_OFFSET                (PWR_OFFSET + 0x00)
#define DBP_BitNumber            0x08
#define CR_DBP_BB                (PERIPH_BB_BASE + (CR_OFFSET * 32) + (DBP_BitNumber * 4))


#define PVDE_BitNumber           0x04
#define CR_PVDE_BB               (PERIPH_BB_BASE + (CR_OFFSET * 32) + (PVDE_BitNumber * 4))


#define FPDS_BitNumber           0x09
#define CR_FPDS_BB               (PERIPH_BB_BASE + (CR_OFFSET * 32) + (FPDS_BitNumber * 4))


#define PMODE_BitNumber           0x0E
#define CR_PMODE_BB               (PERIPH_BB_BASE + (CR_OFFSET * 32) + (PMODE_BitNumber * 4))


#define ODEN_BitNumber           0x10
#define CR_ODEN_BB               (PERIPH_BB_BASE + (CR_OFFSET * 32) + (ODEN_BitNumber * 4))


#define ODSWEN_BitNumber         0x11
#define CR_ODSWEN_BB             (PERIPH_BB_BASE + (CR_OFFSET * 32) + (ODSWEN_BitNumber * 4))


#define MRLVDS_BitNumber         0x0B
#define CR_MRLVDS_BB             (PERIPH_BB_BASE + (CR_OFFSET * 32) + (MRLVDS_BitNumber * 4))


#define LPLVDS_BitNumber         0x0A
#define CR_LPLVDS_BB             (PERIPH_BB_BASE + (CR_OFFSET * 32) + (LPLVDS_BitNumber * 4))


#define CSR_OFFSET               (PWR_OFFSET + 0x04)
#define EWUP_BitNumber           0x08
#define CSR_EWUP_BB              (PERIPH_BB_BASE + (CSR_OFFSET * 32) + (EWUP_BitNumber * 4))


#define BRE_BitNumber            0x09
#define CSR_BRE_BB              (PERIPH_BB_BASE + (CSR_OFFSET * 32) + (BRE_BitNumber * 4))


#define CR_DS_MASK               ((uint32_t)0xFFFFF3FC)
#define CR_PLS_MASK              ((uint32_t)0xFFFFFF1F)
#define CR_VOS_MASK              ((uint32_t)0xFFFF3FFF)


void PWR_DeInit(void)
{
  RCC_APB1PeriphResetCmd(RCC_APB1Periph_PWR, ENABLE);
  RCC_APB1PeriphResetCmd(RCC_APB1Periph_PWR, DISABLE);
}


void PWR_BackupAccessCmd(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));

  *(__IO uint32_t *) CR_DBP_BB = (uint32_t)NewState;
}


void PWR_PVDLevelConfig(uint32_t PWR_PVDLevel)
{
  uint32_t tmpreg = 0;


  assert_param(IS_PWR_PVD_LEVEL(PWR_PVDLevel));

  tmpreg = PWR->CR;


  tmpreg &= CR_PLS_MASK;


  tmpreg |= PWR_PVDLevel;


  PWR->CR = tmpreg;
}


void PWR_PVDCmd(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));

  *(__IO uint32_t *) CR_PVDE_BB = (uint32_t)NewState;
}


void PWR_WakeUpPinCmd(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));

  *(__IO uint32_t *) CSR_EWUP_BB = (uint32_t)NewState;
}


void PWR_BackupRegulatorCmd(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));

  *(__IO uint32_t *) CSR_BRE_BB = (uint32_t)NewState;
}


void PWR_MainRegulatorModeConfig(uint32_t PWR_Regulator_Voltage)
{
  uint32_t tmpreg = 0;


  assert_param(IS_PWR_REGULATOR_VOLTAGE(PWR_Regulator_Voltage));

  tmpreg = PWR->CR;


  tmpreg &= CR_VOS_MASK;


  tmpreg |= PWR_Regulator_Voltage;


  PWR->CR = tmpreg;
}


void PWR_OverDriveCmd(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));


  *(__IO uint32_t *) CR_ODEN_BB = (uint32_t)NewState;
}


void PWR_OverDriveSWCmd(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));


  *(__IO uint32_t *) CR_ODSWEN_BB = (uint32_t)NewState;
}


void PWR_UnderDriveCmd(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    PWR->CR |= (uint32_t)PWR_CR_UDEN;
  }
  else
  {

    PWR->CR &= (uint32_t)(~PWR_CR_UDEN);
  }
}


void PWR_MainRegulatorLowVoltageCmd(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {
    *(__IO uint32_t *) CR_MRLVDS_BB = (uint32_t)ENABLE;
  }
  else
  {
    *(__IO uint32_t *) CR_MRLVDS_BB = (uint32_t)DISABLE;
  }
}


void PWR_LowRegulatorLowVoltageCmd(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {
    *(__IO uint32_t *) CR_LPLVDS_BB = (uint32_t)ENABLE;
  }
  else
  {
    *(__IO uint32_t *) CR_LPLVDS_BB = (uint32_t)DISABLE;
  }
}


void PWR_FlashPowerDownCmd(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));

  *(__IO uint32_t *) CR_FPDS_BB = (uint32_t)NewState;
}


void PWR_EnterSTOPMode(uint32_t PWR_Regulator, uint8_t PWR_STOPEntry)
{
  uint32_t tmpreg = 0;


  assert_param(IS_PWR_REGULATOR(PWR_Regulator));
  assert_param(IS_PWR_STOP_ENTRY(PWR_STOPEntry));


  tmpreg = PWR->CR;

  tmpreg &= CR_DS_MASK;


  tmpreg |= PWR_Regulator;


  PWR->CR = tmpreg;


  SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;


  if(PWR_STOPEntry == PWR_STOPEntry_WFI)
  {

    __WFI();
  }
  else
  {

    __WFE();
  }

  SCB->SCR &= (uint32_t)~((uint32_t)SCB_SCR_SLEEPDEEP_Msk);
}


void PWR_EnterUnderDriveSTOPMode(uint32_t PWR_Regulator, uint8_t PWR_STOPEntry)
{
  uint32_t tmpreg = 0;


  assert_param(IS_PWR_REGULATOR_UNDERDRIVE(PWR_Regulator));
  assert_param(IS_PWR_STOP_ENTRY(PWR_STOPEntry));


  tmpreg = PWR->CR;

  tmpreg &= CR_DS_MASK;


  tmpreg |= PWR_Regulator;


  PWR->CR = tmpreg;


  SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;


  if(PWR_STOPEntry == PWR_STOPEntry_WFI)
  {

    __WFI();
  }
  else
  {

    __WFE();
  }

  SCB->SCR &= (uint32_t)~((uint32_t)SCB_SCR_SLEEPDEEP_Msk);
}


void PWR_EnterSTANDBYMode(void)
{

  PWR->CR |= PWR_CR_PDDS;


  SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;


#if defined ( __CC_ARM   )
  __force_stores();
#endif

  __WFI();
}


FlagStatus PWR_GetFlagStatus(uint32_t PWR_FLAG)
{
  FlagStatus bitstatus = RESET;


  assert_param(IS_PWR_GET_FLAG(PWR_FLAG));

  if ((PWR->CSR & PWR_FLAG) != (uint32_t)RESET)
  {
    bitstatus = SET;
  }
  else
  {
    bitstatus = RESET;
  }

  return bitstatus;
}


void PWR_ClearFlag(uint32_t PWR_FLAG)
{

  assert_param(IS_PWR_CLEAR_FLAG(PWR_FLAG));

#if defined (STM32F427_437xx) || defined (STM32F429_439xx)
  if (PWR_FLAG != PWR_FLAG_UDRDY)
  {
    PWR->CR |=  PWR_FLAG << 2;
  }
  else
  {
    PWR->CSR |= PWR_FLAG_UDRDY;
  }
#endif

#if defined (STM32F40_41xxx) || defined (STM32F401xx) || defined (STM32F411xE)
  PWR->CR |=  PWR_FLAG << 2;
#endif
}
