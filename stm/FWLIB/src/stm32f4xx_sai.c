/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        stm32f4xx_sai.c
 *
 * Description:  stm32f4xx_sai.c module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#include "stm32f4xx_sai.h"
#include "stm32f4xx_rcc.h"


#define CR1_CLEAR_MASK            ((uint32_t)0xFF07C010)
#define FRCR_CLEAR_MASK           ((uint32_t)0xFFF88000)
#define SLOTR_CLEAR_MASK          ((uint32_t)0x0000F020)


void SAI_DeInit(SAI_TypeDef* SAIx)
{

  assert_param(IS_SAI_PERIPH(SAIx));


  RCC_APB2PeriphResetCmd(RCC_APB2Periph_SAI1, ENABLE);

  RCC_APB2PeriphResetCmd(RCC_APB2Periph_SAI1, DISABLE);
}


void SAI_Init(SAI_Block_TypeDef* SAI_Block_x, SAI_InitTypeDef* SAI_InitStruct)
{
  uint32_t tmpreg = 0;


  assert_param(IS_SAI_BLOCK_PERIPH(SAI_Block_x));


  assert_param(IS_SAI_BLOCK_MODE(SAI_InitStruct->SAI_AudioMode));
  assert_param(IS_SAI_BLOCK_PROTOCOL(SAI_InitStruct->SAI_Protocol));
  assert_param(IS_SAI_BLOCK_DATASIZE(SAI_InitStruct->SAI_DataSize));
  assert_param(IS_SAI_BLOCK_FIRST_BIT(SAI_InitStruct->SAI_FirstBit));
  assert_param(IS_SAI_BLOCK_CLOCK_STROBING(SAI_InitStruct->SAI_ClockStrobing));
  assert_param(IS_SAI_BLOCK_SYNCHRO(SAI_InitStruct->SAI_Synchro));
  assert_param(IS_SAI_BLOCK_OUTPUT_DRIVE(SAI_InitStruct->SAI_OUTDRIV));
  assert_param(IS_SAI_BLOCK_NODIVIDER(SAI_InitStruct->SAI_NoDivider));
  assert_param(IS_SAI_BLOCK_MASTER_DIVIDER(SAI_InitStruct->SAI_MasterDivider));
  assert_param(IS_SAI_BLOCK_FIFO_THRESHOLD(SAI_InitStruct->SAI_FIFOThreshold));


  tmpreg = SAI_Block_x->CR1;

  tmpreg &= CR1_CLEAR_MASK;


  tmpreg |= (uint32_t)(SAI_InitStruct->SAI_AudioMode     | SAI_InitStruct->SAI_Protocol  |
                       SAI_InitStruct->SAI_DataSize      | SAI_InitStruct->SAI_FirstBit  |
                       SAI_InitStruct->SAI_ClockStrobing | SAI_InitStruct->SAI_Synchro   |
                       SAI_InitStruct->SAI_OUTDRIV       | SAI_InitStruct->SAI_NoDivider |
                       (uint32_t)((SAI_InitStruct->SAI_MasterDivider) << 20));

  SAI_Block_x->CR1 = tmpreg;


  tmpreg = SAI_Block_x->CR2;

  tmpreg &= ~(SAI_xCR2_FTH);


  tmpreg |= (uint32_t)(SAI_InitStruct->SAI_FIFOThreshold);

  SAI_Block_x->CR2 = tmpreg;
}


void SAI_FrameInit(SAI_Block_TypeDef* SAI_Block_x, SAI_FrameInitTypeDef* SAI_FrameInitStruct)
{
  uint32_t tmpreg = 0;


  assert_param(IS_SAI_BLOCK_PERIPH(SAI_Block_x));


  assert_param(IS_SAI_BLOCK_FRAME_LENGTH(SAI_FrameInitStruct->SAI_FrameLength));
  assert_param(IS_SAI_BLOCK_ACTIVE_FRAME(SAI_FrameInitStruct->SAI_ActiveFrameLength));
  assert_param(IS_SAI_BLOCK_FS_DEFINITION(SAI_FrameInitStruct->SAI_FSDefinition));
  assert_param(IS_SAI_BLOCK_FS_POLARITY(SAI_FrameInitStruct->SAI_FSPolarity));
  assert_param(IS_SAI_BLOCK_FS_OFFSET(SAI_FrameInitStruct->SAI_FSOffset));


  tmpreg = SAI_Block_x->FRCR;

  tmpreg &= FRCR_CLEAR_MASK;


  tmpreg |= (uint32_t)((uint32_t)(SAI_FrameInitStruct->SAI_FrameLength - 1)  |
                       SAI_FrameInitStruct->SAI_FSOffset     |
                       SAI_FrameInitStruct->SAI_FSDefinition |
                       SAI_FrameInitStruct->SAI_FSPolarity   |
                       (uint32_t)((SAI_FrameInitStruct->SAI_ActiveFrameLength - 1) << 8));


  SAI_Block_x->FRCR = tmpreg;
}


void SAI_SlotInit(SAI_Block_TypeDef* SAI_Block_x, SAI_SlotInitTypeDef* SAI_SlotInitStruct)
{
  uint32_t tmpreg = 0;


  assert_param(IS_SAI_BLOCK_PERIPH(SAI_Block_x));


  assert_param(IS_SAI_BLOCK_FIRSTBIT_OFFSET(SAI_SlotInitStruct->SAI_FirstBitOffset));
  assert_param(IS_SAI_BLOCK_SLOT_SIZE(SAI_SlotInitStruct->SAI_SlotSize));
  assert_param(IS_SAI_BLOCK_SLOT_NUMBER(SAI_SlotInitStruct->SAI_SlotNumber));
  assert_param(IS_SAI_SLOT_ACTIVE(SAI_SlotInitStruct->SAI_SlotActive));


  tmpreg = SAI_Block_x->SLOTR;

  tmpreg &= SLOTR_CLEAR_MASK;


  tmpreg |= (uint32_t)(SAI_SlotInitStruct->SAI_FirstBitOffset |
                       SAI_SlotInitStruct->SAI_SlotSize       |
                       SAI_SlotInitStruct->SAI_SlotActive     |
                       (uint32_t)((SAI_SlotInitStruct->SAI_SlotNumber - 1) <<  8));


  SAI_Block_x->SLOTR = tmpreg;
}


void SAI_StructInit(SAI_InitTypeDef* SAI_InitStruct)
{


  SAI_InitStruct->SAI_AudioMode = SAI_Mode_MasterTx;

  SAI_InitStruct->SAI_Protocol = SAI_Free_Protocol;

  SAI_InitStruct->SAI_DataSize = SAI_DataSize_8b;

  SAI_InitStruct->SAI_FirstBit = SAI_FirstBit_MSB;

  SAI_InitStruct->SAI_ClockStrobing = SAI_ClockStrobing_FallingEdge;

  SAI_InitStruct->SAI_Synchro = SAI_Asynchronous;

  SAI_InitStruct->SAI_OUTDRIV = SAI_OutputDrive_Disabled;

  SAI_InitStruct->SAI_NoDivider = SAI_MasterDivider_Enabled;

  SAI_InitStruct->SAI_MasterDivider = 0;

  SAI_InitStruct->SAI_FIFOThreshold = SAI_Threshold_FIFOEmpty;
}


void SAI_FrameStructInit(SAI_FrameInitTypeDef* SAI_FrameInitStruct)
{


  SAI_FrameInitStruct->SAI_FrameLength = 8;

  SAI_FrameInitStruct->SAI_ActiveFrameLength = 1;

  SAI_FrameInitStruct->SAI_FSDefinition = SAI_FS_StartFrame;

  SAI_FrameInitStruct->SAI_FSPolarity = SAI_FS_ActiveLow;

  SAI_FrameInitStruct->SAI_FSOffset = SAI_FS_FirstBit;
}


void SAI_SlotStructInit(SAI_SlotInitTypeDef* SAI_SlotInitStruct)
{


  SAI_SlotInitStruct->SAI_FirstBitOffset = 0;

  SAI_SlotInitStruct->SAI_SlotSize = SAI_SlotSize_DataSize;

  SAI_SlotInitStruct->SAI_SlotNumber = 1;

  SAI_SlotInitStruct->SAI_SlotActive = SAI_Slot_NotActive;

}


void SAI_Cmd(SAI_Block_TypeDef* SAI_Block_x, FunctionalState NewState)
{

  assert_param(IS_SAI_BLOCK_PERIPH(SAI_Block_x));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {

    SAI_Block_x->CR1 |= SAI_xCR1_SAIEN;
  }
  else
  {

    SAI_Block_x->CR1 &= ~(SAI_xCR1_SAIEN);
  }
}


void SAI_MonoModeConfig(SAI_Block_TypeDef* SAI_Block_x, uint32_t SAI_Mono_StreoMode)
{

  assert_param(IS_SAI_BLOCK_PERIPH(SAI_Block_x));
  assert_param(IS_SAI_BLOCK_MONO_STREO_MODE(SAI_MonoMode));

  SAI_Block_x->CR1 &= ~(SAI_xCR1_MONO);

  SAI_Block_x->CR1 |= SAI_MonoMode;
}


void SAI_TRIStateConfig(SAI_Block_TypeDef* SAI_Block_x, uint32_t SAI_TRIState)
{

  assert_param(IS_SAI_BLOCK_PERIPH(SAI_Block_x));
  assert_param(IS_SAI_BLOCK_TRISTATE_MANAGEMENT(SAI_TRIState));

  SAI_Block_x->CR1 &= ~(SAI_xCR1_MONO);

  SAI_Block_x->CR1 |= SAI_MonoMode;

}


void SAI_CompandingModeConfig(SAI_Block_TypeDef* SAI_Block_x, uint32_t SAI_CompandingMode)
{

  assert_param(IS_SAI_BLOCK_PERIPH(SAI_Block_x));
  assert_param(IS_SAI_BLOCK_COMPANDING_MODE(SAI_CompandingMode));

  SAI_Block_x->CR2 &= ~(SAI_xCR2_COMP);

  SAI_Block_x->CR2 |= SAI_CompandingMode;
}


void SAI_MuteModeCmd(SAI_Block_TypeDef* SAI_Block_x, FunctionalState NewState)
{

  assert_param(IS_SAI_BLOCK_PERIPH(SAI_Block_x));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {

    SAI_Block_x->CR2 |= SAI_xCR2_MUTE;
  }
  else
  {

    SAI_Block_x->CR2 &= ~(SAI_xCR2_MUTE);
  }
}


void SAI_MuteValueConfig(SAI_Block_TypeDef* SAI_Block_x, uint32_t SAI_MuteValue)
{

  assert_param(IS_SAI_BLOCK_PERIPH(SAI_Block_x));
  assert_param(IS_SAI_BLOCK_MUTE_VALUE(SAI_MuteValue));


  SAI_Block_x->CR2 &= ~(SAI_xCR2_MUTEVAL);

  SAI_Block_x->CR2 |= SAI_MuteValue;
}


void SAI_MuteFrameCounterConfig(SAI_Block_TypeDef* SAI_Block_x, uint32_t SAI_MuteCounter)
{

  assert_param(IS_SAI_BLOCK_PERIPH(SAI_Block_x));
  assert_param(IS_SAI_BLOCK_MUTE_COUNTER(SAI_MuteCounter));


  SAI_Block_x->CR2 &= ~(SAI_xCR2_MUTECNT);

  SAI_Block_x->CR2 |= (SAI_MuteCounter << 7);
}


void SAI_FlushFIFO(SAI_Block_TypeDef* SAI_Block_x)
{

  assert_param(IS_SAI_BLOCK_PERIPH(SAI_Block_x));


  SAI_Block_x->CR2 |= SAI_xCR2_FFLUSH;
}


uint32_t SAI_ReceiveData(SAI_Block_TypeDef* SAI_Block_x)
{

  assert_param(IS_SAI_BLOCK_PERIPH(SAI_Block_x));


  return SAI_Block_x->DR;
}


void SAI_SendData(SAI_Block_TypeDef* SAI_Block_x, uint32_t Data)
{

  assert_param(IS_SAI_BLOCK_PERIPH(SAI_Block_x));


  SAI_Block_x->DR = Data;
}


void SAI_DMACmd(SAI_Block_TypeDef* SAI_Block_x, FunctionalState NewState)
{

  assert_param(IS_SAI_BLOCK_PERIPH(SAI_Block_x));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    SAI_Block_x->CR1 |= SAI_xCR1_DMAEN;
  }
  else
  {

    SAI_Block_x->CR1 &= ~(SAI_xCR1_DMAEN);
  }
}


void SAI_ITConfig(SAI_Block_TypeDef* SAI_Block_x, uint32_t SAI_IT, FunctionalState NewState)
{

  assert_param(IS_SAI_BLOCK_PERIPH(SAI_Block_x));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  assert_param(IS_SAI_BLOCK_CONFIG_IT(SAI_IT));

  if (NewState != DISABLE)
  {

    SAI_Block_x->IMR |= SAI_IT;
  }
  else
  {

    SAI_Block_x->IMR &= ~(SAI_IT);
  }
}


FlagStatus SAI_GetFlagStatus(SAI_Block_TypeDef* SAI_Block_x, uint32_t SAI_FLAG)
{
  FlagStatus bitstatus = RESET;


  assert_param(IS_SAI_BLOCK_PERIPH(SAI_Block_x));
  assert_param(IS_SAI_BLOCK_GET_FLAG(SAI_FLAG));


  if ((SAI_Block_x->SR & SAI_FLAG) != (uint32_t)RESET)
  {

    bitstatus = SET;
  }
  else
  {

    bitstatus = RESET;
  }

  return  bitstatus;
}


void SAI_ClearFlag(SAI_Block_TypeDef* SAI_Block_x, uint32_t SAI_FLAG)
{

  assert_param(IS_SAI_BLOCK_PERIPH(SAI_Block_x));
  assert_param(IS_SAI_BLOCK_CLEAR_FLAG(SAI_FLAG));


  SAI_Block_x->CLRFR |= SAI_FLAG;
}


ITStatus SAI_GetITStatus(SAI_Block_TypeDef* SAI_Block_x, uint32_t SAI_IT)
{
  ITStatus bitstatus = RESET;
  uint32_t  enablestatus = 0;


  assert_param(IS_SAI_BLOCK_PERIPH(SAI_Block_x));
  assert_param(IS_SAI_BLOCK_CONFIG_IT(SAI_IT));


  enablestatus = (SAI_Block_x->IMR & SAI_IT) ;


  if (((SAI_Block_x->SR & SAI_IT) != (uint32_t)RESET) && (enablestatus != (uint32_t)RESET))
  {

    bitstatus = SET;
  }
  else
  {

    bitstatus = RESET;
  }

  return bitstatus;
}


void SAI_ClearITPendingBit(SAI_Block_TypeDef* SAI_Block_x, uint32_t SAI_IT)
{

  assert_param(IS_SAI_BLOCK_PERIPH(SAI_Block_x));
  assert_param(IS_SAI_BLOCK_CONFIG_IT(SAI_IT));


  SAI_Block_x->CLRFR |= SAI_IT;
}


FunctionalState SAI_GetCmdStatus(SAI_Block_TypeDef* SAI_Block_x)
{
  FunctionalState state = DISABLE;


  assert_param(IS_SAI_BLOCK_PERIPH(SAI_Block_x));
  if ((SAI_Block_x->CR1 & (uint32_t)SAI_xCR1_SAIEN) != 0)
  {

    state = ENABLE;
  }
  else
  {

    state = DISABLE;
  }
  return state;
}


uint32_t SAI_GetFIFOStatus(SAI_Block_TypeDef* SAI_Block_x)
{
  uint32_t tmpreg = 0;


  assert_param(IS_SAI_BLOCK_PERIPH(SAI_Block_x));


  tmpreg = (uint32_t)((SAI_Block_x->SR & SAI_xSR_FLVL));

  return tmpreg;
}
