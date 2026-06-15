/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        stm32f4xx_hash.c
 *
 * Description:  stm32f4xx_hash.c module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#include "stm32f4xx_hash.h"
#include "stm32f4xx_rcc.h"


void HASH_DeInit(void)
{

  RCC_AHB2PeriphResetCmd(RCC_AHB2Periph_HASH, ENABLE);

  RCC_AHB2PeriphResetCmd(RCC_AHB2Periph_HASH, DISABLE);
}


void HASH_Init(HASH_InitTypeDef* HASH_InitStruct)
{

  assert_param(IS_HASH_ALGOSELECTION(HASH_InitStruct->HASH_AlgoSelection));
  assert_param(IS_HASH_DATATYPE(HASH_InitStruct->HASH_DataType));
  assert_param(IS_HASH_ALGOMODE(HASH_InitStruct->HASH_AlgoMode));


  HASH->CR &= ~ (HASH_CR_ALGO | HASH_CR_DATATYPE | HASH_CR_MODE);
  HASH->CR |= (HASH_InitStruct->HASH_AlgoSelection | \
               HASH_InitStruct->HASH_DataType | \
               HASH_InitStruct->HASH_AlgoMode);


  if(HASH_InitStruct->HASH_AlgoMode == HASH_AlgoMode_HMAC)
  {
    assert_param(IS_HASH_HMAC_KEYTYPE(HASH_InitStruct->HASH_HMACKeyType));
    HASH->CR &= ~HASH_CR_LKEY;
    HASH->CR |= HASH_InitStruct->HASH_HMACKeyType;
  }


  HASH->CR |= HASH_CR_INIT;
}


void HASH_StructInit(HASH_InitTypeDef* HASH_InitStruct)
{

  HASH_InitStruct->HASH_AlgoSelection = HASH_AlgoSelection_SHA1;


  HASH_InitStruct->HASH_AlgoMode = HASH_AlgoMode_HASH;


  HASH_InitStruct->HASH_DataType = HASH_DataType_32b;


  HASH_InitStruct->HASH_HMACKeyType = HASH_HMACKeyType_ShortKey;
}


void HASH_Reset(void)
{

  HASH->CR |= HASH_CR_INIT;
}


void HASH_SetLastWordValidBitsNbr(uint16_t ValidNumber)
{

  assert_param(IS_HASH_VALIDBITSNUMBER(ValidNumber));


  HASH->STR &= ~(HASH_STR_NBW);
  HASH->STR |= ValidNumber;
}


void HASH_DataIn(uint32_t Data)
{

  HASH->DIN = Data;
}


uint8_t HASH_GetInFIFOWordsNbr(void)
{

  return ((HASH->CR & HASH_CR_NBW) >> 8);
}


void HASH_GetDigest(HASH_MsgDigest* HASH_MessageDigest)
{

  HASH_MessageDigest->Data[0] = HASH->HR[0];
  HASH_MessageDigest->Data[1] = HASH->HR[1];
  HASH_MessageDigest->Data[2] = HASH->HR[2];
  HASH_MessageDigest->Data[3] = HASH->HR[3];
  HASH_MessageDigest->Data[4] = HASH->HR[4];
  HASH_MessageDigest->Data[5] = HASH_DIGEST->HR[5];
  HASH_MessageDigest->Data[6] = HASH_DIGEST->HR[6];
  HASH_MessageDigest->Data[7] = HASH_DIGEST->HR[7];
}


void HASH_StartDigest(void)
{

  HASH->STR |= HASH_STR_DCAL;
}


void HASH_SaveContext(HASH_Context* HASH_ContextSave)
{
  uint8_t i = 0;


  HASH_ContextSave->HASH_IMR = HASH->IMR;
  HASH_ContextSave->HASH_STR = HASH->STR;
  HASH_ContextSave->HASH_CR  = HASH->CR;
  for(i=0; i<=53;i++)
  {
     HASH_ContextSave->HASH_CSR[i] = HASH->CSR[i];
  }
}


void HASH_RestoreContext(HASH_Context* HASH_ContextRestore)
{
  uint8_t i = 0;


  HASH->IMR = HASH_ContextRestore->HASH_IMR;
  HASH->STR = HASH_ContextRestore->HASH_STR;
  HASH->CR = HASH_ContextRestore->HASH_CR;


  HASH->CR |= HASH_CR_INIT;


  for(i=0; i<=53;i++)
  {
     HASH->CSR[i] = HASH_ContextRestore->HASH_CSR[i];
  }
}


void HASH_AutoStartDigest(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    HASH->CR &= ~HASH_CR_MDMAT;
  }
  else
  {

    HASH->CR |= HASH_CR_MDMAT;
  }
}


void HASH_DMACmd(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    HASH->CR |= HASH_CR_DMAE;
  }
  else
  {

    HASH->CR &= ~HASH_CR_DMAE;
  }
}


void HASH_ITConfig(uint32_t HASH_IT, FunctionalState NewState)
{

  assert_param(IS_HASH_IT(HASH_IT));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    HASH->IMR |= HASH_IT;
  }
  else
  {

    HASH->IMR &= (uint32_t)(~HASH_IT);
  }
}


FlagStatus HASH_GetFlagStatus(uint32_t HASH_FLAG)
{
  FlagStatus bitstatus = RESET;
  uint32_t tempreg = 0;


  assert_param(IS_HASH_GET_FLAG(HASH_FLAG));


  if ((HASH_FLAG & HASH_FLAG_DINNE) != (uint32_t)RESET )
  {
    tempreg = HASH->CR;
  }
  else
  {
    tempreg = HASH->SR;
  }


  if ((tempreg & HASH_FLAG) != (uint32_t)RESET)
  {

    bitstatus = SET;
  }
  else
  {

    bitstatus = RESET;
  }


  return  bitstatus;
}

void HASH_ClearFlag(uint32_t HASH_FLAG)
{

  assert_param(IS_HASH_CLEAR_FLAG(HASH_FLAG));


  HASH->SR = ~(uint32_t)HASH_FLAG;
}

ITStatus HASH_GetITStatus(uint32_t HASH_IT)
{
  ITStatus bitstatus = RESET;
  uint32_t tmpreg = 0;


  assert_param(IS_HASH_GET_IT(HASH_IT));


  tmpreg =  HASH->SR;

  if (((HASH->IMR & tmpreg) & HASH_IT) != RESET)
  {

    bitstatus = SET;
  }
  else
  {

    bitstatus = RESET;
  }

  return bitstatus;
}


void HASH_ClearITPendingBit(uint32_t HASH_IT)
{

  assert_param(IS_HASH_IT(HASH_IT));


  HASH->SR = (uint32_t)(~HASH_IT);
}
