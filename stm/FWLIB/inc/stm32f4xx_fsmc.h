/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        stm32f4xx_fsmc.h
 *
 * Description:  stm32f4xx_fsmc.h module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#ifndef __STM32F4xx_FSMC_H
#define __STM32F4xx_FSMC_H

#ifdef __cplusplus
 extern "C" {
#endif


#include "stm32f4xx.h"


typedef struct
{
  uint32_t FSMC_AddressSetupTime;

  uint32_t FSMC_AddressHoldTime;

  uint32_t FSMC_DataSetupTime;

  uint32_t FSMC_BusTurnAroundDuration;

  uint32_t FSMC_CLKDivision;

  uint32_t FSMC_DataLatency;

  uint32_t FSMC_AccessMode;
}FSMC_NORSRAMTimingInitTypeDef;


typedef struct
{
  uint32_t FSMC_Bank;

  uint32_t FSMC_DataAddressMux;

  uint32_t FSMC_MemoryType;

  uint32_t FSMC_MemoryDataWidth;

  uint32_t FSMC_BurstAccessMode;

  uint32_t FSMC_AsynchronousWait;

  uint32_t FSMC_WaitSignalPolarity;

  uint32_t FSMC_WrapMode;

  uint32_t FSMC_WaitSignalActive;

  uint32_t FSMC_WriteOperation;

  uint32_t FSMC_WaitSignal;

  uint32_t FSMC_ExtendedMode;

  uint32_t FSMC_WriteBurst;

  FSMC_NORSRAMTimingInitTypeDef* FSMC_ReadWriteTimingStruct;

  FSMC_NORSRAMTimingInitTypeDef* FSMC_WriteTimingStruct;
}FSMC_NORSRAMInitTypeDef;


typedef struct
{
  uint32_t FSMC_SetupTime;

  uint32_t FSMC_WaitSetupTime;

  uint32_t FSMC_HoldSetupTime;

  uint32_t FSMC_HiZSetupTime;
}FSMC_NAND_PCCARDTimingInitTypeDef;


typedef struct
{
  uint32_t FSMC_Bank;

  uint32_t FSMC_Waitfeature;

  uint32_t FSMC_MemoryDataWidth;

  uint32_t FSMC_ECC;

  uint32_t FSMC_ECCPageSize;

  uint32_t FSMC_TCLRSetupTime;

  uint32_t FSMC_TARSetupTime;

  FSMC_NAND_PCCARDTimingInitTypeDef*  FSMC_CommonSpaceTimingStruct;

  FSMC_NAND_PCCARDTimingInitTypeDef*  FSMC_AttributeSpaceTimingStruct;
}FSMC_NANDInitTypeDef;


typedef struct
{
  uint32_t FSMC_Waitfeature;

  uint32_t FSMC_TCLRSetupTime;

  uint32_t FSMC_TARSetupTime;


  FSMC_NAND_PCCARDTimingInitTypeDef*  FSMC_CommonSpaceTimingStruct;

  FSMC_NAND_PCCARDTimingInitTypeDef*  FSMC_AttributeSpaceTimingStruct;

  FSMC_NAND_PCCARDTimingInitTypeDef*  FSMC_IOSpaceTimingStruct;
}FSMC_PCCARDInitTypeDef;


#define FSMC_Bank1_NORSRAM1                      ((uint32_t)0x00000000)
#define FSMC_Bank1_NORSRAM2                      ((uint32_t)0x00000002)
#define FSMC_Bank1_NORSRAM3                      ((uint32_t)0x00000004)
#define FSMC_Bank1_NORSRAM4                      ((uint32_t)0x00000006)


#define FSMC_Bank2_NAND                          ((uint32_t)0x00000010)
#define FSMC_Bank3_NAND                          ((uint32_t)0x00000100)


#define FSMC_Bank4_PCCARD                        ((uint32_t)0x00001000)


#define IS_FSMC_NORSRAM_BANK(BANK) (((BANK) == FSMC_Bank1_NORSRAM1) || \
                                    ((BANK) == FSMC_Bank1_NORSRAM2) || \
                                    ((BANK) == FSMC_Bank1_NORSRAM3) || \
                                    ((BANK) == FSMC_Bank1_NORSRAM4))

#define IS_FSMC_NAND_BANK(BANK) (((BANK) == FSMC_Bank2_NAND) || \
                                 ((BANK) == FSMC_Bank3_NAND))

#define IS_FSMC_GETFLAG_BANK(BANK) (((BANK) == FSMC_Bank2_NAND) || \
                                    ((BANK) == FSMC_Bank3_NAND) || \
                                    ((BANK) == FSMC_Bank4_PCCARD))

#define IS_FSMC_IT_BANK(BANK) (((BANK) == FSMC_Bank2_NAND) || \
                               ((BANK) == FSMC_Bank3_NAND) || \
                               ((BANK) == FSMC_Bank4_PCCARD))


#define FSMC_DataAddressMux_Disable                ((uint32_t)0x00000000)
#define FSMC_DataAddressMux_Enable                 ((uint32_t)0x00000002)
#define IS_FSMC_MUX(MUX) (((MUX) == FSMC_DataAddressMux_Disable) || \
                          ((MUX) == FSMC_DataAddressMux_Enable))


#define FSMC_MemoryType_SRAM                     ((uint32_t)0x00000000)
#define FSMC_MemoryType_PSRAM                    ((uint32_t)0x00000004)
#define FSMC_MemoryType_NOR                      ((uint32_t)0x00000008)
#define IS_FSMC_MEMORY(MEMORY) (((MEMORY) == FSMC_MemoryType_SRAM) || \
                                ((MEMORY) == FSMC_MemoryType_PSRAM)|| \
                                ((MEMORY) == FSMC_MemoryType_NOR))


#define FSMC_MemoryDataWidth_8b                  ((uint32_t)0x00000000)
#define FSMC_MemoryDataWidth_16b                 ((uint32_t)0x00000010)
#define IS_FSMC_MEMORY_WIDTH(WIDTH) (((WIDTH) == FSMC_MemoryDataWidth_8b) || \
                                     ((WIDTH) == FSMC_MemoryDataWidth_16b))


#define FSMC_BurstAccessMode_Disable             ((uint32_t)0x00000000)
#define FSMC_BurstAccessMode_Enable              ((uint32_t)0x00000100)
#define IS_FSMC_BURSTMODE(STATE) (((STATE) == FSMC_BurstAccessMode_Disable) || \
                                  ((STATE) == FSMC_BurstAccessMode_Enable))


#define FSMC_AsynchronousWait_Disable            ((uint32_t)0x00000000)
#define FSMC_AsynchronousWait_Enable             ((uint32_t)0x00008000)
#define IS_FSMC_ASYNWAIT(STATE) (((STATE) == FSMC_AsynchronousWait_Disable) || \
                                 ((STATE) == FSMC_AsynchronousWait_Enable))


#define FSMC_WaitSignalPolarity_Low              ((uint32_t)0x00000000)
#define FSMC_WaitSignalPolarity_High             ((uint32_t)0x00000200)
#define IS_FSMC_WAIT_POLARITY(POLARITY) (((POLARITY) == FSMC_WaitSignalPolarity_Low) || \
                                         ((POLARITY) == FSMC_WaitSignalPolarity_High))


#define FSMC_WrapMode_Disable                    ((uint32_t)0x00000000)
#define FSMC_WrapMode_Enable                     ((uint32_t)0x00000400)
#define IS_FSMC_WRAP_MODE(MODE) (((MODE) == FSMC_WrapMode_Disable) || \
                                 ((MODE) == FSMC_WrapMode_Enable))


#define FSMC_WaitSignalActive_BeforeWaitState    ((uint32_t)0x00000000)
#define FSMC_WaitSignalActive_DuringWaitState    ((uint32_t)0x00000800)
#define IS_FSMC_WAIT_SIGNAL_ACTIVE(ACTIVE) (((ACTIVE) == FSMC_WaitSignalActive_BeforeWaitState) || \
                                            ((ACTIVE) == FSMC_WaitSignalActive_DuringWaitState))


#define FSMC_WriteOperation_Disable                     ((uint32_t)0x00000000)
#define FSMC_WriteOperation_Enable                      ((uint32_t)0x00001000)
#define IS_FSMC_WRITE_OPERATION(OPERATION) (((OPERATION) == FSMC_WriteOperation_Disable) || \
                                            ((OPERATION) == FSMC_WriteOperation_Enable))


#define FSMC_WaitSignal_Disable                  ((uint32_t)0x00000000)
#define FSMC_WaitSignal_Enable                   ((uint32_t)0x00002000)
#define IS_FSMC_WAITE_SIGNAL(SIGNAL) (((SIGNAL) == FSMC_WaitSignal_Disable) || \
                                      ((SIGNAL) == FSMC_WaitSignal_Enable))


#define FSMC_ExtendedMode_Disable                ((uint32_t)0x00000000)
#define FSMC_ExtendedMode_Enable                 ((uint32_t)0x00004000)

#define IS_FSMC_EXTENDED_MODE(MODE) (((MODE) == FSMC_ExtendedMode_Disable) || \
                                     ((MODE) == FSMC_ExtendedMode_Enable))


#define FSMC_WriteBurst_Disable                  ((uint32_t)0x00000000)
#define FSMC_WriteBurst_Enable                   ((uint32_t)0x00080000)
#define IS_FSMC_WRITE_BURST(BURST) (((BURST) == FSMC_WriteBurst_Disable) || \
                                    ((BURST) == FSMC_WriteBurst_Enable))


#define IS_FSMC_ADDRESS_SETUP_TIME(TIME) ((TIME) <= 0xF)


#define IS_FSMC_ADDRESS_HOLD_TIME(TIME) ((TIME) <= 0xF)


#define IS_FSMC_DATASETUP_TIME(TIME) (((TIME) > 0) && ((TIME) <= 0xFF))


#define IS_FSMC_TURNAROUND_TIME(TIME) ((TIME) <= 0xF)


#define IS_FSMC_CLK_DIV(DIV) ((DIV) <= 0xF)


#define IS_FSMC_DATA_LATENCY(LATENCY) ((LATENCY) <= 0xF)


#define FSMC_AccessMode_A                        ((uint32_t)0x00000000)
#define FSMC_AccessMode_B                        ((uint32_t)0x10000000)
#define FSMC_AccessMode_C                        ((uint32_t)0x20000000)
#define FSMC_AccessMode_D                        ((uint32_t)0x30000000)
#define IS_FSMC_ACCESS_MODE(MODE) (((MODE) == FSMC_AccessMode_A) || \
                                   ((MODE) == FSMC_AccessMode_B) || \
                                   ((MODE) == FSMC_AccessMode_C) || \
                                   ((MODE) == FSMC_AccessMode_D))


#define FSMC_Waitfeature_Disable                 ((uint32_t)0x00000000)
#define FSMC_Waitfeature_Enable                  ((uint32_t)0x00000002)
#define IS_FSMC_WAIT_FEATURE(FEATURE) (((FEATURE) == FSMC_Waitfeature_Disable) || \
                                       ((FEATURE) == FSMC_Waitfeature_Enable))


#define FSMC_ECC_Disable                         ((uint32_t)0x00000000)
#define FSMC_ECC_Enable                          ((uint32_t)0x00000040)
#define IS_FSMC_ECC_STATE(STATE) (((STATE) == FSMC_ECC_Disable) || \
                                  ((STATE) == FSMC_ECC_Enable))


#define FSMC_ECCPageSize_256Bytes                ((uint32_t)0x00000000)
#define FSMC_ECCPageSize_512Bytes                ((uint32_t)0x00020000)
#define FSMC_ECCPageSize_1024Bytes               ((uint32_t)0x00040000)
#define FSMC_ECCPageSize_2048Bytes               ((uint32_t)0x00060000)
#define FSMC_ECCPageSize_4096Bytes               ((uint32_t)0x00080000)
#define FSMC_ECCPageSize_8192Bytes               ((uint32_t)0x000A0000)
#define IS_FSMC_ECCPAGE_SIZE(SIZE) (((SIZE) == FSMC_ECCPageSize_256Bytes) || \
                                    ((SIZE) == FSMC_ECCPageSize_512Bytes) || \
                                    ((SIZE) == FSMC_ECCPageSize_1024Bytes) || \
                                    ((SIZE) == FSMC_ECCPageSize_2048Bytes) || \
                                    ((SIZE) == FSMC_ECCPageSize_4096Bytes) || \
                                    ((SIZE) == FSMC_ECCPageSize_8192Bytes))


#define IS_FSMC_TCLR_TIME(TIME) ((TIME) <= 0xFF)


#define IS_FSMC_TAR_TIME(TIME) ((TIME) <= 0xFF)


#define IS_FSMC_SETUP_TIME(TIME) ((TIME) <= 0xFF)


#define IS_FSMC_WAIT_TIME(TIME) ((TIME) <= 0xFF)


#define IS_FSMC_HOLD_TIME(TIME) ((TIME) <= 0xFF)


#define IS_FSMC_HIZ_TIME(TIME) ((TIME) <= 0xFF)


#define FSMC_IT_RisingEdge                       ((uint32_t)0x00000008)
#define FSMC_IT_Level                            ((uint32_t)0x00000010)
#define FSMC_IT_FallingEdge                      ((uint32_t)0x00000020)
#define IS_FSMC_IT(IT) ((((IT) & (uint32_t)0xFFFFFFC7) == 0x00000000) && ((IT) != 0x00000000))
#define IS_FSMC_GET_IT(IT) (((IT) == FSMC_IT_RisingEdge) || \
                            ((IT) == FSMC_IT_Level) || \
                            ((IT) == FSMC_IT_FallingEdge))


#define FSMC_FLAG_RisingEdge                     ((uint32_t)0x00000001)
#define FSMC_FLAG_Level                          ((uint32_t)0x00000002)
#define FSMC_FLAG_FallingEdge                    ((uint32_t)0x00000004)
#define FSMC_FLAG_FEMPT                          ((uint32_t)0x00000040)
#define IS_FSMC_GET_FLAG(FLAG) (((FLAG) == FSMC_FLAG_RisingEdge) || \
                                ((FLAG) == FSMC_FLAG_Level) || \
                                ((FLAG) == FSMC_FLAG_FallingEdge) || \
                                ((FLAG) == FSMC_FLAG_FEMPT))

#define IS_FSMC_CLEAR_FLAG(FLAG) ((((FLAG) & (uint32_t)0xFFFFFFF8) == 0x00000000) && ((FLAG) != 0x00000000))


void FSMC_NORSRAMDeInit(uint32_t FSMC_Bank);
void FSMC_NORSRAMInit(FSMC_NORSRAMInitTypeDef* FSMC_NORSRAMInitStruct);
void FSMC_NORSRAMStructInit(FSMC_NORSRAMInitTypeDef* FSMC_NORSRAMInitStruct);
void FSMC_NORSRAMCmd(uint32_t FSMC_Bank, FunctionalState NewState);


void FSMC_NANDDeInit(uint32_t FSMC_Bank);
void FSMC_NANDInit(FSMC_NANDInitTypeDef* FSMC_NANDInitStruct);
void FSMC_NANDStructInit(FSMC_NANDInitTypeDef* FSMC_NANDInitStruct);
void FSMC_NANDCmd(uint32_t FSMC_Bank, FunctionalState NewState);
void FSMC_NANDECCCmd(uint32_t FSMC_Bank, FunctionalState NewState);
uint32_t FSMC_GetECC(uint32_t FSMC_Bank);


void FSMC_PCCARDDeInit(void);
void FSMC_PCCARDInit(FSMC_PCCARDInitTypeDef* FSMC_PCCARDInitStruct);
void FSMC_PCCARDStructInit(FSMC_PCCARDInitTypeDef* FSMC_PCCARDInitStruct);
void FSMC_PCCARDCmd(FunctionalState NewState);


void FSMC_ITConfig(uint32_t FSMC_Bank, uint32_t FSMC_IT, FunctionalState NewState);
FlagStatus FSMC_GetFlagStatus(uint32_t FSMC_Bank, uint32_t FSMC_FLAG);
void FSMC_ClearFlag(uint32_t FSMC_Bank, uint32_t FSMC_FLAG);
ITStatus FSMC_GetITStatus(uint32_t FSMC_Bank, uint32_t FSMC_IT);
void FSMC_ClearITPendingBit(uint32_t FSMC_Bank, uint32_t FSMC_IT);

#ifdef __cplusplus
}
#endif

#endif
