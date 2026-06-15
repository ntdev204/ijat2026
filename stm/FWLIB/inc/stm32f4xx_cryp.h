/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        stm32f4xx_cryp.h
 *
 * Description:  stm32f4xx_cryp.h module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#ifndef __STM32F4xx_CRYP_H
#define __STM32F4xx_CRYP_H

#ifdef __cplusplus
 extern "C" {
#endif


#include "stm32f4xx.h"


typedef struct
{
  uint32_t CRYP_AlgoDir;
  uint32_t CRYP_AlgoMode;
  uint32_t CRYP_DataType;
  uint32_t CRYP_KeySize;
}CRYP_InitTypeDef;


typedef struct
{
  uint32_t CRYP_Key0Left;
  uint32_t CRYP_Key0Right;
  uint32_t CRYP_Key1Left;
  uint32_t CRYP_Key1Right;
  uint32_t CRYP_Key2Left;
  uint32_t CRYP_Key2Right;
  uint32_t CRYP_Key3Left;
  uint32_t CRYP_Key3Right;
}CRYP_KeyInitTypeDef;

typedef struct
{
  uint32_t CRYP_IV0Left;
  uint32_t CRYP_IV0Right;
  uint32_t CRYP_IV1Left;
  uint32_t CRYP_IV1Right;
}CRYP_IVInitTypeDef;


typedef struct
{

  uint32_t CR_CurrentConfig;

  uint32_t CRYP_IV0LR;
  uint32_t CRYP_IV0RR;
  uint32_t CRYP_IV1LR;
  uint32_t CRYP_IV1RR;

  uint32_t CRYP_K0LR;
  uint32_t CRYP_K0RR;
  uint32_t CRYP_K1LR;
  uint32_t CRYP_K1RR;
  uint32_t CRYP_K2LR;
  uint32_t CRYP_K2RR;
  uint32_t CRYP_K3LR;
  uint32_t CRYP_K3RR;
  uint32_t CRYP_CSGCMCCMR[8];
  uint32_t CRYP_CSGCMR[8];
}CRYP_Context;


#define CRYP_AlgoDir_Encrypt      ((uint16_t)0x0000)
#define CRYP_AlgoDir_Decrypt      ((uint16_t)0x0004)
#define IS_CRYP_ALGODIR(ALGODIR) (((ALGODIR) == CRYP_AlgoDir_Encrypt) || \
                                  ((ALGODIR) == CRYP_AlgoDir_Decrypt))


#define CRYP_AlgoMode_TDES_ECB    ((uint32_t)0x00000000)
#define CRYP_AlgoMode_TDES_CBC    ((uint32_t)0x00000008)


#define CRYP_AlgoMode_DES_ECB     ((uint32_t)0x00000010)
#define CRYP_AlgoMode_DES_CBC     ((uint32_t)0x00000018)


#define CRYP_AlgoMode_AES_ECB     ((uint32_t)0x00000020)
#define CRYP_AlgoMode_AES_CBC     ((uint32_t)0x00000028)
#define CRYP_AlgoMode_AES_CTR     ((uint32_t)0x00000030)
#define CRYP_AlgoMode_AES_Key     ((uint32_t)0x00000038)
#define CRYP_AlgoMode_AES_GCM     ((uint32_t)0x00080000)
#define CRYP_AlgoMode_AES_CCM     ((uint32_t)0x00080008)

#define IS_CRYP_ALGOMODE(ALGOMODE) (((ALGOMODE) == CRYP_AlgoMode_TDES_ECB) || \
                                   ((ALGOMODE) == CRYP_AlgoMode_TDES_CBC)|| \
                                   ((ALGOMODE) == CRYP_AlgoMode_DES_ECB) || \
                                   ((ALGOMODE) == CRYP_AlgoMode_DES_CBC) || \
                                   ((ALGOMODE) == CRYP_AlgoMode_AES_ECB) || \
                                   ((ALGOMODE) == CRYP_AlgoMode_AES_CBC) || \
                                   ((ALGOMODE) == CRYP_AlgoMode_AES_CTR) || \
                                   ((ALGOMODE) == CRYP_AlgoMode_AES_Key) || \
                                   ((ALGOMODE) == CRYP_AlgoMode_AES_GCM) || \
                                   ((ALGOMODE) == CRYP_AlgoMode_AES_CCM))


#define CRYP_Phase_Init           ((uint32_t)0x00000000)
#define CRYP_Phase_Header         CRYP_CR_GCM_CCMPH_0
#define CRYP_Phase_Payload        CRYP_CR_GCM_CCMPH_1
#define CRYP_Phase_Final          CRYP_CR_GCM_CCMPH

#define IS_CRYP_PHASE(PHASE) (((PHASE) == CRYP_Phase_Init)    || \
                              ((PHASE) == CRYP_Phase_Header)  || \
                              ((PHASE) == CRYP_Phase_Payload) || \
                              ((PHASE) == CRYP_Phase_Final))


#define CRYP_DataType_32b         ((uint16_t)0x0000)
#define CRYP_DataType_16b         ((uint16_t)0x0040)
#define CRYP_DataType_8b          ((uint16_t)0x0080)
#define CRYP_DataType_1b          ((uint16_t)0x00C0)
#define IS_CRYP_DATATYPE(DATATYPE) (((DATATYPE) == CRYP_DataType_32b) || \
                                    ((DATATYPE) == CRYP_DataType_16b)|| \
                                    ((DATATYPE) == CRYP_DataType_8b)|| \
                                    ((DATATYPE) == CRYP_DataType_1b))


#define CRYP_KeySize_128b         ((uint16_t)0x0000)
#define CRYP_KeySize_192b         ((uint16_t)0x0100)
#define CRYP_KeySize_256b         ((uint16_t)0x0200)
#define IS_CRYP_KEYSIZE(KEYSIZE) (((KEYSIZE) == CRYP_KeySize_128b)|| \
                                  ((KEYSIZE) == CRYP_KeySize_192b)|| \
                                  ((KEYSIZE) == CRYP_KeySize_256b))


#define CRYP_FLAG_BUSY            ((uint8_t)0x10)
#define CRYP_FLAG_IFEM            ((uint8_t)0x01)
#define CRYP_FLAG_IFNF            ((uint8_t)0x02)
#define CRYP_FLAG_INRIS           ((uint8_t)0x22)
#define CRYP_FLAG_OFNE            ((uint8_t)0x04)
#define CRYP_FLAG_OFFU            ((uint8_t)0x08)
#define CRYP_FLAG_OUTRIS          ((uint8_t)0x21)

#define IS_CRYP_GET_FLAG(FLAG) (((FLAG) == CRYP_FLAG_IFEM)  || \
                                ((FLAG) == CRYP_FLAG_IFNF)  || \
                                ((FLAG) == CRYP_FLAG_OFNE)  || \
                                ((FLAG) == CRYP_FLAG_OFFU)  || \
                                ((FLAG) == CRYP_FLAG_BUSY)  || \
                                ((FLAG) == CRYP_FLAG_OUTRIS)|| \
                                ((FLAG) == CRYP_FLAG_INRIS))


#define CRYP_IT_INI               ((uint8_t)0x01)
#define CRYP_IT_OUTI              ((uint8_t)0x02)
#define IS_CRYP_CONFIG_IT(IT) ((((IT) & (uint8_t)0xFC) == 0x00) && ((IT) != 0x00))
#define IS_CRYP_GET_IT(IT) (((IT) == CRYP_IT_INI) || ((IT) == CRYP_IT_OUTI))


#define MODE_ENCRYPT             ((uint8_t)0x01)
#define MODE_DECRYPT             ((uint8_t)0x00)


#define CRYP_DMAReq_DataIN             ((uint8_t)0x01)
#define CRYP_DMAReq_DataOUT            ((uint8_t)0x02)
#define IS_CRYP_DMAREQ(DMAREQ) ((((DMAREQ) & (uint8_t)0xFC) == 0x00) && ((DMAREQ) != 0x00))


void CRYP_DeInit(void);


void CRYP_Init(CRYP_InitTypeDef* CRYP_InitStruct);
void CRYP_StructInit(CRYP_InitTypeDef* CRYP_InitStruct);
void CRYP_KeyInit(CRYP_KeyInitTypeDef* CRYP_KeyInitStruct);
void CRYP_KeyStructInit(CRYP_KeyInitTypeDef* CRYP_KeyInitStruct);
void CRYP_IVInit(CRYP_IVInitTypeDef* CRYP_IVInitStruct);
void CRYP_IVStructInit(CRYP_IVInitTypeDef* CRYP_IVInitStruct);
void CRYP_Cmd(FunctionalState NewState);
void CRYP_PhaseConfig(uint32_t CRYP_Phase);
void CRYP_FIFOFlush(void);

void CRYP_DataIn(uint32_t Data);
uint32_t CRYP_DataOut(void);


ErrorStatus CRYP_SaveContext(CRYP_Context* CRYP_ContextSave,
                             CRYP_KeyInitTypeDef* CRYP_KeyInitStruct);
void CRYP_RestoreContext(CRYP_Context* CRYP_ContextRestore);


void CRYP_DMACmd(uint8_t CRYP_DMAReq, FunctionalState NewState);


void CRYP_ITConfig(uint8_t CRYP_IT, FunctionalState NewState);
ITStatus CRYP_GetITStatus(uint8_t CRYP_IT);
FunctionalState CRYP_GetCmdStatus(void);
FlagStatus CRYP_GetFlagStatus(uint8_t CRYP_FLAG);


ErrorStatus CRYP_AES_ECB(uint8_t Mode,
                         uint8_t *Key, uint16_t Keysize,
                         uint8_t *Input, uint32_t Ilength,
                         uint8_t *Output);

ErrorStatus CRYP_AES_CBC(uint8_t Mode,
                         uint8_t InitVectors[16],
                         uint8_t *Key, uint16_t Keysize,
                         uint8_t *Input, uint32_t Ilength,
                         uint8_t *Output);

ErrorStatus CRYP_AES_CTR(uint8_t Mode,
                         uint8_t InitVectors[16],
                         uint8_t *Key, uint16_t Keysize,
                         uint8_t *Input, uint32_t Ilength,
                         uint8_t *Output);

ErrorStatus CRYP_AES_GCM(uint8_t Mode, uint8_t InitVectors[16],
                         uint8_t *Key, uint16_t Keysize,
                         uint8_t *Input, uint32_t ILength,
                         uint8_t *Header, uint32_t HLength,
                         uint8_t *Output, uint8_t *AuthTAG);

ErrorStatus CRYP_AES_CCM(uint8_t Mode,
                         uint8_t* Nonce, uint32_t NonceSize,
                         uint8_t* Key, uint16_t Keysize,
                         uint8_t* Input, uint32_t ILength,
                         uint8_t* Header, uint32_t HLength, uint8_t *HBuffer,
                         uint8_t* Output,
                         uint8_t* AuthTAG, uint32_t TAGSize);


ErrorStatus CRYP_TDES_ECB(uint8_t Mode,
                           uint8_t Key[24],
                           uint8_t *Input, uint32_t Ilength,
                           uint8_t *Output);

ErrorStatus CRYP_TDES_CBC(uint8_t Mode,
                          uint8_t Key[24],
                          uint8_t InitVectors[8],
                          uint8_t *Input, uint32_t Ilength,
                          uint8_t *Output);


ErrorStatus CRYP_DES_ECB(uint8_t Mode,
                         uint8_t Key[8],
                         uint8_t *Input, uint32_t Ilength,
                         uint8_t *Output);

ErrorStatus CRYP_DES_CBC(uint8_t Mode,
                         uint8_t Key[8],
                         uint8_t InitVectors[8],
                         uint8_t *Input,uint32_t Ilength,
                         uint8_t *Output);

#ifdef __cplusplus
}
#endif

#endif
