/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        stm32f4xx_cryp_aes.c
 *
 * Description:  stm32f4xx_cryp_aes.c module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#include "stm32f4xx_cryp.h"


#define AESBUSY_TIMEOUT    ((uint32_t) 0x00010000)


ErrorStatus CRYP_AES_ECB(uint8_t Mode, uint8_t* Key, uint16_t Keysize,
                         uint8_t* Input, uint32_t Ilength, uint8_t* Output)
{
  CRYP_InitTypeDef AES_CRYP_InitStructure;
  CRYP_KeyInitTypeDef AES_CRYP_KeyInitStructure;
  __IO uint32_t counter = 0;
  uint32_t busystatus = 0;
  ErrorStatus status = SUCCESS;
  uint32_t keyaddr    = (uint32_t)Key;
  uint32_t inputaddr  = (uint32_t)Input;
  uint32_t outputaddr = (uint32_t)Output;
  uint32_t i = 0;


  CRYP_KeyStructInit(&AES_CRYP_KeyInitStructure);

  switch(Keysize)
  {
    case 128:
    AES_CRYP_InitStructure.CRYP_KeySize = CRYP_KeySize_128b;
    AES_CRYP_KeyInitStructure.CRYP_Key2Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key2Right= __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key3Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key3Right= __REV(*(uint32_t*)(keyaddr));
    break;
    case 192:
    AES_CRYP_InitStructure.CRYP_KeySize  = CRYP_KeySize_192b;
    AES_CRYP_KeyInitStructure.CRYP_Key1Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key1Right= __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key2Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key2Right= __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key3Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key3Right= __REV(*(uint32_t*)(keyaddr));
    break;
    case 256:
    AES_CRYP_InitStructure.CRYP_KeySize  = CRYP_KeySize_256b;
    AES_CRYP_KeyInitStructure.CRYP_Key0Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key0Right= __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key1Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key1Right= __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key2Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key2Right= __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key3Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key3Right= __REV(*(uint32_t*)(keyaddr));
    break;
    default:
    break;
  }


  if(Mode == MODE_DECRYPT)
  {

    CRYP_FIFOFlush();


    AES_CRYP_InitStructure.CRYP_AlgoDir = CRYP_AlgoDir_Decrypt;
    AES_CRYP_InitStructure.CRYP_AlgoMode = CRYP_AlgoMode_AES_Key;
    AES_CRYP_InitStructure.CRYP_DataType = CRYP_DataType_32b;
    CRYP_Init(&AES_CRYP_InitStructure);


    CRYP_KeyInit(&AES_CRYP_KeyInitStructure);


    CRYP_Cmd(ENABLE);


    do
    {
      busystatus = CRYP_GetFlagStatus(CRYP_FLAG_BUSY);
      counter++;
    }while ((counter != AESBUSY_TIMEOUT) && (busystatus != RESET));

    if (busystatus != RESET)
   {
       status = ERROR;
    }
    else
    {

      AES_CRYP_InitStructure.CRYP_AlgoDir = CRYP_AlgoDir_Decrypt;
    }
  }

  else
  {

    CRYP_KeyInit(&AES_CRYP_KeyInitStructure);


    AES_CRYP_InitStructure.CRYP_AlgoDir  = CRYP_AlgoDir_Encrypt;
  }

  AES_CRYP_InitStructure.CRYP_AlgoMode = CRYP_AlgoMode_AES_ECB;
  AES_CRYP_InitStructure.CRYP_DataType = CRYP_DataType_8b;
  CRYP_Init(&AES_CRYP_InitStructure);


  CRYP_FIFOFlush();


  CRYP_Cmd(ENABLE);

  if(CRYP_GetCmdStatus() == DISABLE)
  {

    return(ERROR);
  }

  for(i=0; ((i<Ilength) && (status != ERROR)); i+=16)
  {


    CRYP_DataIn(*(uint32_t*)(inputaddr));
    inputaddr+=4;
    CRYP_DataIn(*(uint32_t*)(inputaddr));
    inputaddr+=4;
    CRYP_DataIn(*(uint32_t*)(inputaddr));
    inputaddr+=4;
    CRYP_DataIn(*(uint32_t*)(inputaddr));
    inputaddr+=4;


    counter = 0;
    do
    {
      busystatus = CRYP_GetFlagStatus(CRYP_FLAG_BUSY);
      counter++;
    }while ((counter != AESBUSY_TIMEOUT) && (busystatus != RESET));

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
      *(uint32_t*)(outputaddr) = CRYP_DataOut();
      outputaddr+=4;
      *(uint32_t*)(outputaddr) = CRYP_DataOut();
      outputaddr+=4;
    }
  }


  CRYP_Cmd(DISABLE);

  return status;
}


ErrorStatus CRYP_AES_CBC(uint8_t Mode, uint8_t InitVectors[16], uint8_t *Key,
                         uint16_t Keysize, uint8_t *Input, uint32_t Ilength,
                         uint8_t *Output)
{
  CRYP_InitTypeDef AES_CRYP_InitStructure;
  CRYP_KeyInitTypeDef AES_CRYP_KeyInitStructure;
  CRYP_IVInitTypeDef AES_CRYP_IVInitStructure;
  __IO uint32_t counter = 0;
  uint32_t busystatus = 0;
  ErrorStatus status = SUCCESS;
  uint32_t keyaddr    = (uint32_t)Key;
  uint32_t inputaddr  = (uint32_t)Input;
  uint32_t outputaddr = (uint32_t)Output;
  uint32_t ivaddr = (uint32_t)InitVectors;
  uint32_t i = 0;


  CRYP_KeyStructInit(&AES_CRYP_KeyInitStructure);

  switch(Keysize)
  {
    case 128:
    AES_CRYP_InitStructure.CRYP_KeySize = CRYP_KeySize_128b;
    AES_CRYP_KeyInitStructure.CRYP_Key2Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key2Right= __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key3Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key3Right= __REV(*(uint32_t*)(keyaddr));
    break;
    case 192:
    AES_CRYP_InitStructure.CRYP_KeySize  = CRYP_KeySize_192b;
    AES_CRYP_KeyInitStructure.CRYP_Key1Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key1Right= __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key2Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key2Right= __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key3Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key3Right= __REV(*(uint32_t*)(keyaddr));
    break;
    case 256:
    AES_CRYP_InitStructure.CRYP_KeySize  = CRYP_KeySize_256b;
    AES_CRYP_KeyInitStructure.CRYP_Key0Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key0Right= __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key1Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key1Right= __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key2Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key2Right= __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key3Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key3Right= __REV(*(uint32_t*)(keyaddr));
    break;
    default:
    break;
  }


  AES_CRYP_IVInitStructure.CRYP_IV0Left = __REV(*(uint32_t*)(ivaddr));
  ivaddr+=4;
  AES_CRYP_IVInitStructure.CRYP_IV0Right= __REV(*(uint32_t*)(ivaddr));
  ivaddr+=4;
  AES_CRYP_IVInitStructure.CRYP_IV1Left = __REV(*(uint32_t*)(ivaddr));
  ivaddr+=4;
  AES_CRYP_IVInitStructure.CRYP_IV1Right= __REV(*(uint32_t*)(ivaddr));


  if(Mode == MODE_DECRYPT)
  {

    CRYP_FIFOFlush();


    AES_CRYP_InitStructure.CRYP_AlgoDir = CRYP_AlgoDir_Decrypt;
    AES_CRYP_InitStructure.CRYP_AlgoMode = CRYP_AlgoMode_AES_Key;
    AES_CRYP_InitStructure.CRYP_DataType = CRYP_DataType_32b;

    CRYP_Init(&AES_CRYP_InitStructure);


    CRYP_KeyInit(&AES_CRYP_KeyInitStructure);


    CRYP_Cmd(ENABLE);


    do
    {
      busystatus = CRYP_GetFlagStatus(CRYP_FLAG_BUSY);
      counter++;
    }while ((counter != AESBUSY_TIMEOUT) && (busystatus != RESET));

    if (busystatus != RESET)
   {
       status = ERROR;
    }
    else
    {

      AES_CRYP_InitStructure.CRYP_AlgoDir = CRYP_AlgoDir_Decrypt;
    }
  }

  else
  {
    CRYP_KeyInit(&AES_CRYP_KeyInitStructure);


    AES_CRYP_InitStructure.CRYP_AlgoDir  = CRYP_AlgoDir_Encrypt;
  }
  AES_CRYP_InitStructure.CRYP_AlgoMode = CRYP_AlgoMode_AES_CBC;
  AES_CRYP_InitStructure.CRYP_DataType = CRYP_DataType_8b;
  CRYP_Init(&AES_CRYP_InitStructure);


  CRYP_IVInit(&AES_CRYP_IVInitStructure);


  CRYP_FIFOFlush();


  CRYP_Cmd(ENABLE);

  if(CRYP_GetCmdStatus() == DISABLE)
  {

    return(ERROR);
  }

  for(i=0; ((i<Ilength) && (status != ERROR)); i+=16)
  {


    CRYP_DataIn(*(uint32_t*)(inputaddr));
    inputaddr+=4;
    CRYP_DataIn(*(uint32_t*)(inputaddr));
    inputaddr+=4;
    CRYP_DataIn(*(uint32_t*)(inputaddr));
    inputaddr+=4;
    CRYP_DataIn(*(uint32_t*)(inputaddr));
    inputaddr+=4;

    counter = 0;
    do
    {
      busystatus = CRYP_GetFlagStatus(CRYP_FLAG_BUSY);
      counter++;
    }while ((counter != AESBUSY_TIMEOUT) && (busystatus != RESET));

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
      *(uint32_t*)(outputaddr) = CRYP_DataOut();
      outputaddr+=4;
      *(uint32_t*)(outputaddr) = CRYP_DataOut();
      outputaddr+=4;
    }
  }


  CRYP_Cmd(DISABLE);

  return status;
}


ErrorStatus CRYP_AES_CTR(uint8_t Mode, uint8_t InitVectors[16], uint8_t *Key,
                         uint16_t Keysize, uint8_t *Input, uint32_t Ilength,
                         uint8_t *Output)
{
  CRYP_InitTypeDef AES_CRYP_InitStructure;
  CRYP_KeyInitTypeDef AES_CRYP_KeyInitStructure;
  CRYP_IVInitTypeDef AES_CRYP_IVInitStructure;
  __IO uint32_t counter = 0;
  uint32_t busystatus = 0;
  ErrorStatus status = SUCCESS;
  uint32_t keyaddr    = (uint32_t)Key;
  uint32_t inputaddr  = (uint32_t)Input;
  uint32_t outputaddr = (uint32_t)Output;
  uint32_t ivaddr     = (uint32_t)InitVectors;
  uint32_t i = 0;


  CRYP_KeyStructInit(&AES_CRYP_KeyInitStructure);

  switch(Keysize)
  {
    case 128:
    AES_CRYP_InitStructure.CRYP_KeySize = CRYP_KeySize_128b;
    AES_CRYP_KeyInitStructure.CRYP_Key2Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key2Right= __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key3Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key3Right= __REV(*(uint32_t*)(keyaddr));
    break;
    case 192:
    AES_CRYP_InitStructure.CRYP_KeySize  = CRYP_KeySize_192b;
    AES_CRYP_KeyInitStructure.CRYP_Key1Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key1Right= __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key2Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key2Right= __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key3Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key3Right= __REV(*(uint32_t*)(keyaddr));
    break;
    case 256:
    AES_CRYP_InitStructure.CRYP_KeySize  = CRYP_KeySize_256b;
    AES_CRYP_KeyInitStructure.CRYP_Key0Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key0Right= __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key1Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key1Right= __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key2Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key2Right= __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key3Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key3Right= __REV(*(uint32_t*)(keyaddr));
    break;
    default:
    break;
  }

  AES_CRYP_IVInitStructure.CRYP_IV0Left = __REV(*(uint32_t*)(ivaddr));
  ivaddr+=4;
  AES_CRYP_IVInitStructure.CRYP_IV0Right= __REV(*(uint32_t*)(ivaddr));
  ivaddr+=4;
  AES_CRYP_IVInitStructure.CRYP_IV1Left = __REV(*(uint32_t*)(ivaddr));
  ivaddr+=4;
  AES_CRYP_IVInitStructure.CRYP_IV1Right= __REV(*(uint32_t*)(ivaddr));


  CRYP_KeyInit(&AES_CRYP_KeyInitStructure);


  if(Mode == MODE_DECRYPT)
  {

    AES_CRYP_InitStructure.CRYP_AlgoDir = CRYP_AlgoDir_Decrypt;
  }

  else
  {

    AES_CRYP_InitStructure.CRYP_AlgoDir = CRYP_AlgoDir_Encrypt;
  }
  AES_CRYP_InitStructure.CRYP_AlgoMode = CRYP_AlgoMode_AES_CTR;
  AES_CRYP_InitStructure.CRYP_DataType = CRYP_DataType_8b;
  CRYP_Init(&AES_CRYP_InitStructure);


  CRYP_IVInit(&AES_CRYP_IVInitStructure);


  CRYP_FIFOFlush();


  CRYP_Cmd(ENABLE);

  if(CRYP_GetCmdStatus() == DISABLE)
  {

    return(ERROR);
  }

  for(i=0; ((i<Ilength) && (status != ERROR)); i+=16)
  {


    CRYP_DataIn(*(uint32_t*)(inputaddr));
    inputaddr+=4;
    CRYP_DataIn(*(uint32_t*)(inputaddr));
    inputaddr+=4;
    CRYP_DataIn(*(uint32_t*)(inputaddr));
    inputaddr+=4;
    CRYP_DataIn(*(uint32_t*)(inputaddr));
    inputaddr+=4;

    counter = 0;
    do
    {
      busystatus = CRYP_GetFlagStatus(CRYP_FLAG_BUSY);
      counter++;
    }while ((counter != AESBUSY_TIMEOUT) && (busystatus != RESET));

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
      *(uint32_t*)(outputaddr) = CRYP_DataOut();
      outputaddr+=4;
      *(uint32_t*)(outputaddr) = CRYP_DataOut();
      outputaddr+=4;
    }
  }

  CRYP_Cmd(DISABLE);

  return status;
}


ErrorStatus CRYP_AES_GCM(uint8_t Mode, uint8_t InitVectors[16],
                         uint8_t *Key, uint16_t Keysize,
                         uint8_t *Input, uint32_t ILength,
                         uint8_t *Header, uint32_t HLength,
                         uint8_t *Output, uint8_t *AuthTAG)
{
  CRYP_InitTypeDef AES_CRYP_InitStructure;
  CRYP_KeyInitTypeDef AES_CRYP_KeyInitStructure;
  CRYP_IVInitTypeDef AES_CRYP_IVInitStructure;
  __IO uint32_t counter = 0;
  uint32_t busystatus = 0;
  ErrorStatus status = SUCCESS;
  uint32_t keyaddr    = (uint32_t)Key;
  uint32_t inputaddr  = (uint32_t)Input;
  uint32_t outputaddr = (uint32_t)Output;
  uint32_t ivaddr     = (uint32_t)InitVectors;
  uint32_t headeraddr = (uint32_t)Header;
  uint32_t tagaddr = (uint32_t)AuthTAG;
  uint64_t headerlength = HLength * 8;
  uint64_t inputlength = ILength * 8;
  uint32_t loopcounter = 0;


  CRYP_KeyStructInit(&AES_CRYP_KeyInitStructure);

  switch(Keysize)
  {
    case 128:
    AES_CRYP_InitStructure.CRYP_KeySize = CRYP_KeySize_128b;
    AES_CRYP_KeyInitStructure.CRYP_Key2Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key2Right= __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key3Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key3Right= __REV(*(uint32_t*)(keyaddr));
    break;
    case 192:
    AES_CRYP_InitStructure.CRYP_KeySize  = CRYP_KeySize_192b;
    AES_CRYP_KeyInitStructure.CRYP_Key1Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key1Right= __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key2Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key2Right= __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key3Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key3Right= __REV(*(uint32_t*)(keyaddr));
    break;
    case 256:
    AES_CRYP_InitStructure.CRYP_KeySize  = CRYP_KeySize_256b;
    AES_CRYP_KeyInitStructure.CRYP_Key0Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key0Right= __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key1Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key1Right= __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key2Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key2Right= __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key3Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key3Right= __REV(*(uint32_t*)(keyaddr));
    break;
    default:
    break;
  }


  AES_CRYP_IVInitStructure.CRYP_IV0Left = __REV(*(uint32_t*)(ivaddr));
  ivaddr+=4;
  AES_CRYP_IVInitStructure.CRYP_IV0Right= __REV(*(uint32_t*)(ivaddr));
  ivaddr+=4;
  AES_CRYP_IVInitStructure.CRYP_IV1Left = __REV(*(uint32_t*)(ivaddr));
  ivaddr+=4;
  AES_CRYP_IVInitStructure.CRYP_IV1Right= __REV(*(uint32_t*)(ivaddr));


  if(Mode == MODE_ENCRYPT)
  {

    CRYP_FIFOFlush();


    CRYP_KeyInit(&AES_CRYP_KeyInitStructure);


    CRYP_IVInit(&AES_CRYP_IVInitStructure);


    AES_CRYP_InitStructure.CRYP_AlgoDir = CRYP_AlgoDir_Encrypt;
    AES_CRYP_InitStructure.CRYP_AlgoMode = CRYP_AlgoMode_AES_GCM;
    AES_CRYP_InitStructure.CRYP_DataType = CRYP_DataType_8b;
    CRYP_Init(&AES_CRYP_InitStructure);


    CRYP_PhaseConfig(CRYP_Phase_Init);


    CRYP_Cmd(ENABLE);


    while(CRYP_GetCmdStatus() == ENABLE)
    {
    }


    if(HLength != 0)
    {

      CRYP_PhaseConfig(CRYP_Phase_Header);


      CRYP_Cmd(ENABLE);

      if(CRYP_GetCmdStatus() == DISABLE)
      {

         return(ERROR);
      }

      for(loopcounter = 0; (loopcounter < HLength); loopcounter+=16)
      {

        while(CRYP_GetFlagStatus(CRYP_FLAG_IFEM) == RESET)
        {
        }


        CRYP_DataIn(*(uint32_t*)(headeraddr));
        headeraddr+=4;
        CRYP_DataIn(*(uint32_t*)(headeraddr));
        headeraddr+=4;
        CRYP_DataIn(*(uint32_t*)(headeraddr));
        headeraddr+=4;
        CRYP_DataIn(*(uint32_t*)(headeraddr));
        headeraddr+=4;
      }


      counter = 0;
      do
      {
        busystatus = CRYP_GetFlagStatus(CRYP_FLAG_BUSY);
        counter++;
      }while ((counter != AESBUSY_TIMEOUT) && (busystatus != RESET));

      if (busystatus != RESET)
      {
        status = ERROR;
      }
    }


    if(ILength != 0)
    {

      CRYP_PhaseConfig(CRYP_Phase_Payload);


      CRYP_Cmd(ENABLE);

      if(CRYP_GetCmdStatus() == DISABLE)
      {

        return(ERROR);
      }

      for(loopcounter = 0; ((loopcounter < ILength) && (status != ERROR)); loopcounter+=16)
      {

        while(CRYP_GetFlagStatus(CRYP_FLAG_IFEM) == RESET)
        {
        }

        CRYP_DataIn(*(uint32_t*)(inputaddr));
        inputaddr+=4;
        CRYP_DataIn(*(uint32_t*)(inputaddr));
        inputaddr+=4;
        CRYP_DataIn(*(uint32_t*)(inputaddr));
        inputaddr+=4;
        CRYP_DataIn(*(uint32_t*)(inputaddr));
        inputaddr+=4;


        counter = 0;
        do
        {
          busystatus = CRYP_GetFlagStatus(CRYP_FLAG_BUSY);
          counter++;
        }while ((counter != AESBUSY_TIMEOUT) && (busystatus != RESET));

        if (busystatus != RESET)
        {
          status = ERROR;
        }
        else
        {

          while(CRYP_GetFlagStatus(CRYP_FLAG_OFNE) == RESET)
          {
          }


          *(uint32_t*)(outputaddr) = CRYP_DataOut();
          outputaddr+=4;
          *(uint32_t*)(outputaddr) = CRYP_DataOut();
          outputaddr+=4;
          *(uint32_t*)(outputaddr) = CRYP_DataOut();
          outputaddr+=4;
          *(uint32_t*)(outputaddr) = CRYP_DataOut();
          outputaddr+=4;
        }
      }
    }


    CRYP_PhaseConfig(CRYP_Phase_Final);


    CRYP_Cmd(ENABLE);

    if(CRYP_GetCmdStatus() == DISABLE)
    {

      return(ERROR);
    }


    CRYP_DataIn(__REV(headerlength>>32));
    CRYP_DataIn(__REV(headerlength));
    CRYP_DataIn(__REV(inputlength>>32));
    CRYP_DataIn(__REV(inputlength));

    while(CRYP_GetFlagStatus(CRYP_FLAG_OFNE) == RESET)
    {
    }

    tagaddr = (uint32_t)AuthTAG;

    *(uint32_t*)(tagaddr) = CRYP_DataOut();
    tagaddr+=4;
    *(uint32_t*)(tagaddr) = CRYP_DataOut();
    tagaddr+=4;
    *(uint32_t*)(tagaddr) = CRYP_DataOut();
    tagaddr+=4;
    *(uint32_t*)(tagaddr) = CRYP_DataOut();
    tagaddr+=4;
  }

  else
  {

    CRYP_FIFOFlush();


    CRYP_KeyInit(&AES_CRYP_KeyInitStructure);


    CRYP_IVInit(&AES_CRYP_IVInitStructure);


    AES_CRYP_InitStructure.CRYP_AlgoDir = CRYP_AlgoDir_Decrypt;
    AES_CRYP_InitStructure.CRYP_AlgoMode = CRYP_AlgoMode_AES_GCM;
    AES_CRYP_InitStructure.CRYP_DataType = CRYP_DataType_8b;
    CRYP_Init(&AES_CRYP_InitStructure);


    CRYP_PhaseConfig(CRYP_Phase_Init);


    CRYP_Cmd(ENABLE);


    while(CRYP_GetCmdStatus() == ENABLE)
    {
    }


    if(HLength != 0)
    {

      CRYP_PhaseConfig(CRYP_Phase_Header);


      CRYP_Cmd(ENABLE);

      if(CRYP_GetCmdStatus() == DISABLE)
      {

        return(ERROR);
      }

      for(loopcounter = 0; (loopcounter < HLength); loopcounter+=16)
      {

        while(CRYP_GetFlagStatus(CRYP_FLAG_IFEM) == RESET)
        {
        }


        CRYP_DataIn(*(uint32_t*)(headeraddr));
        headeraddr+=4;
        CRYP_DataIn(*(uint32_t*)(headeraddr));
        headeraddr+=4;
        CRYP_DataIn(*(uint32_t*)(headeraddr));
        headeraddr+=4;
        CRYP_DataIn(*(uint32_t*)(headeraddr));
        headeraddr+=4;
      }


      counter = 0;
      do
      {
        busystatus = CRYP_GetFlagStatus(CRYP_FLAG_BUSY);
        counter++;
      }while ((counter != AESBUSY_TIMEOUT) && (busystatus != RESET));

      if (busystatus != RESET)
      {
        status = ERROR;
      }
    }


    if(ILength != 0)
    {

      CRYP_PhaseConfig(CRYP_Phase_Payload);


      CRYP_Cmd(ENABLE);

      if(CRYP_GetCmdStatus() == DISABLE)
      {

        return(ERROR);
      }

      for(loopcounter = 0; ((loopcounter < ILength) && (status != ERROR)); loopcounter+=16)
      {

        while(CRYP_GetFlagStatus(CRYP_FLAG_IFEM) == RESET)
        {
        }

        CRYP_DataIn(*(uint32_t*)(inputaddr));
        inputaddr+=4;
        CRYP_DataIn(*(uint32_t*)(inputaddr));
        inputaddr+=4;
        CRYP_DataIn(*(uint32_t*)(inputaddr));
        inputaddr+=4;
        CRYP_DataIn(*(uint32_t*)(inputaddr));
        inputaddr+=4;


        counter = 0;
        do
        {
          busystatus = CRYP_GetFlagStatus(CRYP_FLAG_BUSY);
          counter++;
        }while ((counter != AESBUSY_TIMEOUT) && (busystatus != RESET));

        if (busystatus != RESET)
        {
          status = ERROR;
        }
        else
        {

          while(CRYP_GetFlagStatus(CRYP_FLAG_OFNE) == RESET)
          {
          }


          *(uint32_t*)(outputaddr) = CRYP_DataOut();
          outputaddr+=4;
          *(uint32_t*)(outputaddr) = CRYP_DataOut();
          outputaddr+=4;
          *(uint32_t*)(outputaddr) = CRYP_DataOut();
          outputaddr+=4;
          *(uint32_t*)(outputaddr) = CRYP_DataOut();
          outputaddr+=4;
        }
      }
    }


    CRYP_PhaseConfig(CRYP_Phase_Final);


    CRYP_Cmd(ENABLE);

    if(CRYP_GetCmdStatus() == DISABLE)
    {

      return(ERROR);
    }


    CRYP_DataIn(__REV(headerlength>>32));
    CRYP_DataIn(__REV(headerlength));
    CRYP_DataIn(__REV(inputlength>>32));
    CRYP_DataIn(__REV(inputlength));

    while(CRYP_GetFlagStatus(CRYP_FLAG_OFNE) == RESET)
    {
    }

    tagaddr = (uint32_t)AuthTAG;

    *(uint32_t*)(tagaddr) = CRYP_DataOut();
    tagaddr+=4;
    *(uint32_t*)(tagaddr) = CRYP_DataOut();
    tagaddr+=4;
    *(uint32_t*)(tagaddr) = CRYP_DataOut();
    tagaddr+=4;
    *(uint32_t*)(tagaddr) = CRYP_DataOut();
    tagaddr+=4;
  }

  CRYP_Cmd(DISABLE);

  return status;
}


ErrorStatus CRYP_AES_CCM(uint8_t Mode,
                         uint8_t* Nonce, uint32_t NonceSize,
                         uint8_t *Key, uint16_t Keysize,
                         uint8_t *Input, uint32_t ILength,
                         uint8_t *Header, uint32_t HLength, uint8_t *HBuffer,
                         uint8_t *Output,
                         uint8_t *AuthTAG, uint32_t TAGSize)
{
  CRYP_InitTypeDef AES_CRYP_InitStructure;
  CRYP_KeyInitTypeDef AES_CRYP_KeyInitStructure;
  CRYP_IVInitTypeDef AES_CRYP_IVInitStructure;
  __IO uint32_t counter = 0;
  uint32_t busystatus = 0;
  ErrorStatus status = SUCCESS;
  uint32_t keyaddr    = (uint32_t)Key;
  uint32_t inputaddr  = (uint32_t)Input;
  uint32_t outputaddr = (uint32_t)Output;
  uint32_t headeraddr = (uint32_t)Header;
  uint32_t tagaddr = (uint32_t)AuthTAG;
  uint32_t headersize = HLength;
  uint32_t loopcounter = 0;
  uint32_t bufferidx = 0;
  uint8_t blockb0[16] = {0};
  uint8_t ctr[16] = {0};
  uint32_t temptag[4] = {0};
  uint32_t ctraddr = (uint32_t)ctr;
  uint32_t b0addr = (uint32_t)blockb0;


  if(headersize != 0)
  {

    if(headersize < 65280)
    {
      HBuffer[bufferidx++] = (uint8_t) ((headersize >> 8) & 0xFF);
      HBuffer[bufferidx++] = (uint8_t) ((headersize) & 0xFF);
      headersize += 2;
    }
    else
    {

      HBuffer[bufferidx++] = 0xFF;
      HBuffer[bufferidx++] = 0xFE;
      HBuffer[bufferidx++] = headersize & 0xff000000;
      HBuffer[bufferidx++] = headersize & 0x00ff0000;
      HBuffer[bufferidx++] = headersize & 0x0000ff00;
      HBuffer[bufferidx++] = headersize & 0x000000ff;
      headersize += 6;
    }

    for(loopcounter = 0; loopcounter < headersize; loopcounter++)
    {
      HBuffer[bufferidx++] = Header[loopcounter];
    }

    if ((headersize % 16) != 0)
    {

      for(loopcounter = headersize; loopcounter <= ((headersize/16) + 1) * 16; loopcounter++)
      {
        HBuffer[loopcounter] = 0;
      }

      headersize = ((headersize/16) + 1) * 16;
    }

    headeraddr = (uint32_t)HBuffer;
  }

  if(headersize != 0)
  {
    blockb0[0] = 0x40;
  }

  blockb0[0] |= 0u | (((( (uint8_t) TAGSize - 2) / 2) & 0x07 ) << 3 ) | ( ( (uint8_t) (15 - NonceSize) - 1) & 0x07);

  for (loopcounter = 0; loopcounter < NonceSize; loopcounter++)
  {
    blockb0[loopcounter+1] = Nonce[loopcounter];
  }
  for ( ; loopcounter < 13; loopcounter++)
  {
    blockb0[loopcounter+1] = 0;
  }

  blockb0[14] = ((ILength >> 8) & 0xFF);
  blockb0[15] = (ILength & 0xFF);


  ctr[0] = blockb0[0] & 0x07;

  for(loopcounter = 1; loopcounter < NonceSize + 1; loopcounter++)
  {
    ctr[loopcounter] = blockb0[loopcounter];
  }

  ctr[15] |= 0x01;


  CRYP_KeyStructInit(&AES_CRYP_KeyInitStructure);

  switch(Keysize)
  {
    case 128:
    AES_CRYP_InitStructure.CRYP_KeySize = CRYP_KeySize_128b;
    AES_CRYP_KeyInitStructure.CRYP_Key2Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key2Right= __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key3Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key3Right= __REV(*(uint32_t*)(keyaddr));
    break;
    case 192:
    AES_CRYP_InitStructure.CRYP_KeySize  = CRYP_KeySize_192b;
    AES_CRYP_KeyInitStructure.CRYP_Key1Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key1Right= __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key2Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key2Right= __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key3Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key3Right= __REV(*(uint32_t*)(keyaddr));
    break;
    case 256:
    AES_CRYP_InitStructure.CRYP_KeySize  = CRYP_KeySize_256b;
    AES_CRYP_KeyInitStructure.CRYP_Key0Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key0Right= __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key1Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key1Right= __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key2Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key2Right= __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key3Left = __REV(*(uint32_t*)(keyaddr));
    keyaddr+=4;
    AES_CRYP_KeyInitStructure.CRYP_Key3Right= __REV(*(uint32_t*)(keyaddr));
    break;
    default:
    break;
  }


  AES_CRYP_IVInitStructure.CRYP_IV0Left = (__REV(*(uint32_t*)(ctraddr)));
  ctraddr+=4;
  AES_CRYP_IVInitStructure.CRYP_IV0Right= (__REV(*(uint32_t*)(ctraddr)));
  ctraddr+=4;
  AES_CRYP_IVInitStructure.CRYP_IV1Left = (__REV(*(uint32_t*)(ctraddr)));
  ctraddr+=4;
  AES_CRYP_IVInitStructure.CRYP_IV1Right= (__REV(*(uint32_t*)(ctraddr)));


  if(Mode == MODE_ENCRYPT)
  {

    CRYP_FIFOFlush();


    CRYP_KeyInit(&AES_CRYP_KeyInitStructure);


    CRYP_IVInit(&AES_CRYP_IVInitStructure);


    AES_CRYP_InitStructure.CRYP_AlgoDir = CRYP_AlgoDir_Encrypt;
    AES_CRYP_InitStructure.CRYP_AlgoMode = CRYP_AlgoMode_AES_CCM;
    AES_CRYP_InitStructure.CRYP_DataType = CRYP_DataType_8b;
    CRYP_Init(&AES_CRYP_InitStructure);


    CRYP_PhaseConfig(CRYP_Phase_Init);

    b0addr = (uint32_t)blockb0;

    CRYP_DataIn((*(uint32_t*)(b0addr)));
    b0addr+=4;
    CRYP_DataIn((*(uint32_t*)(b0addr)));
    b0addr+=4;
    CRYP_DataIn((*(uint32_t*)(b0addr)));
    b0addr+=4;
    CRYP_DataIn((*(uint32_t*)(b0addr)));


    CRYP_Cmd(ENABLE);


    while(CRYP_GetCmdStatus() == ENABLE)
    {
    }

    if(headersize != 0)
    {

      CRYP_PhaseConfig(CRYP_Phase_Header);


      CRYP_Cmd(ENABLE);

      if(CRYP_GetCmdStatus() == DISABLE)
      {

         return(ERROR);
      }

      for(loopcounter = 0; (loopcounter < headersize); loopcounter+=16)
      {

        while(CRYP_GetFlagStatus(CRYP_FLAG_IFEM) == RESET)
        {
        }


        CRYP_DataIn(*(uint32_t*)(headeraddr));
        headeraddr+=4;
        CRYP_DataIn(*(uint32_t*)(headeraddr));
        headeraddr+=4;
        CRYP_DataIn(*(uint32_t*)(headeraddr));
        headeraddr+=4;
        CRYP_DataIn(*(uint32_t*)(headeraddr));
        headeraddr+=4;
      }


      counter = 0;
      do
      {
        busystatus = CRYP_GetFlagStatus(CRYP_FLAG_BUSY);
        counter++;
      }while ((counter != AESBUSY_TIMEOUT) && (busystatus != RESET));

      if (busystatus != RESET)
      {
        status = ERROR;
      }
    }


    if(ILength != 0)
    {

      CRYP_PhaseConfig(CRYP_Phase_Payload);


      CRYP_Cmd(ENABLE);

      if(CRYP_GetCmdStatus() == DISABLE)
      {

        return(ERROR);
      }

      for(loopcounter = 0; ((loopcounter < ILength) && (status != ERROR)); loopcounter+=16)
      {

        while(CRYP_GetFlagStatus(CRYP_FLAG_IFEM) == RESET)
        {
        }


        CRYP_DataIn(*(uint32_t*)(inputaddr));
        inputaddr+=4;
        CRYP_DataIn(*(uint32_t*)(inputaddr));
        inputaddr+=4;
        CRYP_DataIn(*(uint32_t*)(inputaddr));
        inputaddr+=4;
        CRYP_DataIn(*(uint32_t*)(inputaddr));
        inputaddr+=4;


        counter = 0;
        do
        {
          busystatus = CRYP_GetFlagStatus(CRYP_FLAG_BUSY);
          counter++;
        }while ((counter != AESBUSY_TIMEOUT) && (busystatus != RESET));

        if (busystatus != RESET)
        {
          status = ERROR;
        }
        else
        {

          while(CRYP_GetFlagStatus(CRYP_FLAG_OFNE) == RESET)
          {
          }


          *(uint32_t*)(outputaddr) = CRYP_DataOut();
          outputaddr+=4;
          *(uint32_t*)(outputaddr) = CRYP_DataOut();
          outputaddr+=4;
          *(uint32_t*)(outputaddr) = CRYP_DataOut();
          outputaddr+=4;
          *(uint32_t*)(outputaddr) = CRYP_DataOut();
          outputaddr+=4;
        }
      }
    }


    CRYP_PhaseConfig(CRYP_Phase_Final);


    CRYP_Cmd(ENABLE);

    if(CRYP_GetCmdStatus() == DISABLE)
    {

      return(ERROR);
    }

    ctraddr = (uint32_t)ctr;

    CRYP_DataIn(*(uint32_t*)(ctraddr));
    ctraddr+=4;
    CRYP_DataIn(*(uint32_t*)(ctraddr));
    ctraddr+=4;
    CRYP_DataIn(*(uint32_t*)(ctraddr));
    ctraddr+=4;

    CRYP_DataIn(*(uint32_t*)(ctraddr) & 0xfeffffff);


    while(CRYP_GetFlagStatus(CRYP_FLAG_OFNE) == RESET)
    {
    }


    temptag[0] = CRYP_DataOut();
    temptag[1] = CRYP_DataOut();
    temptag[2] = CRYP_DataOut();
    temptag[3] = CRYP_DataOut();
  }

  else
  {

    CRYP_FIFOFlush();


    CRYP_KeyInit(&AES_CRYP_KeyInitStructure);


    CRYP_IVInit(&AES_CRYP_IVInitStructure);


    AES_CRYP_InitStructure.CRYP_AlgoDir = CRYP_AlgoDir_Decrypt;
    AES_CRYP_InitStructure.CRYP_AlgoMode = CRYP_AlgoMode_AES_CCM;
    AES_CRYP_InitStructure.CRYP_DataType = CRYP_DataType_8b;
    CRYP_Init(&AES_CRYP_InitStructure);


    CRYP_PhaseConfig(CRYP_Phase_Init);

    b0addr = (uint32_t)blockb0;

    CRYP_DataIn((*(uint32_t*)(b0addr)));
    b0addr+=4;
    CRYP_DataIn((*(uint32_t*)(b0addr)));
    b0addr+=4;
    CRYP_DataIn((*(uint32_t*)(b0addr)));
    b0addr+=4;
    CRYP_DataIn((*(uint32_t*)(b0addr)));


    CRYP_Cmd(ENABLE);


    while(CRYP_GetCmdStatus() == ENABLE)
    {
    }


    if(headersize != 0)
    {

      CRYP_PhaseConfig(CRYP_Phase_Header);


      CRYP_Cmd(ENABLE);

      if(CRYP_GetCmdStatus() == DISABLE)
      {

        return(ERROR);
      }

      for(loopcounter = 0; (loopcounter < headersize); loopcounter+=16)
      {

        while(CRYP_GetFlagStatus(CRYP_FLAG_IFEM) == RESET)
        {
        }


        CRYP_DataIn(*(uint32_t*)(headeraddr));
        headeraddr+=4;
        CRYP_DataIn(*(uint32_t*)(headeraddr));
        headeraddr+=4;
        CRYP_DataIn(*(uint32_t*)(headeraddr));
        headeraddr+=4;
        CRYP_DataIn(*(uint32_t*)(headeraddr));
        headeraddr+=4;
      }


      counter = 0;
      do
      {
        busystatus = CRYP_GetFlagStatus(CRYP_FLAG_BUSY);
        counter++;
      }while ((counter != AESBUSY_TIMEOUT) && (busystatus != RESET));

      if (busystatus != RESET)
      {
        status = ERROR;
      }
    }


    if(ILength != 0)
    {

      CRYP_PhaseConfig(CRYP_Phase_Payload);


      CRYP_Cmd(ENABLE);

      if(CRYP_GetCmdStatus() == DISABLE)
      {

        return(ERROR);
      }

      for(loopcounter = 0; ((loopcounter < ILength) && (status != ERROR)); loopcounter+=16)
      {

        while(CRYP_GetFlagStatus(CRYP_FLAG_IFEM) == RESET)
        {
        }


        CRYP_DataIn(*(uint32_t*)(inputaddr));
        inputaddr+=4;
        CRYP_DataIn(*(uint32_t*)(inputaddr));
        inputaddr+=4;
        CRYP_DataIn(*(uint32_t*)(inputaddr));
        inputaddr+=4;
        CRYP_DataIn(*(uint32_t*)(inputaddr));
        inputaddr+=4;


        counter = 0;
        do
        {
          busystatus = CRYP_GetFlagStatus(CRYP_FLAG_BUSY);
          counter++;
        }while ((counter != AESBUSY_TIMEOUT) && (busystatus != RESET));

        if (busystatus != RESET)
        {
          status = ERROR;
        }
        else
        {

          while(CRYP_GetFlagStatus(CRYP_FLAG_OFNE) == RESET)
          {
          }


          *(uint32_t*)(outputaddr) = CRYP_DataOut();
          outputaddr+=4;
          *(uint32_t*)(outputaddr) = CRYP_DataOut();
          outputaddr+=4;
          *(uint32_t*)(outputaddr) = CRYP_DataOut();
          outputaddr+=4;
          *(uint32_t*)(outputaddr) = CRYP_DataOut();
          outputaddr+=4;
        }
      }
    }


    CRYP_PhaseConfig(CRYP_Phase_Final);


    CRYP_Cmd(ENABLE);

    if(CRYP_GetCmdStatus() == DISABLE)
    {

      return(ERROR);
    }

    ctraddr = (uint32_t)ctr;

    CRYP_DataIn(*(uint32_t*)(ctraddr));
    ctraddr+=4;
    CRYP_DataIn(*(uint32_t*)(ctraddr));
    ctraddr+=4;
    CRYP_DataIn(*(uint32_t*)(ctraddr));
    ctraddr+=4;

    CRYP_DataIn(*(uint32_t*)(ctraddr) & 0xfeffffff);


    while(CRYP_GetFlagStatus(CRYP_FLAG_OFNE) == RESET)
    {
    }


    temptag[0] = CRYP_DataOut();
    temptag[1] = CRYP_DataOut();
    temptag[2] = CRYP_DataOut();
    temptag[3] = CRYP_DataOut();
  }


  for(loopcounter = 0; (loopcounter < TAGSize); loopcounter++)
  {

    *((uint8_t*)tagaddr+loopcounter) = *((uint8_t*)temptag+loopcounter);
  }


  CRYP_Cmd(DISABLE);

  return status;
}
