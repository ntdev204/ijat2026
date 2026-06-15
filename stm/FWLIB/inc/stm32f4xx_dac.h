/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        stm32f4xx_dac.h
 *
 * Description:  stm32f4xx_dac.h module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#ifndef __STM32F4xx_DAC_H
#define __STM32F4xx_DAC_H

#ifdef __cplusplus
 extern "C" {
#endif


#include "stm32f4xx.h"


typedef struct
{
  uint32_t DAC_Trigger;

  uint32_t DAC_WaveGeneration;

  uint32_t DAC_LFSRUnmask_TriangleAmplitude;

  uint32_t DAC_OutputBuffer;
}DAC_InitTypeDef;


#define DAC_Trigger_None                   ((uint32_t)0x00000000)
#define DAC_Trigger_T2_TRGO                ((uint32_t)0x00000024)
#define DAC_Trigger_T4_TRGO                ((uint32_t)0x0000002C)
#define DAC_Trigger_T5_TRGO                ((uint32_t)0x0000001C)
#define DAC_Trigger_T6_TRGO                ((uint32_t)0x00000004)
#define DAC_Trigger_T7_TRGO                ((uint32_t)0x00000014)
#define DAC_Trigger_T8_TRGO                ((uint32_t)0x0000000C)

#define DAC_Trigger_Ext_IT9                ((uint32_t)0x00000034)
#define DAC_Trigger_Software               ((uint32_t)0x0000003C)

#define IS_DAC_TRIGGER(TRIGGER) (((TRIGGER) == DAC_Trigger_None) || \
                                 ((TRIGGER) == DAC_Trigger_T6_TRGO) || \
                                 ((TRIGGER) == DAC_Trigger_T8_TRGO) || \
                                 ((TRIGGER) == DAC_Trigger_T7_TRGO) || \
                                 ((TRIGGER) == DAC_Trigger_T5_TRGO) || \
                                 ((TRIGGER) == DAC_Trigger_T2_TRGO) || \
                                 ((TRIGGER) == DAC_Trigger_T4_TRGO) || \
                                 ((TRIGGER) == DAC_Trigger_Ext_IT9) || \
                                 ((TRIGGER) == DAC_Trigger_Software))


#define DAC_WaveGeneration_None            ((uint32_t)0x00000000)
#define DAC_WaveGeneration_Noise           ((uint32_t)0x00000040)
#define DAC_WaveGeneration_Triangle        ((uint32_t)0x00000080)
#define IS_DAC_GENERATE_WAVE(WAVE) (((WAVE) == DAC_WaveGeneration_None) || \
                                    ((WAVE) == DAC_WaveGeneration_Noise) || \
                                    ((WAVE) == DAC_WaveGeneration_Triangle))


#define DAC_LFSRUnmask_Bit0                ((uint32_t)0x00000000)
#define DAC_LFSRUnmask_Bits1_0             ((uint32_t)0x00000100)
#define DAC_LFSRUnmask_Bits2_0             ((uint32_t)0x00000200)
#define DAC_LFSRUnmask_Bits3_0             ((uint32_t)0x00000300)
#define DAC_LFSRUnmask_Bits4_0             ((uint32_t)0x00000400)
#define DAC_LFSRUnmask_Bits5_0             ((uint32_t)0x00000500)
#define DAC_LFSRUnmask_Bits6_0             ((uint32_t)0x00000600)
#define DAC_LFSRUnmask_Bits7_0             ((uint32_t)0x00000700)
#define DAC_LFSRUnmask_Bits8_0             ((uint32_t)0x00000800)
#define DAC_LFSRUnmask_Bits9_0             ((uint32_t)0x00000900)
#define DAC_LFSRUnmask_Bits10_0            ((uint32_t)0x00000A00)
#define DAC_LFSRUnmask_Bits11_0            ((uint32_t)0x00000B00)
#define DAC_TriangleAmplitude_1            ((uint32_t)0x00000000)
#define DAC_TriangleAmplitude_3            ((uint32_t)0x00000100)
#define DAC_TriangleAmplitude_7            ((uint32_t)0x00000200)
#define DAC_TriangleAmplitude_15           ((uint32_t)0x00000300)
#define DAC_TriangleAmplitude_31           ((uint32_t)0x00000400)
#define DAC_TriangleAmplitude_63           ((uint32_t)0x00000500)
#define DAC_TriangleAmplitude_127          ((uint32_t)0x00000600)
#define DAC_TriangleAmplitude_255          ((uint32_t)0x00000700)
#define DAC_TriangleAmplitude_511          ((uint32_t)0x00000800)
#define DAC_TriangleAmplitude_1023         ((uint32_t)0x00000900)
#define DAC_TriangleAmplitude_2047         ((uint32_t)0x00000A00)
#define DAC_TriangleAmplitude_4095         ((uint32_t)0x00000B00)

#define IS_DAC_LFSR_UNMASK_TRIANGLE_AMPLITUDE(VALUE) (((VALUE) == DAC_LFSRUnmask_Bit0) || \
                                                      ((VALUE) == DAC_LFSRUnmask_Bits1_0) || \
                                                      ((VALUE) == DAC_LFSRUnmask_Bits2_0) || \
                                                      ((VALUE) == DAC_LFSRUnmask_Bits3_0) || \
                                                      ((VALUE) == DAC_LFSRUnmask_Bits4_0) || \
                                                      ((VALUE) == DAC_LFSRUnmask_Bits5_0) || \
                                                      ((VALUE) == DAC_LFSRUnmask_Bits6_0) || \
                                                      ((VALUE) == DAC_LFSRUnmask_Bits7_0) || \
                                                      ((VALUE) == DAC_LFSRUnmask_Bits8_0) || \
                                                      ((VALUE) == DAC_LFSRUnmask_Bits9_0) || \
                                                      ((VALUE) == DAC_LFSRUnmask_Bits10_0) || \
                                                      ((VALUE) == DAC_LFSRUnmask_Bits11_0) || \
                                                      ((VALUE) == DAC_TriangleAmplitude_1) || \
                                                      ((VALUE) == DAC_TriangleAmplitude_3) || \
                                                      ((VALUE) == DAC_TriangleAmplitude_7) || \
                                                      ((VALUE) == DAC_TriangleAmplitude_15) || \
                                                      ((VALUE) == DAC_TriangleAmplitude_31) || \
                                                      ((VALUE) == DAC_TriangleAmplitude_63) || \
                                                      ((VALUE) == DAC_TriangleAmplitude_127) || \
                                                      ((VALUE) == DAC_TriangleAmplitude_255) || \
                                                      ((VALUE) == DAC_TriangleAmplitude_511) || \
                                                      ((VALUE) == DAC_TriangleAmplitude_1023) || \
                                                      ((VALUE) == DAC_TriangleAmplitude_2047) || \
                                                      ((VALUE) == DAC_TriangleAmplitude_4095))


#define DAC_OutputBuffer_Enable            ((uint32_t)0x00000000)
#define DAC_OutputBuffer_Disable           ((uint32_t)0x00000002)
#define IS_DAC_OUTPUT_BUFFER_STATE(STATE) (((STATE) == DAC_OutputBuffer_Enable) || \
                                           ((STATE) == DAC_OutputBuffer_Disable))


#define DAC_Channel_1                      ((uint32_t)0x00000000)
#define DAC_Channel_2                      ((uint32_t)0x00000010)
#define IS_DAC_CHANNEL(CHANNEL) (((CHANNEL) == DAC_Channel_1) || \
                                 ((CHANNEL) == DAC_Channel_2))


#define DAC_Align_12b_R                    ((uint32_t)0x00000000)
#define DAC_Align_12b_L                    ((uint32_t)0x00000004)
#define DAC_Align_8b_R                     ((uint32_t)0x00000008)
#define IS_DAC_ALIGN(ALIGN) (((ALIGN) == DAC_Align_12b_R) || \
                             ((ALIGN) == DAC_Align_12b_L) || \
                             ((ALIGN) == DAC_Align_8b_R))


#define DAC_Wave_Noise                     ((uint32_t)0x00000040)
#define DAC_Wave_Triangle                  ((uint32_t)0x00000080)
#define IS_DAC_WAVE(WAVE) (((WAVE) == DAC_Wave_Noise) || \
                           ((WAVE) == DAC_Wave_Triangle))


#define IS_DAC_DATA(DATA) ((DATA) <= 0xFFF0)


#define DAC_IT_DMAUDR                      ((uint32_t)0x00002000)
#define IS_DAC_IT(IT) (((IT) == DAC_IT_DMAUDR))


#define DAC_FLAG_DMAUDR                    ((uint32_t)0x00002000)
#define IS_DAC_FLAG(FLAG) (((FLAG) == DAC_FLAG_DMAUDR))


void DAC_DeInit(void);


void DAC_Init(uint32_t DAC_Channel, DAC_InitTypeDef* DAC_InitStruct);
void DAC_StructInit(DAC_InitTypeDef* DAC_InitStruct);
void DAC_Cmd(uint32_t DAC_Channel, FunctionalState NewState);
void DAC_SoftwareTriggerCmd(uint32_t DAC_Channel, FunctionalState NewState);
void DAC_DualSoftwareTriggerCmd(FunctionalState NewState);
void DAC_WaveGenerationCmd(uint32_t DAC_Channel, uint32_t DAC_Wave, FunctionalState NewState);
void DAC_SetChannel1Data(uint32_t DAC_Align, uint16_t Data);
void DAC_SetChannel2Data(uint32_t DAC_Align, uint16_t Data);
void DAC_SetDualChannelData(uint32_t DAC_Align, uint16_t Data2, uint16_t Data1);
uint16_t DAC_GetDataOutputValue(uint32_t DAC_Channel);


void DAC_DMACmd(uint32_t DAC_Channel, FunctionalState NewState);


void DAC_ITConfig(uint32_t DAC_Channel, uint32_t DAC_IT, FunctionalState NewState);
FlagStatus DAC_GetFlagStatus(uint32_t DAC_Channel, uint32_t DAC_FLAG);
void DAC_ClearFlag(uint32_t DAC_Channel, uint32_t DAC_FLAG);
ITStatus DAC_GetITStatus(uint32_t DAC_Channel, uint32_t DAC_IT);
void DAC_ClearITPendingBit(uint32_t DAC_Channel, uint32_t DAC_IT);

#ifdef __cplusplus
}
#endif

#endif
