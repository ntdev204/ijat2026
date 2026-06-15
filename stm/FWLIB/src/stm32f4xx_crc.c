/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        stm32f4xx_crc.c
 *
 * Description:  stm32f4xx_crc.c module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#include "stm32f4xx_crc.h"


void CRC_ResetDR(void)
{

  CRC->CR = CRC_CR_RESET;
}


uint32_t CRC_CalcCRC(uint32_t Data)
{
  CRC->DR = Data;

  return (CRC->DR);
}


uint32_t CRC_CalcBlockCRC(uint32_t pBuffer[], uint32_t BufferLength)
{
  uint32_t index = 0;

  for(index = 0; index < BufferLength; index++)
  {
    CRC->DR = pBuffer[index];
  }
  return (CRC->DR);
}


uint32_t CRC_GetCRC(void)
{
  return (CRC->DR);
}


void CRC_SetIDRegister(uint8_t IDValue)
{
  CRC->IDR = IDValue;
}


uint8_t CRC_GetIDRegister(void)
{
  return (CRC->IDR);
}
