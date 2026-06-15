/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        stm32f4xx_hash_md5.c
 *
 * Description:  stm32f4xx_hash_md5.c module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#include "stm32f4xx_hash.h"


#define MD5BUSY_TIMEOUT    ((uint32_t) 0x00010000)


ErrorStatus HASH_MD5(uint8_t *Input, uint32_t Ilen, uint8_t Output[16])
{
  HASH_InitTypeDef MD5_HASH_InitStructure;
  HASH_MsgDigest MD5_MessageDigest;
  __IO uint16_t nbvalidbitsdata = 0;
  uint32_t i = 0;
  __IO uint32_t counter = 0;
  uint32_t busystatus = 0;
  ErrorStatus status = SUCCESS;
  uint32_t inputaddr  = (uint32_t)Input;
  uint32_t outputaddr = (uint32_t)Output;


  nbvalidbitsdata = 8 * (Ilen % 4);


  HASH_DeInit();


  MD5_HASH_InitStructure.HASH_AlgoSelection = HASH_AlgoSelection_MD5;
  MD5_HASH_InitStructure.HASH_AlgoMode = HASH_AlgoMode_HASH;
  MD5_HASH_InitStructure.HASH_DataType = HASH_DataType_8b;
  HASH_Init(&MD5_HASH_InitStructure);


  HASH_SetLastWordValidBitsNbr(nbvalidbitsdata);


  for(i=0; i<Ilen; i+=4)
  {
    HASH_DataIn(*(uint32_t*)inputaddr);
    inputaddr+=4;
  }


  HASH_StartDigest();


  do
  {
    busystatus = HASH_GetFlagStatus(HASH_FLAG_BUSY);
    counter++;
  }while ((counter != MD5BUSY_TIMEOUT) && (busystatus != RESET));

  if (busystatus != RESET)
  {
     status = ERROR;
  }
  else
  {

    HASH_GetDigest(&MD5_MessageDigest);
    *(uint32_t*)(outputaddr)  = __REV(MD5_MessageDigest.Data[0]);
    outputaddr+=4;
    *(uint32_t*)(outputaddr)  = __REV(MD5_MessageDigest.Data[1]);
    outputaddr+=4;
    *(uint32_t*)(outputaddr)  = __REV(MD5_MessageDigest.Data[2]);
    outputaddr+=4;
    *(uint32_t*)(outputaddr)  = __REV(MD5_MessageDigest.Data[3]);
  }
  return status;
}


ErrorStatus HMAC_MD5(uint8_t *Key, uint32_t Keylen, uint8_t *Input,
                     uint32_t Ilen, uint8_t Output[16])
{
  HASH_InitTypeDef MD5_HASH_InitStructure;
  HASH_MsgDigest MD5_MessageDigest;
  __IO uint16_t nbvalidbitsdata = 0;
  __IO uint16_t nbvalidbitskey = 0;
  uint32_t i = 0;
  __IO uint32_t counter = 0;
  uint32_t busystatus = 0;
  ErrorStatus status = SUCCESS;
  uint32_t keyaddr    = (uint32_t)Key;
  uint32_t inputaddr  = (uint32_t)Input;
  uint32_t outputaddr = (uint32_t)Output;


  nbvalidbitsdata = 8 * (Ilen % 4);


  nbvalidbitskey = 8 * (Keylen % 4);


  HASH_DeInit();


  MD5_HASH_InitStructure.HASH_AlgoSelection = HASH_AlgoSelection_MD5;
  MD5_HASH_InitStructure.HASH_AlgoMode = HASH_AlgoMode_HMAC;
  MD5_HASH_InitStructure.HASH_DataType = HASH_DataType_8b;
  if(Keylen > 64)
  {

    MD5_HASH_InitStructure.HASH_HMACKeyType = HASH_HMACKeyType_LongKey;
  }
  else
  {

    MD5_HASH_InitStructure.HASH_HMACKeyType = HASH_HMACKeyType_ShortKey;
  }
  HASH_Init(&MD5_HASH_InitStructure);


  HASH_SetLastWordValidBitsNbr(nbvalidbitskey);


  for(i=0; i<Keylen; i+=4)
  {
    HASH_DataIn(*(uint32_t*)keyaddr);
    keyaddr+=4;
  }


  HASH_StartDigest();


  do
  {
    busystatus = HASH_GetFlagStatus(HASH_FLAG_BUSY);
    counter++;
  }while ((counter != MD5BUSY_TIMEOUT) && (busystatus != RESET));

  if (busystatus != RESET)
  {
     status = ERROR;
  }
  else
  {

    HASH_SetLastWordValidBitsNbr(nbvalidbitsdata);


    for(i=0; i<Ilen; i+=4)
    {
      HASH_DataIn(*(uint32_t*)inputaddr);
      inputaddr+=4;
    }


    HASH_StartDigest();


    counter =0;
    do
    {
       busystatus = HASH_GetFlagStatus(HASH_FLAG_BUSY);
       counter++;
    }while ((counter != MD5BUSY_TIMEOUT) && (busystatus != RESET));

    if (busystatus != RESET)
    {
      status = ERROR;
    }
    else
    {

      HASH_SetLastWordValidBitsNbr(nbvalidbitskey);


      keyaddr = (uint32_t)Key;
      for(i=0; i<Keylen; i+=4)
      {
        HASH_DataIn(*(uint32_t*)keyaddr);
        keyaddr+=4;
      }


       HASH_StartDigest();


       counter =0;
       do
       {
          busystatus = HASH_GetFlagStatus(HASH_FLAG_BUSY);
          counter++;
      }while ((counter != MD5BUSY_TIMEOUT) && (busystatus != RESET));

      if (busystatus != RESET)
      {
         status = ERROR;
      }
      else
      {

         HASH_GetDigest(&MD5_MessageDigest);
         *(uint32_t*)(outputaddr)  = __REV(MD5_MessageDigest.Data[0]);
         outputaddr+=4;
         *(uint32_t*)(outputaddr)  = __REV(MD5_MessageDigest.Data[1]);
         outputaddr+=4;
         *(uint32_t*)(outputaddr)  = __REV(MD5_MessageDigest.Data[2]);
         outputaddr+=4;
         *(uint32_t*)(outputaddr)  = __REV(MD5_MessageDigest.Data[3]);
      }
    }
  }
  return status;
}
