/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        stm32f4xx_cryp.c
 *
 * Description:  stm32f4xx_cryp.c module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#include "stm32f4xx_cryp.h"
#include "stm32f4xx_rcc.h"


#define FLAG_MASK     ((uint8_t)0x20)
#define MAX_TIMEOUT   ((uint16_t)0xFFFF)


void CRYP_DeInit(void)
{

  RCC_AHB2PeriphResetCmd(RCC_AHB2Periph_CRYP, ENABLE);


  RCC_AHB2PeriphResetCmd(RCC_AHB2Periph_CRYP, DISABLE);
}


void CRYP_Init(CRYP_InitTypeDef* CRYP_InitStruct)
{

  assert_param(IS_CRYP_ALGOMODE(CRYP_InitStruct->CRYP_AlgoMode));
  assert_param(IS_CRYP_DATATYPE(CRYP_InitStruct->CRYP_DataType));
  assert_param(IS_CRYP_ALGODIR(CRYP_InitStruct->CRYP_AlgoDir));


  CRYP->CR &= ~CRYP_CR_ALGOMODE;
  CRYP->CR |= CRYP_InitStruct->CRYP_AlgoMode;


  CRYP->CR &= ~CRYP_CR_DATATYPE;
  CRYP->CR |= CRYP_InitStruct->CRYP_DataType;


  if ((CRYP_InitStruct->CRYP_AlgoMode != CRYP_AlgoMode_TDES_ECB) &&
      (CRYP_InitStruct->CRYP_AlgoMode != CRYP_AlgoMode_TDES_CBC) &&
      (CRYP_InitStruct->CRYP_AlgoMode != CRYP_AlgoMode_DES_ECB) &&
      (CRYP_InitStruct->CRYP_AlgoMode != CRYP_AlgoMode_DES_CBC))
  {
    assert_param(IS_CRYP_KEYSIZE(CRYP_InitStruct->CRYP_KeySize));
    CRYP->CR &= ~CRYP_CR_KEYSIZE;
    CRYP->CR |= CRYP_InitStruct->CRYP_KeySize;
  }


  CRYP->CR &= ~CRYP_CR_ALGODIR;
  CRYP->CR |= CRYP_InitStruct->CRYP_AlgoDir;
}


void CRYP_StructInit(CRYP_InitTypeDef* CRYP_InitStruct)
{

  CRYP_InitStruct->CRYP_AlgoDir = CRYP_AlgoDir_Encrypt;


  CRYP_InitStruct->CRYP_AlgoMode = CRYP_AlgoMode_TDES_ECB;


  CRYP_InitStruct->CRYP_DataType = CRYP_DataType_32b;


  CRYP_InitStruct->CRYP_KeySize = CRYP_KeySize_128b;
}


void CRYP_KeyInit(CRYP_KeyInitTypeDef* CRYP_KeyInitStruct)
{

  CRYP->K0LR = CRYP_KeyInitStruct->CRYP_Key0Left;
  CRYP->K0RR = CRYP_KeyInitStruct->CRYP_Key0Right;
  CRYP->K1LR = CRYP_KeyInitStruct->CRYP_Key1Left;
  CRYP->K1RR = CRYP_KeyInitStruct->CRYP_Key1Right;
  CRYP->K2LR = CRYP_KeyInitStruct->CRYP_Key2Left;
  CRYP->K2RR = CRYP_KeyInitStruct->CRYP_Key2Right;
  CRYP->K3LR = CRYP_KeyInitStruct->CRYP_Key3Left;
  CRYP->K3RR = CRYP_KeyInitStruct->CRYP_Key3Right;
}


void CRYP_KeyStructInit(CRYP_KeyInitTypeDef* CRYP_KeyInitStruct)
{
  CRYP_KeyInitStruct->CRYP_Key0Left  = 0;
  CRYP_KeyInitStruct->CRYP_Key0Right = 0;
  CRYP_KeyInitStruct->CRYP_Key1Left  = 0;
  CRYP_KeyInitStruct->CRYP_Key1Right = 0;
  CRYP_KeyInitStruct->CRYP_Key2Left  = 0;
  CRYP_KeyInitStruct->CRYP_Key2Right = 0;
  CRYP_KeyInitStruct->CRYP_Key3Left  = 0;
  CRYP_KeyInitStruct->CRYP_Key3Right = 0;
}

void CRYP_IVInit(CRYP_IVInitTypeDef* CRYP_IVInitStruct)
{
  CRYP->IV0LR = CRYP_IVInitStruct->CRYP_IV0Left;
  CRYP->IV0RR = CRYP_IVInitStruct->CRYP_IV0Right;
  CRYP->IV1LR = CRYP_IVInitStruct->CRYP_IV1Left;
  CRYP->IV1RR = CRYP_IVInitStruct->CRYP_IV1Right;
}


void CRYP_IVStructInit(CRYP_IVInitTypeDef* CRYP_IVInitStruct)
{
  CRYP_IVInitStruct->CRYP_IV0Left  = 0;
  CRYP_IVInitStruct->CRYP_IV0Right = 0;
  CRYP_IVInitStruct->CRYP_IV1Left  = 0;
  CRYP_IVInitStruct->CRYP_IV1Right = 0;
}


void CRYP_PhaseConfig(uint32_t CRYP_Phase)
{ uint32_t tempcr = 0;


  assert_param(IS_CRYP_PHASE(CRYP_Phase));


  tempcr = CRYP->CR;


  tempcr &= (uint32_t)(~CRYP_CR_GCM_CCMPH);

  tempcr |= (uint32_t)CRYP_Phase;


  CRYP->CR = tempcr;
}


void CRYP_FIFOFlush(void)
{

  CRYP->CR |= CRYP_CR_FFLUSH;
}


void CRYP_Cmd(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    CRYP->CR |= CRYP_CR_CRYPEN;
  }
  else
  {

    CRYP->CR &= ~CRYP_CR_CRYPEN;
  }
}


void CRYP_DataIn(uint32_t Data)
{
  CRYP->DR = Data;
}


uint32_t CRYP_DataOut(void)
{
  return CRYP->DOUT;
}


ErrorStatus CRYP_SaveContext(CRYP_Context* CRYP_ContextSave,
                             CRYP_KeyInitTypeDef* CRYP_KeyInitStruct)
{
  __IO uint32_t timeout = 0;
  uint32_t ckeckmask = 0, bitstatus;
  ErrorStatus status = ERROR;


  CRYP->DMACR &= ~(uint32_t)CRYP_DMACR_DIEN;


  if ((CRYP->CR & (uint32_t)(CRYP_CR_ALGOMODE_TDES_ECB | CRYP_CR_ALGOMODE_TDES_CBC)) != (uint32_t)0 )
  {
    ckeckmask =  CRYP_SR_IFEM | CRYP_SR_BUSY ;
  }
  else
  {
    ckeckmask =  CRYP_SR_IFEM | CRYP_SR_BUSY | CRYP_SR_OFNE;
  }

  do
  {
    bitstatus = CRYP->SR & ckeckmask;
    timeout++;
  }
  while ((timeout != MAX_TIMEOUT) && (bitstatus != CRYP_SR_IFEM));

  if ((CRYP->SR & ckeckmask) != CRYP_SR_IFEM)
  {
    status = ERROR;
  }
  else
  {


    CRYP->DMACR &= ~(uint32_t)CRYP_DMACR_DOEN;
    CRYP->CR &= ~(uint32_t)CRYP_CR_CRYPEN;


    CRYP_ContextSave->CR_CurrentConfig  = CRYP->CR & (CRYP_CR_GCM_CCMPH |
                                                      CRYP_CR_KEYSIZE  |
                                                      CRYP_CR_DATATYPE |
                                                      CRYP_CR_ALGOMODE |
                                                      CRYP_CR_ALGODIR);


    CRYP_ContextSave->CRYP_IV0LR = CRYP->IV0LR;
    CRYP_ContextSave->CRYP_IV0RR = CRYP->IV0RR;
    CRYP_ContextSave->CRYP_IV1LR = CRYP->IV1LR;
    CRYP_ContextSave->CRYP_IV1RR = CRYP->IV1RR;


    CRYP_ContextSave->CRYP_K0LR = CRYP_KeyInitStruct->CRYP_Key0Left;
    CRYP_ContextSave->CRYP_K0RR = CRYP_KeyInitStruct->CRYP_Key0Right;
    CRYP_ContextSave->CRYP_K1LR = CRYP_KeyInitStruct->CRYP_Key1Left;
    CRYP_ContextSave->CRYP_K1RR = CRYP_KeyInitStruct->CRYP_Key1Right;
    CRYP_ContextSave->CRYP_K2LR = CRYP_KeyInitStruct->CRYP_Key2Left;
    CRYP_ContextSave->CRYP_K2RR = CRYP_KeyInitStruct->CRYP_Key2Right;
    CRYP_ContextSave->CRYP_K3LR = CRYP_KeyInitStruct->CRYP_Key3Left;
    CRYP_ContextSave->CRYP_K3RR = CRYP_KeyInitStruct->CRYP_Key3Right;


    CRYP_ContextSave->CRYP_CSGCMCCMR[0] = CRYP->CSGCMCCM0R;
    CRYP_ContextSave->CRYP_CSGCMCCMR[1] = CRYP->CSGCMCCM1R;
    CRYP_ContextSave->CRYP_CSGCMCCMR[2] = CRYP->CSGCMCCM2R;
    CRYP_ContextSave->CRYP_CSGCMCCMR[3] = CRYP->CSGCMCCM3R;
    CRYP_ContextSave->CRYP_CSGCMCCMR[4] = CRYP->CSGCMCCM4R;
    CRYP_ContextSave->CRYP_CSGCMCCMR[5] = CRYP->CSGCMCCM5R;
    CRYP_ContextSave->CRYP_CSGCMCCMR[6] = CRYP->CSGCMCCM6R;
    CRYP_ContextSave->CRYP_CSGCMCCMR[7] = CRYP->CSGCMCCM7R;

    CRYP_ContextSave->CRYP_CSGCMR[0] = CRYP->CSGCM0R;
    CRYP_ContextSave->CRYP_CSGCMR[1] = CRYP->CSGCM1R;
    CRYP_ContextSave->CRYP_CSGCMR[2] = CRYP->CSGCM2R;
    CRYP_ContextSave->CRYP_CSGCMR[3] = CRYP->CSGCM3R;
    CRYP_ContextSave->CRYP_CSGCMR[4] = CRYP->CSGCM4R;
    CRYP_ContextSave->CRYP_CSGCMR[5] = CRYP->CSGCM5R;
    CRYP_ContextSave->CRYP_CSGCMR[6] = CRYP->CSGCM6R;
    CRYP_ContextSave->CRYP_CSGCMR[7] = CRYP->CSGCM7R;


    status = SUCCESS;
  }

   return status;
}


void CRYP_RestoreContext(CRYP_Context* CRYP_ContextRestore)
{


  CRYP->CR = CRYP_ContextRestore->CR_CurrentConfig;


  CRYP->K0LR = CRYP_ContextRestore->CRYP_K0LR;
  CRYP->K0RR = CRYP_ContextRestore->CRYP_K0RR;
  CRYP->K1LR = CRYP_ContextRestore->CRYP_K1LR;
  CRYP->K1RR = CRYP_ContextRestore->CRYP_K1RR;
  CRYP->K2LR = CRYP_ContextRestore->CRYP_K2LR;
  CRYP->K2RR = CRYP_ContextRestore->CRYP_K2RR;
  CRYP->K3LR = CRYP_ContextRestore->CRYP_K3LR;
  CRYP->K3RR = CRYP_ContextRestore->CRYP_K3RR;


  CRYP->IV0LR = CRYP_ContextRestore->CRYP_IV0LR;
  CRYP->IV0RR = CRYP_ContextRestore->CRYP_IV0RR;
  CRYP->IV1LR = CRYP_ContextRestore->CRYP_IV1LR;
  CRYP->IV1RR = CRYP_ContextRestore->CRYP_IV1RR;


  CRYP->CSGCMCCM0R = CRYP_ContextRestore->CRYP_CSGCMCCMR[0];
  CRYP->CSGCMCCM1R = CRYP_ContextRestore->CRYP_CSGCMCCMR[1];
  CRYP->CSGCMCCM2R = CRYP_ContextRestore->CRYP_CSGCMCCMR[2];
  CRYP->CSGCMCCM3R = CRYP_ContextRestore->CRYP_CSGCMCCMR[3];
  CRYP->CSGCMCCM4R = CRYP_ContextRestore->CRYP_CSGCMCCMR[4];
  CRYP->CSGCMCCM5R = CRYP_ContextRestore->CRYP_CSGCMCCMR[5];
  CRYP->CSGCMCCM6R = CRYP_ContextRestore->CRYP_CSGCMCCMR[6];
  CRYP->CSGCMCCM7R = CRYP_ContextRestore->CRYP_CSGCMCCMR[7];

  CRYP->CSGCM0R = CRYP_ContextRestore->CRYP_CSGCMR[0];
  CRYP->CSGCM1R = CRYP_ContextRestore->CRYP_CSGCMR[1];
  CRYP->CSGCM2R = CRYP_ContextRestore->CRYP_CSGCMR[2];
  CRYP->CSGCM3R = CRYP_ContextRestore->CRYP_CSGCMR[3];
  CRYP->CSGCM4R = CRYP_ContextRestore->CRYP_CSGCMR[4];
  CRYP->CSGCM5R = CRYP_ContextRestore->CRYP_CSGCMR[5];
  CRYP->CSGCM6R = CRYP_ContextRestore->CRYP_CSGCMR[6];
  CRYP->CSGCM7R = CRYP_ContextRestore->CRYP_CSGCMR[7];


  CRYP->CR |= CRYP_CR_CRYPEN;
}


void CRYP_DMACmd(uint8_t CRYP_DMAReq, FunctionalState NewState)
{

  assert_param(IS_CRYP_DMAREQ(CRYP_DMAReq));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    CRYP->DMACR |= CRYP_DMAReq;
  }
  else
  {

    CRYP->DMACR &= (uint8_t)~CRYP_DMAReq;
  }
}


void CRYP_ITConfig(uint8_t CRYP_IT, FunctionalState NewState)
{

  assert_param(IS_CRYP_CONFIG_IT(CRYP_IT));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    CRYP->IMSCR |= CRYP_IT;
  }
  else
  {

    CRYP->IMSCR &= (uint8_t)~CRYP_IT;
  }
}


ITStatus CRYP_GetITStatus(uint8_t CRYP_IT)
{
  ITStatus bitstatus = RESET;

  assert_param(IS_CRYP_GET_IT(CRYP_IT));


  if ((CRYP->MISR &  CRYP_IT) != (uint8_t)RESET)
  {

    bitstatus = SET;
  }
  else
  {

    bitstatus = RESET;
  }

  return bitstatus;
}


FunctionalState CRYP_GetCmdStatus(void)
{
  FunctionalState state = DISABLE;

  if ((CRYP->CR & CRYP_CR_CRYPEN) != 0)
  {

    state = ENABLE;
  }
  else
  {

    state = DISABLE;
  }
  return state;
}


FlagStatus CRYP_GetFlagStatus(uint8_t CRYP_FLAG)
{
  FlagStatus bitstatus = RESET;
  uint32_t tempreg = 0;


  assert_param(IS_CRYP_GET_FLAG(CRYP_FLAG));


  if ((CRYP_FLAG & FLAG_MASK) != 0x00)
  {
    tempreg = CRYP->RISR;
  }
  else
  {
    tempreg = CRYP->SR;
  }


  if ((tempreg & CRYP_FLAG ) != (uint8_t)RESET)
  {

    bitstatus = SET;
  }
  else
  {

    bitstatus = RESET;
  }


  return  bitstatus;
}
