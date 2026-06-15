/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        stm32f4xx_cryp_tdes.c
 *
 * Description:  stm32f4xx_cryp_tdes.c module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#include "stm32f4xx_cryp.h"


#define TDESBUSY_TIMEOUT    ((uint32_t) 0x00010000)


ErrorStatus CRYP_TDES_ECB(uint8_t Mode, uint8_t Key[24], uint8_t *Input,
                          uint32_t Ilength, uint8_t *Output)
{
  CRYP_InitTypeDef TDES_CRYP_InitStructure;
  CRYP_KeyInitTypeDef TDES_CRYP_KeyInitStructure;
  __IO uint32_t counter = 0;
  uint32_t busystatus = 0;
  ErrorStatus status = SUCCESS;
  uint32_t keyaddr    = (uint32_t)Key;
  uint32_t inputaddr  = (uint32_t)Input;
  uint32_t outputaddr = (uint32_t)Output;
  uint32_t i = 0;


  CRYP_KeyStructInit(&TDES_CRYP_KeyInitStructure);


  if(Mode == MODE_ENCRYPT)
  {
     TDES_CRYP_InitStructure.CRYP_AlgoDir = CRYP_AlgoDir_Encrypt;
  }
  else
  {
     TDES_CRYP_InitStructure.CRYP_AlgoDir = CRYP_AlgoDir_Decrypt;
  }

  TDES_CRYP_InitStructure.CRYP_AlgoMode = CRYP_AlgoMode_TDES_ECB;
  TDES_CRYP_InitStructure.CRYP_DataType = CRYP_DataType_8b;
  CRYP_Init(&TDES_CRYP_InitStructure);


  TDES_CRYP_KeyInitStructure.CRYP_Key1Left = __REV(*(uint32_t*)(keyaddr));
  keyaddr+=4;
  TDES_CRYP_KeyInitStructure.CRYP_Key1Right= __REV(*(uint32_t*)(keyaddr));
  keyaddr+=4;
  TDES_CRYP_KeyInitStructure.CRYP_Key2Left = __REV(*(uint32_t*)(keyaddr));
  keyaddr+=4;
  TDES_CRYP_KeyInitStructure.CRYP_Key2Right= __REV(*(uint32_t*)(keyaddr));
  keyaddr+=4;
  TDES_CRYP_KeyInitStructure.CRYP_Key3Left = __REV(*(uint32_t*)(keyaddr));
  keyaddr+=4;
  TDES_CRYP_KeyInitStructure.CRYP_Key3Right= __REV(*(uint32_t*)(keyaddr));
  CRYP_KeyInit(& TDES_CRYP_KeyInitStructure);


  CRYP_FIFOFlush();


  CRYP_Cmd(ENABLE);

  if(CRYP_GetCmdStatus() == DISABLE)
  {

    return(ERROR);
  }
  for(i=0; ((i<Ilength) && (status != ERROR)); i+=8)
  {

    CRYP_DataIn(*(uint32_t*)(inputaddr));
    inputaddr+=4;
    CRYP_DataIn(*(uint32_t*)(inputaddr));
    inputaddr+=4;


    counter = 0;
    do
    {
      busystatus = CRYP_GetFlagStatus(CRYP_FLAG_BUSY);
      counter++;
    }while ((counter != TDESBUSY_TIMEOUT) && (busystatus != RESET));

    if (busystatus != RESET)
    {
       status = ERROR;
    }
    else
    {


      *(uint32_t*)(outputaddr) = CRYP_DataOut();
      outputaddr+=4;
      *(uint32_t*)(outputaddr) = CRYP_DataOut();
      outputaddr+=4;
    }
  }


  CRYP_Cmd(DISABLE);

  return status;
}


ErrorStatus CRYP_TDES_CBC(uint8_t Mode, uint8_t Key[24], uint8_t InitVectors[8],
                          uint8_t *Input, uint32_t Ilength, uint8_t *Output)
{
  CRYP_InitTypeDef TDES_CRYP_InitStructure;
  CRYP_KeyInitTypeDef TDES_CRYP_KeyInitStructure;
  CRYP_IVInitTypeDef TDES_CRYP_IVInitStructure;
  __IO uint32_t counter = 0;
  uint32_t busystatus = 0;
  ErrorStatus status = SUCCESS;
  uint32_t keyaddr    = (uint32_t)Key;
  uint32_t inputaddr  = (uint32_t)Input;
  uint32_t outputaddr = (uint32_t)Output;
  uint32_t ivaddr     = (uint32_t)InitVectors;
  uint32_t i = 0;


  CRYP_KeyStructInit(&TDES_CRYP_KeyInitStructure);


  if(Mode == MODE_ENCRYPT)
  {
    TDES_CRYP_InitStructure.CRYP_AlgoDir = CRYP_AlgoDir_Encrypt;
  }
  else
  {
    TDES_CRYP_InitStructure.CRYP_AlgoDir = CRYP_AlgoDir_Decrypt;
  }
  TDES_CRYP_InitStructure.CRYP_AlgoMode = CRYP_AlgoMode_TDES_CBC;
  TDES_CRYP_InitStructure.CRYP_DataType = CRYP_DataType_8b;

  CRYP_Init(&TDES_CRYP_InitStructure);


  TDES_CRYP_KeyInitStructure.CRYP_Key1Left = __REV(*(uint32_t*)(keyaddr));
  keyaddr+=4;
  TDES_CRYP_KeyInitStructure.CRYP_Key1Right= __REV(*(uint32_t*)(keyaddr));
  keyaddr+=4;
  TDES_CRYP_KeyInitStructure.CRYP_Key2Left = __REV(*(uint32_t*)(keyaddr));
  keyaddr+=4;
  TDES_CRYP_KeyInitStructure.CRYP_Key2Right= __REV(*(uint32_t*)(keyaddr));
  keyaddr+=4;
  TDES_CRYP_KeyInitStructure.CRYP_Key3Left = __REV(*(uint32_t*)(keyaddr));
  keyaddr+=4;
  TDES_CRYP_KeyInitStructure.CRYP_Key3Right= __REV(*(uint32_t*)(keyaddr));
  CRYP_KeyInit(& TDES_CRYP_KeyInitStructure);


  TDES_CRYP_IVInitStructure.CRYP_IV0Left = __REV(*(uint32_t*)(ivaddr));
  ivaddr+=4;
  TDES_CRYP_IVInitStructure.CRYP_IV0Right= __REV(*(uint32_t*)(ivaddr));
  CRYP_IVInit(&TDES_CRYP_IVInitStructure);


  CRYP_FIFOFlush();


  CRYP_Cmd(ENABLE);

  if(CRYP_GetCmdStatus() == DISABLE)
  {

    return(ERROR);
  }

  for(i=0; ((i<Ilength) && (status != ERROR)); i+=8)
  {

    CRYP_DataIn(*(uint32_t*)(inputaddr));
    inputaddr+=4;
    CRYP_DataIn(*(uint32_t*)(inputaddr));
    inputaddr+=4;


    counter = 0;
    do
    {
      busystatus = CRYP_GetFlagStatus(CRYP_FLAG_BUSY);
      counter++;
    }while ((counter != TDESBUSY_TIMEOUT) && (busystatus != RESET));

    if (busystatus != RESET)
   {
       status = ERROR;
    }
    else
    {


      *(uint32_t*)(outputaddr) = CRYP_DataOut();
      outputaddr+=4;
      *(uint32_t*)(outputaddr) = CRYP_DataOut();
      outputaddr+=4;
    }
  }


  CRYP_Cmd(DISABLE);

  return status;
}
