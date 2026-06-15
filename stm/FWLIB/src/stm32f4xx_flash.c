/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        stm32f4xx_flash.c
 *
 * Description:  stm32f4xx_flash.c module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#include "stm32f4xx_flash.h"


#define SECTOR_MASK               ((uint32_t)0xFFFFFF07)


void FLASH_SetLatency(uint32_t FLASH_Latency)
{

  assert_param(IS_FLASH_LATENCY(FLASH_Latency));


  *(__IO uint8_t *)ACR_BYTE0_ADDRESS = (uint8_t)FLASH_Latency;
}


void FLASH_PrefetchBufferCmd(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));


  if(NewState != DISABLE)
  {
    FLASH->ACR |= FLASH_ACR_PRFTEN;
  }
  else
  {
    FLASH->ACR &= (~FLASH_ACR_PRFTEN);
  }
}


void FLASH_InstructionCacheCmd(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if(NewState != DISABLE)
  {
    FLASH->ACR |= FLASH_ACR_ICEN;
  }
  else
  {
    FLASH->ACR &= (~FLASH_ACR_ICEN);
  }
}


void FLASH_DataCacheCmd(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if(NewState != DISABLE)
  {
    FLASH->ACR |= FLASH_ACR_DCEN;
  }
  else
  {
    FLASH->ACR &= (~FLASH_ACR_DCEN);
  }
}


void FLASH_InstructionCacheReset(void)
{
  FLASH->ACR |= FLASH_ACR_ICRST;
}


void FLASH_DataCacheReset(void)
{
  FLASH->ACR |= FLASH_ACR_DCRST;
}


void FLASH_Unlock(void)
{
  if((FLASH->CR & FLASH_CR_LOCK) != RESET)
  {

    FLASH->KEYR = FLASH_KEY1;
    FLASH->KEYR = FLASH_KEY2;
  }
}


void FLASH_Lock(void)
{

  FLASH->CR |= FLASH_CR_LOCK;
}


FLASH_Status FLASH_EraseSector(uint32_t FLASH_Sector, uint8_t VoltageRange)
{
  uint32_t tmp_psize = 0x0;
  FLASH_Status status = FLASH_COMPLETE;


  assert_param(IS_FLASH_SECTOR(FLASH_Sector));
  assert_param(IS_VOLTAGERANGE(VoltageRange));

  if(VoltageRange == VoltageRange_1)
  {
     tmp_psize = FLASH_PSIZE_BYTE;
  }
  else if(VoltageRange == VoltageRange_2)
  {
    tmp_psize = FLASH_PSIZE_HALF_WORD;
  }
  else if(VoltageRange == VoltageRange_3)
  {
    tmp_psize = FLASH_PSIZE_WORD;
  }
  else
  {
    tmp_psize = FLASH_PSIZE_DOUBLE_WORD;
  }

  status = FLASH_WaitForLastOperation();

  if(status == FLASH_COMPLETE)
  {

    FLASH->CR &= CR_PSIZE_MASK;
    FLASH->CR |= tmp_psize;
    FLASH->CR &= SECTOR_MASK;
    FLASH->CR |= FLASH_CR_SER | FLASH_Sector;
    FLASH->CR |= FLASH_CR_STRT;


    status = FLASH_WaitForLastOperation();


    FLASH->CR &= (~FLASH_CR_SER);
    FLASH->CR &= SECTOR_MASK;
  }

  return status;
}


FLASH_Status FLASH_EraseAllSectors(uint8_t VoltageRange)
{
  uint32_t tmp_psize = 0x0;
  FLASH_Status status = FLASH_COMPLETE;


  status = FLASH_WaitForLastOperation();
  assert_param(IS_VOLTAGERANGE(VoltageRange));

  if(VoltageRange == VoltageRange_1)
  {
     tmp_psize = FLASH_PSIZE_BYTE;
  }
  else if(VoltageRange == VoltageRange_2)
  {
    tmp_psize = FLASH_PSIZE_HALF_WORD;
  }
  else if(VoltageRange == VoltageRange_3)
  {
    tmp_psize = FLASH_PSIZE_WORD;
  }
  else
  {
    tmp_psize = FLASH_PSIZE_DOUBLE_WORD;
  }
  if(status == FLASH_COMPLETE)
  {

#if defined (STM32F427_437xx) || defined (STM32F429_439xx)
    FLASH->CR &= CR_PSIZE_MASK;
    FLASH->CR |= tmp_psize;
    FLASH->CR |= (FLASH_CR_MER1 | FLASH_CR_MER2);
    FLASH->CR |= FLASH_CR_STRT;


    status = FLASH_WaitForLastOperation();


    FLASH->CR &= ~(FLASH_CR_MER1 | FLASH_CR_MER2);
#endif

#if defined (STM32F40_41xxx) || defined (STM32F401xx) || defined (STM32F411xE)
    FLASH->CR &= CR_PSIZE_MASK;
    FLASH->CR |= tmp_psize;
    FLASH->CR |= FLASH_CR_MER;
    FLASH->CR |= FLASH_CR_STRT;


    status = FLASH_WaitForLastOperation();


    FLASH->CR &= (~FLASH_CR_MER);
#endif

  }

  return status;
}


FLASH_Status FLASH_EraseAllBank1Sectors(uint8_t VoltageRange)
{
  uint32_t tmp_psize = 0x0;
  FLASH_Status status = FLASH_COMPLETE;


  status = FLASH_WaitForLastOperation();
  assert_param(IS_VOLTAGERANGE(VoltageRange));

  if(VoltageRange == VoltageRange_1)
  {
     tmp_psize = FLASH_PSIZE_BYTE;
  }
  else if(VoltageRange == VoltageRange_2)
  {
    tmp_psize = FLASH_PSIZE_HALF_WORD;
  }
  else if(VoltageRange == VoltageRange_3)
  {
    tmp_psize = FLASH_PSIZE_WORD;
  }
  else
  {
    tmp_psize = FLASH_PSIZE_DOUBLE_WORD;
  }
  if(status == FLASH_COMPLETE)
  {

     FLASH->CR &= CR_PSIZE_MASK;
     FLASH->CR |= tmp_psize;
     FLASH->CR |= FLASH_CR_MER1;
     FLASH->CR |= FLASH_CR_STRT;


    status = FLASH_WaitForLastOperation();


    FLASH->CR &= (~FLASH_CR_MER1);

  }

  return status;
}


FLASH_Status FLASH_EraseAllBank2Sectors(uint8_t VoltageRange)
{
  uint32_t tmp_psize = 0x0;
  FLASH_Status status = FLASH_COMPLETE;


  status = FLASH_WaitForLastOperation();
  assert_param(IS_VOLTAGERANGE(VoltageRange));

  if(VoltageRange == VoltageRange_1)
  {
     tmp_psize = FLASH_PSIZE_BYTE;
  }
  else if(VoltageRange == VoltageRange_2)
  {
    tmp_psize = FLASH_PSIZE_HALF_WORD;
  }
  else if(VoltageRange == VoltageRange_3)
  {
    tmp_psize = FLASH_PSIZE_WORD;
  }
  else
  {
    tmp_psize = FLASH_PSIZE_DOUBLE_WORD;
  }
  if(status == FLASH_COMPLETE)
  {

     FLASH->CR &= CR_PSIZE_MASK;
     FLASH->CR |= tmp_psize;
     FLASH->CR |= FLASH_CR_MER2;
     FLASH->CR |= FLASH_CR_STRT;


    status = FLASH_WaitForLastOperation();


    FLASH->CR &= (~FLASH_CR_MER2);

  }

  return status;
}


FLASH_Status FLASH_ProgramDoubleWord(uint32_t Address, uint64_t Data)
{
  FLASH_Status status = FLASH_COMPLETE;


  assert_param(IS_FLASH_ADDRESS(Address));


  status = FLASH_WaitForLastOperation();

  if(status == FLASH_COMPLETE)
  {

    FLASH->CR &= CR_PSIZE_MASK;
    FLASH->CR |= FLASH_PSIZE_DOUBLE_WORD;
    FLASH->CR |= FLASH_CR_PG;

    *(__IO uint64_t*)Address = Data;


    status = FLASH_WaitForLastOperation();


    FLASH->CR &= (~FLASH_CR_PG);
  }

  return status;
}


FLASH_Status FLASH_ProgramWord(uint32_t Address, uint32_t Data)
{
  FLASH_Status status = FLASH_COMPLETE;


  assert_param(IS_FLASH_ADDRESS(Address));


  status = FLASH_WaitForLastOperation();

  if(status == FLASH_COMPLETE)
  {

    FLASH->CR &= CR_PSIZE_MASK;
    FLASH->CR |= FLASH_PSIZE_WORD;
    FLASH->CR |= FLASH_CR_PG;

    *(__IO uint32_t*)Address = Data;


    status = FLASH_WaitForLastOperation();


    FLASH->CR &= (~FLASH_CR_PG);
  }

  return status;
}


FLASH_Status FLASH_ProgramHalfWord(uint32_t Address, uint16_t Data)
{
  FLASH_Status status = FLASH_COMPLETE;


  assert_param(IS_FLASH_ADDRESS(Address));


  status = FLASH_WaitForLastOperation();

  if(status == FLASH_COMPLETE)
  {

    FLASH->CR &= CR_PSIZE_MASK;
    FLASH->CR |= FLASH_PSIZE_HALF_WORD;
    FLASH->CR |= FLASH_CR_PG;

    *(__IO uint16_t*)Address = Data;


    status = FLASH_WaitForLastOperation();


    FLASH->CR &= (~FLASH_CR_PG);
  }

  return status;
}


FLASH_Status FLASH_ProgramByte(uint32_t Address, uint8_t Data)
{
  FLASH_Status status = FLASH_COMPLETE;


  assert_param(IS_FLASH_ADDRESS(Address));


  status = FLASH_WaitForLastOperation();

  if(status == FLASH_COMPLETE)
  {

    FLASH->CR &= CR_PSIZE_MASK;
    FLASH->CR |= FLASH_PSIZE_BYTE;
    FLASH->CR |= FLASH_CR_PG;

    *(__IO uint8_t*)Address = Data;


    status = FLASH_WaitForLastOperation();


    FLASH->CR &= (~FLASH_CR_PG);
  }


  return status;
}


void FLASH_OB_Unlock(void)
{
  if((FLASH->OPTCR & FLASH_OPTCR_OPTLOCK) != RESET)
  {

    FLASH->OPTKEYR = FLASH_OPT_KEY1;
    FLASH->OPTKEYR = FLASH_OPT_KEY2;
  }
}


void FLASH_OB_Lock(void)
{

  FLASH->OPTCR |= FLASH_OPTCR_OPTLOCK;
}


void FLASH_OB_WRPConfig(uint32_t OB_WRP, FunctionalState NewState)
{
  FLASH_Status status = FLASH_COMPLETE;


  assert_param(IS_OB_WRP(OB_WRP));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  status = FLASH_WaitForLastOperation();

  if(status == FLASH_COMPLETE)
  {
    if(NewState != DISABLE)
    {
      *(__IO uint16_t*)OPTCR_BYTE2_ADDRESS &= (~OB_WRP);
    }
    else
    {
      *(__IO uint16_t*)OPTCR_BYTE2_ADDRESS |= (uint16_t)OB_WRP;
    }
  }
}


void FLASH_OB_WRP1Config(uint32_t OB_WRP, FunctionalState NewState)
{
  FLASH_Status status = FLASH_COMPLETE;


  assert_param(IS_OB_WRP(OB_WRP));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  status = FLASH_WaitForLastOperation();

  if(status == FLASH_COMPLETE)
  {
    if(NewState != DISABLE)
    {
      *(__IO uint16_t*)OPTCR1_BYTE2_ADDRESS &= (~OB_WRP);
    }
    else
    {
      *(__IO uint16_t*)OPTCR1_BYTE2_ADDRESS |= (uint16_t)OB_WRP;
    }
  }
}


void FLASH_OB_PCROPSelectionConfig(uint8_t OB_PcROP)
{
  uint8_t optiontmp = 0xFF;


  assert_param(IS_OB_PCROP_SELECT(OB_PcROP));


  optiontmp =  (uint8_t)((*(__IO uint8_t *)OPTCR_BYTE3_ADDRESS) & (uint8_t)0x7F);

  *(__IO uint8_t *)OPTCR_BYTE3_ADDRESS = (uint8_t)(OB_PcROP | optiontmp);

}


void FLASH_OB_PCROPConfig(uint32_t OB_PCROP, FunctionalState NewState)
{
  FLASH_Status status = FLASH_COMPLETE;


  assert_param(IS_OB_PCROP(OB_PCROP));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  status = FLASH_WaitForLastOperation();

  if(status == FLASH_COMPLETE)
  {
    if(NewState != DISABLE)
    {
      *(__IO uint16_t*)OPTCR_BYTE2_ADDRESS |= (uint16_t)OB_PCROP;
    }
    else
    {
      *(__IO uint16_t*)OPTCR_BYTE2_ADDRESS &= (~OB_PCROP);
    }
  }
}


void FLASH_OB_PCROP1Config(uint32_t OB_PCROP, FunctionalState NewState)
{
  FLASH_Status status = FLASH_COMPLETE;


  assert_param(IS_OB_PCROP(OB_PCROP));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  status = FLASH_WaitForLastOperation();

  if(status == FLASH_COMPLETE)
  {
    if(NewState != DISABLE)
    {
      *(__IO uint16_t*)OPTCR1_BYTE2_ADDRESS |= (uint16_t)OB_PCROP;
    }
    else
    {
      *(__IO uint16_t*)OPTCR1_BYTE2_ADDRESS &= (~OB_PCROP);
    }
  }
}


void FLASH_OB_RDPConfig(uint8_t OB_RDP)
{
  FLASH_Status status = FLASH_COMPLETE;


  assert_param(IS_OB_RDP(OB_RDP));

  status = FLASH_WaitForLastOperation();

  if(status == FLASH_COMPLETE)
  {
    *(__IO uint8_t*)OPTCR_BYTE1_ADDRESS = OB_RDP;

  }
}


void FLASH_OB_UserConfig(uint8_t OB_IWDG, uint8_t OB_STOP, uint8_t OB_STDBY)
{
  uint8_t optiontmp = 0xFF;
  FLASH_Status status = FLASH_COMPLETE;


  assert_param(IS_OB_IWDG_SOURCE(OB_IWDG));
  assert_param(IS_OB_STOP_SOURCE(OB_STOP));
  assert_param(IS_OB_STDBY_SOURCE(OB_STDBY));


  status = FLASH_WaitForLastOperation();

  if(status == FLASH_COMPLETE)
  {
#if defined (STM32F427_437xx) || defined (STM32F429_439xx)

    optiontmp =  (uint8_t)((*(__IO uint8_t *)OPTCR_BYTE0_ADDRESS) & (uint8_t)0x1F);
#endif

#if defined (STM32F40_41xxx) || defined (STM32F401xx) || defined (STM32F411xE)

    optiontmp =  (uint8_t)((*(__IO uint8_t *)OPTCR_BYTE0_ADDRESS) & (uint8_t)0x0F);
#endif


    *(__IO uint8_t *)OPTCR_BYTE0_ADDRESS = OB_IWDG | (uint8_t)(OB_STDBY | (uint8_t)(OB_STOP | ((uint8_t)optiontmp)));
  }
}


void FLASH_OB_BootConfig(uint8_t OB_BOOT)
{

  assert_param(IS_OB_BOOT(OB_BOOT));


  *(__IO uint8_t *)OPTCR_BYTE0_ADDRESS &= (~FLASH_OPTCR_BFB2);
  *(__IO uint8_t *)OPTCR_BYTE0_ADDRESS |= OB_BOOT;

}


void FLASH_OB_BORConfig(uint8_t OB_BOR)
{

  assert_param(IS_OB_BOR(OB_BOR));


  *(__IO uint8_t *)OPTCR_BYTE0_ADDRESS &= (~FLASH_OPTCR_BOR_LEV);
  *(__IO uint8_t *)OPTCR_BYTE0_ADDRESS |= OB_BOR;

}


FLASH_Status FLASH_OB_Launch(void)
{
  FLASH_Status status = FLASH_COMPLETE;


  *(__IO uint8_t *)OPTCR_BYTE0_ADDRESS |= FLASH_OPTCR_OPTSTRT;


  status = FLASH_WaitForLastOperation();

  return status;
}


uint8_t FLASH_OB_GetUser(void)
{

  return (uint8_t)(FLASH->OPTCR >> 5);
}


uint16_t FLASH_OB_GetWRP(void)
{

  return (*(__IO uint16_t *)(OPTCR_BYTE2_ADDRESS));
}


uint16_t FLASH_OB_GetWRP1(void)
{

  return (*(__IO uint16_t *)(OPTCR1_BYTE2_ADDRESS));
}


uint16_t FLASH_OB_GetPCROP(void)
{

  return (*(__IO uint16_t *)(OPTCR_BYTE2_ADDRESS));
}


uint16_t FLASH_OB_GetPCROP1(void)
{

  return (*(__IO uint16_t *)(OPTCR1_BYTE2_ADDRESS));
}


FlagStatus FLASH_OB_GetRDP(void)
{
  FlagStatus readstatus = RESET;

  if ((*(__IO uint8_t*)(OPTCR_BYTE1_ADDRESS) != (uint8_t)OB_RDP_Level_0))
  {
    readstatus = SET;
  }
  else
  {
    readstatus = RESET;
  }
  return readstatus;
}


uint8_t FLASH_OB_GetBOR(void)
{

  return (uint8_t)(*(__IO uint8_t *)(OPTCR_BYTE0_ADDRESS) & (uint8_t)0x0C);
}


void FLASH_ITConfig(uint32_t FLASH_IT, FunctionalState NewState)
{

  assert_param(IS_FLASH_IT(FLASH_IT));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if(NewState != DISABLE)
  {

    FLASH->CR |= FLASH_IT;
  }
  else
  {

    FLASH->CR &= ~(uint32_t)FLASH_IT;
  }
}


FlagStatus FLASH_GetFlagStatus(uint32_t FLASH_FLAG)
{
  FlagStatus bitstatus = RESET;

  assert_param(IS_FLASH_GET_FLAG(FLASH_FLAG));

  if((FLASH->SR & FLASH_FLAG) != (uint32_t)RESET)
  {
    bitstatus = SET;
  }
  else
  {
    bitstatus = RESET;
  }

  return bitstatus;
}


void FLASH_ClearFlag(uint32_t FLASH_FLAG)
{

  assert_param(IS_FLASH_CLEAR_FLAG(FLASH_FLAG));


  FLASH->SR = FLASH_FLAG;
}


FLASH_Status FLASH_GetStatus(void)
{
  FLASH_Status flashstatus = FLASH_COMPLETE;

  if((FLASH->SR & FLASH_FLAG_BSY) == FLASH_FLAG_BSY)
  {
    flashstatus = FLASH_BUSY;
  }
  else
  {
    if((FLASH->SR & FLASH_FLAG_WRPERR) != (uint32_t)0x00)
    {
      flashstatus = FLASH_ERROR_WRP;
    }
    else
    {
      if((FLASH->SR & FLASH_FLAG_RDERR) != (uint32_t)0x00)
      {
        flashstatus = FLASH_ERROR_RD;
      }
      else
      {
        if((FLASH->SR & (uint32_t)0xEF) != (uint32_t)0x00)
        {
          flashstatus = FLASH_ERROR_PROGRAM;
        }
        else
        {
          if((FLASH->SR & FLASH_FLAG_OPERR) != (uint32_t)0x00)
          {
            flashstatus = FLASH_ERROR_OPERATION;
          }
          else
          {
            flashstatus = FLASH_COMPLETE;
          }
        }
      }
    }
  }

  return flashstatus;
}


FLASH_Status FLASH_WaitForLastOperation(void)
{
  __IO FLASH_Status status = FLASH_COMPLETE;


  status = FLASH_GetStatus();


  while(status == FLASH_BUSY)
  {
    status = FLASH_GetStatus();
  }

  return status;
}
