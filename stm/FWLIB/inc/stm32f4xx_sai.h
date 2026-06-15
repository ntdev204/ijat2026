/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        stm32f4xx_sai.h
 *
 * Description:  stm32f4xx_sai.h module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#ifndef __STM32F4xx_SAI_H
#define __STM32F4xx_SAI_H

#ifdef __cplusplus
 extern "C" {
#endif


#include "stm32f4xx.h"


typedef struct
{
  uint32_t SAI_AudioMode;

  uint32_t SAI_Protocol;

  uint32_t SAI_DataSize;

  uint32_t SAI_FirstBit;

  uint32_t SAI_ClockStrobing;

  uint32_t SAI_Synchro;

  uint32_t SAI_OUTDRIV;

  uint32_t SAI_NoDivider;

  uint32_t SAI_MasterDivider;

  uint32_t SAI_FIFOThreshold;
}SAI_InitTypeDef;


typedef struct
{

  uint32_t SAI_FrameLength;

  uint32_t SAI_ActiveFrameLength;

  uint32_t SAI_FSDefinition;

  uint32_t SAI_FSPolarity;

  uint32_t SAI_FSOffset;

}SAI_FrameInitTypeDef;


typedef struct
{
  uint32_t SAI_FirstBitOffset;

  uint32_t SAI_SlotSize;

  uint32_t SAI_SlotNumber;

  uint32_t SAI_SlotActive;
}SAI_SlotInitTypeDef;


#define IS_SAI_PERIPH(PERIPH) ((PERIPH) == SAI1)

#define IS_SAI_BLOCK_PERIPH(PERIPH) (((PERIPH) == SAI1_Block_A) || \
                                     ((PERIPH) == SAI1_Block_B))


#define SAI_Mode_MasterTx               ((uint32_t)0x00000000)
#define SAI_Mode_MasterRx               ((uint32_t)0x00000001)
#define SAI_Mode_SlaveTx                ((uint32_t)0x00000002)
#define SAI_Mode_SlaveRx                ((uint32_t)0x00000003)
#define IS_SAI_BLOCK_MODE(MODE) (((MODE) == SAI_Mode_MasterTx) || \
                                 ((MODE) == SAI_Mode_MasterRx) || \
                                 ((MODE) == SAI_Mode_SlaveTx)  || \
                                 ((MODE) == SAI_Mode_SlaveRx))


#define SAI_Free_Protocol                 ((uint32_t)0x00000000)
#define SAI_SPDIF_Protocol                ((uint32_t)SAI_xCR1_PRTCFG_0)
#define SAI_AC97_Protocol                 ((uint32_t)SAI_xCR1_PRTCFG_1)
#define IS_SAI_BLOCK_PROTOCOL(PROTOCOL) (((PROTOCOL) == SAI_Free_Protocol)  || \
                                         ((PROTOCOL) == SAI_SPDIF_Protocol) || \
                                         ((PROTOCOL) == SAI_AC97_Protocol))


#define SAI_DataSize_8b                   ((uint32_t)0x00000040)
#define SAI_DataSize_10b                  ((uint32_t)0x00000060)
#define SAI_DataSize_16b                  ((uint32_t)0x00000080)
#define SAI_DataSize_20b                  ((uint32_t)0x000000A0)
#define SAI_DataSize_24b                  ((uint32_t)0x000000C0)
#define SAI_DataSize_32b                  ((uint32_t)0x000000E0)
#define IS_SAI_BLOCK_DATASIZE(DATASIZE) (((DATASIZE) == SAI_DataSize_8b)  || \
                                         ((DATASIZE) == SAI_DataSize_10b) || \
                                         ((DATASIZE) == SAI_DataSize_16b) || \
                                         ((DATASIZE) == SAI_DataSize_20b) || \
                                         ((DATASIZE) == SAI_DataSize_24b) || \
                                         ((DATASIZE) == SAI_DataSize_32b))


#define SAI_FirstBit_MSB                  ((uint32_t)0x00000000)
#define SAI_FirstBit_LSB                  ((uint32_t)SAI_xCR1_LSBFIRST)
#define IS_SAI_BLOCK_FIRST_BIT(BIT) (((BIT) == SAI_FirstBit_MSB) || \
                                     ((BIT) == SAI_FirstBit_LSB))


#define SAI_ClockStrobing_FallingEdge     ((uint32_t)0x00000000)
#define SAI_ClockStrobing_RisingEdge      ((uint32_t)SAI_xCR1_CKSTR)
#define IS_SAI_BLOCK_CLOCK_STROBING(CLOCK) (((CLOCK) == SAI_ClockStrobing_FallingEdge) || \
                                            ((CLOCK) == SAI_ClockStrobing_RisingEdge))


#define SAI_Asynchronous                   ((uint32_t)0x00000000)
#define SAI_Synchronous                    ((uint32_t)SAI_xCR1_SYNCEN_0)
#define IS_SAI_BLOCK_SYNCHRO(SYNCHRO) (((SYNCHRO) == SAI_Synchronous) || \
                                       ((SYNCHRO) == SAI_Asynchronous))


#define SAI_OutputDrive_Disabled          ((uint32_t)0x00000000)
#define SAI_OutputDrive_Enabled           ((uint32_t)SAI_xCR1_OUTDRIV)
#define IS_SAI_BLOCK_OUTPUT_DRIVE(DRIVE) (((DRIVE) == SAI_OutputDrive_Disabled) || \
                                          ((DRIVE) == SAI_OutputDrive_Enabled))


#define SAI_MasterDivider_Enabled         ((uint32_t)0x00000000)
#define SAI_MasterDivider_Disabled        ((uint32_t)SAI_xCR1_NODIV)
#define IS_SAI_BLOCK_NODIVIDER(NODIVIDER) (((NODIVIDER) == SAI_MasterDivider_Enabled) || \
                                           ((NODIVIDER) == SAI_MasterDivider_Disabled))


#define IS_SAI_BLOCK_MASTER_DIVIDER(DIVIDER) ((DIVIDER) <= 15)


#define IS_SAI_BLOCK_FRAME_LENGTH(LENGTH) ((8 <= (LENGTH)) && ((LENGTH) <= 256))


#define IS_SAI_BLOCK_ACTIVE_FRAME(LENGTH) ((1 <= (LENGTH)) && ((LENGTH) <= 128))


#define SAI_FS_StartFrame                 ((uint32_t)0x00000000)
#define I2S_FS_ChannelIdentification      ((uint32_t)SAI_xFRCR_FSDEF)
#define IS_SAI_BLOCK_FS_DEFINITION(DEFINITION) (((DEFINITION) == SAI_FS_StartFrame) || \
                                                ((DEFINITION) == I2S_FS_ChannelIdentification))


#define SAI_FS_ActiveLow                  ((uint32_t)0x00000000)
#define SAI_FS_ActiveHigh                 ((uint32_t)SAI_xFRCR_FSPO)
#define IS_SAI_BLOCK_FS_POLARITY(POLARITY) (((POLARITY) == SAI_FS_ActiveLow) || \
                                            ((POLARITY) == SAI_FS_ActiveHigh))


#define SAI_FS_FirstBit                   ((uint32_t)0x00000000)
#define SAI_FS_BeforeFirstBit             ((uint32_t)SAI_xFRCR_FSOFF)
#define IS_SAI_BLOCK_FS_OFFSET(OFFSET) (((OFFSET) == SAI_FS_FirstBit) || \
                                        ((OFFSET) == SAI_FS_BeforeFirstBit))


#define IS_SAI_BLOCK_FIRSTBIT_OFFSET(OFFSET) ((OFFSET) <= 24)


#define SAI_SlotSize_DataSize             ((uint32_t)0x00000000)
#define SAI_SlotSize_16b                  ((uint32_t)SAI_xSLOTR_SLOTSZ_0)
#define SAI_SlotSize_32b                  ((uint32_t)SAI_xSLOTR_SLOTSZ_1)
#define IS_SAI_BLOCK_SLOT_SIZE(SIZE) (((SIZE) == SAI_SlotSize_DataSize) || \
                                      ((SIZE) == SAI_SlotSize_16b)      || \
                                      ((SIZE) == SAI_SlotSize_32b))


#define IS_SAI_BLOCK_SLOT_NUMBER(NUMBER) ((1 <= (NUMBER)) && ((NUMBER) <= 16))


#define SAI_Slot_NotActive           ((uint32_t)0x00000000)
#define SAI_SlotActive_0             ((uint32_t)0x00010000)
#define SAI_SlotActive_1             ((uint32_t)0x00020000)
#define SAI_SlotActive_2             ((uint32_t)0x00040000)
#define SAI_SlotActive_3             ((uint32_t)0x00080000)
#define SAI_SlotActive_4             ((uint32_t)0x00100000)
#define SAI_SlotActive_5             ((uint32_t)0x00200000)
#define SAI_SlotActive_6             ((uint32_t)0x00400000)
#define SAI_SlotActive_7             ((uint32_t)0x00800000)
#define SAI_SlotActive_8             ((uint32_t)0x01000000)
#define SAI_SlotActive_9             ((uint32_t)0x02000000)
#define SAI_SlotActive_10            ((uint32_t)0x04000000)
#define SAI_SlotActive_11            ((uint32_t)0x08000000)
#define SAI_SlotActive_12            ((uint32_t)0x10000000)
#define SAI_SlotActive_13            ((uint32_t)0x20000000)
#define SAI_SlotActive_14            ((uint32_t)0x40000000)
#define SAI_SlotActive_15            ((uint32_t)0x80000000)
#define SAI_SlotActive_ALL           ((uint32_t)0xFFFF0000)

#define IS_SAI_SLOT_ACTIVE(ACTIVE) ((ACTIVE) != 0)


#define SAI_MonoMode                      ((uint32_t)SAI_xCR1_MONO)
#define SAI_StreoMode                     ((uint32_t)0x00000000)
#define IS_SAI_BLOCK_MONO_STREO_MODE(MODE) (((MODE) == SAI_MonoMode) ||\
                                            ((MODE) == SAI_StreoMode))


#define SAI_Output_NotReleased              ((uint32_t)0x00000000)
#define SAI_Output_Released                 ((uint32_t)SAI_xCR2_TRIS)
#define IS_SAI_BLOCK_TRISTATE_MANAGEMENT(STATE) (((STATE) == SAI_Output_NotReleased) ||\
                                                 ((STATE) == SAI_Output_Released))


#define SAI_Threshold_FIFOEmpty           ((uint32_t)0x00000000)
#define SAI_FIFOThreshold_1QuarterFull    ((uint32_t)0x00000001)
#define SAI_FIFOThreshold_HalfFull        ((uint32_t)0x00000002)
#define SAI_FIFOThreshold_3QuartersFull   ((uint32_t)0x00000003)
#define SAI_FIFOThreshold_Full            ((uint32_t)0x00000004)
#define IS_SAI_BLOCK_FIFO_THRESHOLD(THRESHOLD) (((THRESHOLD) == SAI_Threshold_FIFOEmpty)         || \
                                                ((THRESHOLD) == SAI_FIFOThreshold_1QuarterFull)  || \
                                                ((THRESHOLD) == SAI_FIFOThreshold_HalfFull)      || \
                                                ((THRESHOLD) == SAI_FIFOThreshold_3QuartersFull) || \
                                                ((THRESHOLD) == SAI_FIFOThreshold_Full))


#define SAI_NoCompanding                  ((uint32_t)0x00000000)
#define SAI_ULaw_1CPL_Companding          ((uint32_t)0x00008000)
#define SAI_ALaw_1CPL_Companding          ((uint32_t)0x0000C000)
#define SAI_ULaw_2CPL_Companding          ((uint32_t)0x0000A000)
#define SAI_ALaw_2CPL_Companding          ((uint32_t)0x0000E000)
#define IS_SAI_BLOCK_COMPANDING_MODE(MODE)    (((MODE) == SAI_NoCompanding)        || \
                                              ((MODE) == SAI_ULaw_1CPL_Companding) || \
                                              ((MODE) == SAI_ALaw_1CPL_Companding) || \
                                              ((MODE) == SAI_ULaw_2CPL_Companding) || \
                                              ((MODE) == SAI_ALaw_2CPL_Companding))


#define SAI_ZeroValue                     ((uint32_t)0x00000000)
#define SAI_LastSentValue                 ((uint32_t)SAI_xCR2_MUTEVAL)
#define IS_SAI_BLOCK_MUTE_VALUE(VALUE)    (((VALUE) == SAI_ZeroValue)     || \
                                           ((VALUE) == SAI_LastSentValue))


#define IS_SAI_BLOCK_MUTE_COUNTER(COUNTER) ((COUNTER) <= 63)


#define SAI_IT_OVRUDR                     ((uint32_t)SAI_xIMR_OVRUDRIE)
#define SAI_IT_MUTEDET                    ((uint32_t)SAI_xIMR_MUTEDETIE)
#define SAI_IT_WCKCFG                     ((uint32_t)SAI_xIMR_WCKCFGIE)
#define SAI_IT_FREQ                       ((uint32_t)SAI_xIMR_FREQIE)
#define SAI_IT_CNRDY                      ((uint32_t)SAI_xIMR_CNRDYIE)
#define SAI_IT_AFSDET                     ((uint32_t)SAI_xIMR_AFSDETIE)
#define SAI_IT_LFSDET                     ((uint32_t)SAI_xIMR_LFSDETIE)

#define IS_SAI_BLOCK_CONFIG_IT(IT) (((IT) == SAI_IT_OVRUDR)  || \
                                    ((IT) == SAI_IT_MUTEDET) || \
                                    ((IT) == SAI_IT_WCKCFG)  || \
                                    ((IT) == SAI_IT_FREQ)    || \
                                    ((IT) == SAI_IT_CNRDY)   || \
                                    ((IT) == SAI_IT_AFSDET)  || \
                                    ((IT) == SAI_IT_LFSDET))


#define SAI_FLAG_OVRUDR                   ((uint32_t)SAI_xSR_OVRUDR)
#define SAI_FLAG_MUTEDET                  ((uint32_t)SAI_xSR_MUTEDET)
#define SAI_FLAG_WCKCFG                   ((uint32_t)SAI_xSR_WCKCFG)
#define SAI_FLAG_FREQ                     ((uint32_t)SAI_xSR_FREQ)
#define SAI_FLAG_CNRDY                    ((uint32_t)SAI_xSR_CNRDY)
#define SAI_FLAG_AFSDET                   ((uint32_t)SAI_xSR_AFSDET)
#define SAI_FLAG_LFSDET                   ((uint32_t)SAI_xSR_LFSDET)

#define IS_SAI_BLOCK_GET_FLAG(FLAG) (((FLAG) == SAI_FLAG_OVRUDR)  || \
                                    ((FLAG) == SAI_FLAG_MUTEDET) || \
                                    ((FLAG) == SAI_FLAG_WCKCFG)  || \
                                    ((FLAG) == SAI_FLAG_FREQ)    || \
                                    ((FLAG) == SAI_FLAG_CNRDY)   || \
                                    ((FLAG) == SAI_FLAG_AFSDET)  || \
                                    ((FLAG) == SAI_FLAG_LFSDET))

#define IS_SAI_BLOCK_CLEAR_FLAG(FLAG) (((FLAG) == SAI_FLAG_OVRUDR)  || \
                                       ((FLAG) == SAI_FLAG_MUTEDET) || \
                                       ((FLAG) == SAI_FLAG_WCKCFG)  || \
                                       ((FLAG) == SAI_FLAG_FREQ)    || \
                                       ((FLAG) == SAI_FLAG_CNRDY)   || \
                                       ((FLAG) == SAI_FLAG_AFSDET)  || \
                                       ((FLAG) == SAI_FLAG_LFSDET))


#define SAI_FIFOStatus_Empty              ((uint32_t)0x00000000)
#define SAI_FIFOStatus_Less1QuarterFull   ((uint32_t)0x00010000)
#define SAI_FIFOStatus_1QuarterFull       ((uint32_t)0x00020000)
#define SAI_FIFOStatus_HalfFull           ((uint32_t)0x00030000)
#define SAI_FIFOStatus_3QuartersFull      ((uint32_t)0x00040000)
#define SAI_FIFOStatus_Full               ((uint32_t)0x00050000)

#define IS_SAI_BLOCK_FIFO_STATUS(STATUS) (((STATUS) == SAI_FIFOStatus_Less1QuarterFull ) || \
                                          ((STATUS) == SAI_FIFOStatus_HalfFull)          || \
                                          ((STATUS) == SAI_FIFOStatus_1QuarterFull)      || \
                                          ((STATUS) == SAI_FIFOStatus_3QuartersFull)     || \
                                          ((STATUS) == SAI_FIFOStatus_Full)              || \
                                          ((STATUS) == SAI_FIFOStatus_Empty))


void SAI_DeInit(SAI_TypeDef* SAIx);


void SAI_Init(SAI_Block_TypeDef* SAI_Block_x, SAI_InitTypeDef* SAI_InitStruct);
void SAI_FrameInit(SAI_Block_TypeDef* SAI_Block_x, SAI_FrameInitTypeDef* SAI_FrameInitStruct);
void SAI_SlotInit(SAI_Block_TypeDef* SAI_Block_x, SAI_SlotInitTypeDef* SAI_SlotInitStruct);
void SAI_StructInit(SAI_InitTypeDef* SAI_InitStruct);
void SAI_FrameStructInit(SAI_FrameInitTypeDef* SAI_FrameInitStruct);
void SAI_SlotStructInit(SAI_SlotInitTypeDef* SAI_SlotInitStruct);

void SAI_Cmd(SAI_Block_TypeDef* SAI_Block_x, FunctionalState NewState);
void SAI_MonoModeConfig(SAI_Block_TypeDef* SAI_Block_x, uint32_t SAI_Mono_StreoMode);
void SAI_TRIStateConfig(SAI_Block_TypeDef* SAI_Block_x, uint32_t SAI_TRIState);
void SAI_CompandingModeConfig(SAI_Block_TypeDef* SAI_Block_x, uint32_t SAI_CompandingMode);
void SAI_MuteModeCmd(SAI_Block_TypeDef* SAI_Block_x, FunctionalState NewState);
void SAI_MuteValueConfig(SAI_Block_TypeDef* SAI_Block_x, uint32_t SAI_MuteValue);
void SAI_MuteFrameCounterConfig(SAI_Block_TypeDef* SAI_Block_x, uint32_t SAI_MuteCounter);
void SAI_FlushFIFO(SAI_Block_TypeDef* SAI_Block_x);


void SAI_SendData(SAI_Block_TypeDef* SAI_Block_x, uint32_t Data);
uint32_t SAI_ReceiveData(SAI_Block_TypeDef* SAI_Block_x);


void SAI_DMACmd(SAI_Block_TypeDef* SAI_Block_x, FunctionalState NewState);


void SAI_ITConfig(SAI_Block_TypeDef* SAI_Block_x, uint32_t SAI_IT, FunctionalState NewState);
FlagStatus SAI_GetFlagStatus(SAI_Block_TypeDef* SAI_Block_x, uint32_t SAI_FLAG);
void SAI_ClearFlag(SAI_Block_TypeDef* SAI_Block_x, uint32_t SAI_FLAG);
ITStatus SAI_GetITStatus(SAI_Block_TypeDef* SAI_Block_x, uint32_t SAI_IT);
void SAI_ClearITPendingBit(SAI_Block_TypeDef* SAI_Block_x, uint32_t SAI_IT);
FunctionalState SAI_GetCmdStatus(SAI_Block_TypeDef* SAI_Block_x);
uint32_t SAI_GetFIFOStatus(SAI_Block_TypeDef* SAI_Block_x);

#ifdef __cplusplus
}
#endif

#endif
