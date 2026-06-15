/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        stm32f4xx_hash.h
 *
 * Description:  stm32f4xx_hash.h module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#ifndef __STM32F4xx_HASH_H
#define __STM32F4xx_HASH_H

#ifdef __cplusplus
 extern "C" {
#endif


#include "stm32f4xx.h"


typedef struct
{
  uint32_t HASH_AlgoSelection;
  uint32_t HASH_AlgoMode;
  uint32_t HASH_DataType;
  uint32_t HASH_HMACKeyType;
}HASH_InitTypeDef;


typedef struct
{
  uint32_t Data[8];
} HASH_MsgDigest;


typedef struct
{
  uint32_t HASH_IMR;
  uint32_t HASH_STR;
  uint32_t HASH_CR;
  uint32_t HASH_CSR[54];
}HASH_Context;


#define HASH_AlgoSelection_SHA1      ((uint32_t)0x0000)
#define HASH_AlgoSelection_SHA224    HASH_CR_ALGO_1
#define HASH_AlgoSelection_SHA256    HASH_CR_ALGO
#define HASH_AlgoSelection_MD5       HASH_CR_ALGO_0

#define IS_HASH_ALGOSELECTION(ALGOSELECTION) (((ALGOSELECTION) == HASH_AlgoSelection_SHA1) || \
                                              ((ALGOSELECTION) == HASH_AlgoSelection_SHA224) || \
                                              ((ALGOSELECTION) == HASH_AlgoSelection_SHA256) || \
                                              ((ALGOSELECTION) == HASH_AlgoSelection_MD5))


#define HASH_AlgoMode_HASH         ((uint32_t)0x00000000)
#define HASH_AlgoMode_HMAC         HASH_CR_MODE

#define IS_HASH_ALGOMODE(ALGOMODE) (((ALGOMODE) == HASH_AlgoMode_HASH) || \
                                    ((ALGOMODE) == HASH_AlgoMode_HMAC))


#define HASH_DataType_32b          ((uint32_t)0x0000)
#define HASH_DataType_16b          HASH_CR_DATATYPE_0
#define HASH_DataType_8b           HASH_CR_DATATYPE_1
#define HASH_DataType_1b           HASH_CR_DATATYPE

#define IS_HASH_DATATYPE(DATATYPE) (((DATATYPE) == HASH_DataType_32b)|| \
                                    ((DATATYPE) == HASH_DataType_16b)|| \
                                    ((DATATYPE) == HASH_DataType_8b) || \
                                    ((DATATYPE) == HASH_DataType_1b))


#define HASH_HMACKeyType_ShortKey      ((uint32_t)0x00000000)
#define HASH_HMACKeyType_LongKey       HASH_CR_LKEY

#define IS_HASH_HMAC_KEYTYPE(KEYTYPE) (((KEYTYPE) == HASH_HMACKeyType_ShortKey) || \
                                       ((KEYTYPE) == HASH_HMACKeyType_LongKey))


#define IS_HASH_VALIDBITSNUMBER(VALIDBITS) ((VALIDBITS) <= 0x1F)


#define HASH_IT_DINI               HASH_IMR_DINIM
#define HASH_IT_DCI                HASH_IMR_DCIM

#define IS_HASH_IT(IT) ((((IT) & (uint32_t)0xFFFFFFFC) == 0x00000000) && ((IT) != 0x00000000))
#define IS_HASH_GET_IT(IT) (((IT) == HASH_IT_DINI) || ((IT) == HASH_IT_DCI))


#define HASH_FLAG_DINIS            HASH_SR_DINIS
#define HASH_FLAG_DCIS             HASH_SR_DCIS
#define HASH_FLAG_DMAS             HASH_SR_DMAS
#define HASH_FLAG_BUSY             HASH_SR_BUSY
#define HASH_FLAG_DINNE            HASH_CR_DINNE

#define IS_HASH_GET_FLAG(FLAG) (((FLAG) == HASH_FLAG_DINIS) || \
                                ((FLAG) == HASH_FLAG_DCIS)  || \
                                ((FLAG) == HASH_FLAG_DMAS)  || \
                                ((FLAG) == HASH_FLAG_BUSY)  || \
                                ((FLAG) == HASH_FLAG_DINNE))

#define IS_HASH_CLEAR_FLAG(FLAG)(((FLAG) == HASH_FLAG_DINIS) || \
                                 ((FLAG) == HASH_FLAG_DCIS))


void HASH_DeInit(void);


void HASH_Init(HASH_InitTypeDef* HASH_InitStruct);
void HASH_StructInit(HASH_InitTypeDef* HASH_InitStruct);
void HASH_Reset(void);


void HASH_DataIn(uint32_t Data);
uint8_t HASH_GetInFIFOWordsNbr(void);
void HASH_SetLastWordValidBitsNbr(uint16_t ValidNumber);
void HASH_StartDigest(void);
void HASH_AutoStartDigest(FunctionalState NewState);
void HASH_GetDigest(HASH_MsgDigest* HASH_MessageDigest);


void HASH_SaveContext(HASH_Context* HASH_ContextSave);
void HASH_RestoreContext(HASH_Context* HASH_ContextRestore);


void HASH_DMACmd(FunctionalState NewState);


void HASH_ITConfig(uint32_t HASH_IT, FunctionalState NewState);
FlagStatus HASH_GetFlagStatus(uint32_t HASH_FLAG);
void HASH_ClearFlag(uint32_t HASH_FLAG);
ITStatus HASH_GetITStatus(uint32_t HASH_IT);
void HASH_ClearITPendingBit(uint32_t HASH_IT);


ErrorStatus HASH_SHA1(uint8_t *Input, uint32_t Ilen, uint8_t Output[20]);
ErrorStatus HMAC_SHA1(uint8_t *Key, uint32_t Keylen,
                      uint8_t *Input, uint32_t Ilen,
                      uint8_t Output[20]);


ErrorStatus HASH_MD5(uint8_t *Input, uint32_t Ilen, uint8_t Output[16]);
ErrorStatus HMAC_MD5(uint8_t *Key, uint32_t Keylen,
                     uint8_t *Input, uint32_t Ilen,
                     uint8_t Output[16]);

#ifdef __cplusplus
}
#endif

#endif
