/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        stm32f4xx_dma.c
 *
 * Description:  stm32f4xx_dma.c module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#include "stm32f4xx_dma.h"
#include "stm32f4xx_rcc.h"


#define TRANSFER_IT_ENABLE_MASK (uint32_t)(DMA_SxCR_TCIE | DMA_SxCR_HTIE | \
                                           DMA_SxCR_TEIE | DMA_SxCR_DMEIE)

#define DMA_Stream0_IT_MASK     (uint32_t)(DMA_LISR_FEIF0 | DMA_LISR_DMEIF0 | \
                                           DMA_LISR_TEIF0 | DMA_LISR_HTIF0 | \
                                           DMA_LISR_TCIF0)

#define DMA_Stream1_IT_MASK     (uint32_t)(DMA_Stream0_IT_MASK << 6)
#define DMA_Stream2_IT_MASK     (uint32_t)(DMA_Stream0_IT_MASK << 16)
#define DMA_Stream3_IT_MASK     (uint32_t)(DMA_Stream0_IT_MASK << 22)
#define DMA_Stream4_IT_MASK     (uint32_t)(DMA_Stream0_IT_MASK | (uint32_t)0x20000000)
#define DMA_Stream5_IT_MASK     (uint32_t)(DMA_Stream1_IT_MASK | (uint32_t)0x20000000)
#define DMA_Stream6_IT_MASK     (uint32_t)(DMA_Stream2_IT_MASK | (uint32_t)0x20000000)
#define DMA_Stream7_IT_MASK     (uint32_t)(DMA_Stream3_IT_MASK | (uint32_t)0x20000000)
#define TRANSFER_IT_MASK        (uint32_t)0x0F3C0F3C
#define HIGH_ISR_MASK           (uint32_t)0x20000000
#define RESERVED_MASK           (uint32_t)0x0F7D0F7D


void DMA_DeInit(DMA_Stream_TypeDef* DMAy_Streamx)
{

  assert_param(IS_DMA_ALL_PERIPH(DMAy_Streamx));


  DMAy_Streamx->CR &= ~((uint32_t)DMA_SxCR_EN);


  DMAy_Streamx->CR  = 0;


  DMAy_Streamx->NDTR = 0;


  DMAy_Streamx->PAR  = 0;


  DMAy_Streamx->M0AR = 0;


  DMAy_Streamx->M1AR = 0;


  DMAy_Streamx->FCR = (uint32_t)0x00000021;


  if (DMAy_Streamx == DMA1_Stream0)
  {

    DMA1->LIFCR = DMA_Stream0_IT_MASK;
  }
  else if (DMAy_Streamx == DMA1_Stream1)
  {

    DMA1->LIFCR = DMA_Stream1_IT_MASK;
  }
  else if (DMAy_Streamx == DMA1_Stream2)
  {

    DMA1->LIFCR = DMA_Stream2_IT_MASK;
  }
  else if (DMAy_Streamx == DMA1_Stream3)
  {

    DMA1->LIFCR = DMA_Stream3_IT_MASK;
  }
  else if (DMAy_Streamx == DMA1_Stream4)
  {

    DMA1->HIFCR = DMA_Stream4_IT_MASK;
  }
  else if (DMAy_Streamx == DMA1_Stream5)
  {

    DMA1->HIFCR = DMA_Stream5_IT_MASK;
  }
  else if (DMAy_Streamx == DMA1_Stream6)
  {

    DMA1->HIFCR = (uint32_t)DMA_Stream6_IT_MASK;
  }
  else if (DMAy_Streamx == DMA1_Stream7)
  {

    DMA1->HIFCR = DMA_Stream7_IT_MASK;
  }
  else if (DMAy_Streamx == DMA2_Stream0)
  {

    DMA2->LIFCR = DMA_Stream0_IT_MASK;
  }
  else if (DMAy_Streamx == DMA2_Stream1)
  {

    DMA2->LIFCR = DMA_Stream1_IT_MASK;
  }
  else if (DMAy_Streamx == DMA2_Stream2)
  {

    DMA2->LIFCR = DMA_Stream2_IT_MASK;
  }
  else if (DMAy_Streamx == DMA2_Stream3)
  {

    DMA2->LIFCR = DMA_Stream3_IT_MASK;
  }
  else if (DMAy_Streamx == DMA2_Stream4)
  {

    DMA2->HIFCR = DMA_Stream4_IT_MASK;
  }
  else if (DMAy_Streamx == DMA2_Stream5)
  {

    DMA2->HIFCR = DMA_Stream5_IT_MASK;
  }
  else if (DMAy_Streamx == DMA2_Stream6)
  {

    DMA2->HIFCR = DMA_Stream6_IT_MASK;
  }
  else
  {
    if (DMAy_Streamx == DMA2_Stream7)
    {

      DMA2->HIFCR = DMA_Stream7_IT_MASK;
    }
  }
}


void DMA_Init(DMA_Stream_TypeDef* DMAy_Streamx, DMA_InitTypeDef* DMA_InitStruct)
{
  uint32_t tmpreg = 0;


  assert_param(IS_DMA_ALL_PERIPH(DMAy_Streamx));
  assert_param(IS_DMA_CHANNEL(DMA_InitStruct->DMA_Channel));
  assert_param(IS_DMA_DIRECTION(DMA_InitStruct->DMA_DIR));
  assert_param(IS_DMA_BUFFER_SIZE(DMA_InitStruct->DMA_BufferSize));
  assert_param(IS_DMA_PERIPHERAL_INC_STATE(DMA_InitStruct->DMA_PeripheralInc));
  assert_param(IS_DMA_MEMORY_INC_STATE(DMA_InitStruct->DMA_MemoryInc));
  assert_param(IS_DMA_PERIPHERAL_DATA_SIZE(DMA_InitStruct->DMA_PeripheralDataSize));
  assert_param(IS_DMA_MEMORY_DATA_SIZE(DMA_InitStruct->DMA_MemoryDataSize));
  assert_param(IS_DMA_MODE(DMA_InitStruct->DMA_Mode));
  assert_param(IS_DMA_PRIORITY(DMA_InitStruct->DMA_Priority));
  assert_param(IS_DMA_FIFO_MODE_STATE(DMA_InitStruct->DMA_FIFOMode));
  assert_param(IS_DMA_FIFO_THRESHOLD(DMA_InitStruct->DMA_FIFOThreshold));
  assert_param(IS_DMA_MEMORY_BURST(DMA_InitStruct->DMA_MemoryBurst));
  assert_param(IS_DMA_PERIPHERAL_BURST(DMA_InitStruct->DMA_PeripheralBurst));


  tmpreg = DMAy_Streamx->CR;


  tmpreg &= ((uint32_t)~(DMA_SxCR_CHSEL | DMA_SxCR_MBURST | DMA_SxCR_PBURST | \
                         DMA_SxCR_PL | DMA_SxCR_MSIZE | DMA_SxCR_PSIZE | \
                         DMA_SxCR_MINC | DMA_SxCR_PINC | DMA_SxCR_CIRC | \
                         DMA_SxCR_DIR));


  tmpreg |= DMA_InitStruct->DMA_Channel | DMA_InitStruct->DMA_DIR |
            DMA_InitStruct->DMA_PeripheralInc | DMA_InitStruct->DMA_MemoryInc |
            DMA_InitStruct->DMA_PeripheralDataSize | DMA_InitStruct->DMA_MemoryDataSize |
            DMA_InitStruct->DMA_Mode | DMA_InitStruct->DMA_Priority |
            DMA_InitStruct->DMA_MemoryBurst | DMA_InitStruct->DMA_PeripheralBurst;


  DMAy_Streamx->CR = tmpreg;


  tmpreg = DMAy_Streamx->FCR;


  tmpreg &= (uint32_t)~(DMA_SxFCR_DMDIS | DMA_SxFCR_FTH);


  tmpreg |= DMA_InitStruct->DMA_FIFOMode | DMA_InitStruct->DMA_FIFOThreshold;


  DMAy_Streamx->FCR = tmpreg;


  DMAy_Streamx->NDTR = DMA_InitStruct->DMA_BufferSize;


  DMAy_Streamx->PAR = DMA_InitStruct->DMA_PeripheralBaseAddr;


  DMAy_Streamx->M0AR = DMA_InitStruct->DMA_Memory0BaseAddr;
}


void DMA_StructInit(DMA_InitTypeDef* DMA_InitStruct)
{


  DMA_InitStruct->DMA_Channel = 0;


  DMA_InitStruct->DMA_PeripheralBaseAddr = 0;


  DMA_InitStruct->DMA_Memory0BaseAddr = 0;


  DMA_InitStruct->DMA_DIR = DMA_DIR_PeripheralToMemory;


  DMA_InitStruct->DMA_BufferSize = 0;


  DMA_InitStruct->DMA_PeripheralInc = DMA_PeripheralInc_Disable;


  DMA_InitStruct->DMA_MemoryInc = DMA_MemoryInc_Disable;


  DMA_InitStruct->DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;


  DMA_InitStruct->DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;


  DMA_InitStruct->DMA_Mode = DMA_Mode_Normal;


  DMA_InitStruct->DMA_Priority = DMA_Priority_Low;


  DMA_InitStruct->DMA_FIFOMode = DMA_FIFOMode_Disable;


  DMA_InitStruct->DMA_FIFOThreshold = DMA_FIFOThreshold_1QuarterFull;


  DMA_InitStruct->DMA_MemoryBurst = DMA_MemoryBurst_Single;


  DMA_InitStruct->DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
}


void DMA_Cmd(DMA_Stream_TypeDef* DMAy_Streamx, FunctionalState NewState)
{

  assert_param(IS_DMA_ALL_PERIPH(DMAy_Streamx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    DMAy_Streamx->CR |= (uint32_t)DMA_SxCR_EN;
  }
  else
  {

    DMAy_Streamx->CR &= ~(uint32_t)DMA_SxCR_EN;
  }
}


void DMA_PeriphIncOffsetSizeConfig(DMA_Stream_TypeDef* DMAy_Streamx, uint32_t DMA_Pincos)
{

  assert_param(IS_DMA_ALL_PERIPH(DMAy_Streamx));
  assert_param(IS_DMA_PINCOS_SIZE(DMA_Pincos));


  if(DMA_Pincos != DMA_PINCOS_Psize)
  {

    DMAy_Streamx->CR |= (uint32_t)DMA_SxCR_PINCOS;
  }
  else
  {

    DMAy_Streamx->CR &= ~(uint32_t)DMA_SxCR_PINCOS;
  }
}


void DMA_FlowControllerConfig(DMA_Stream_TypeDef* DMAy_Streamx, uint32_t DMA_FlowCtrl)
{

  assert_param(IS_DMA_ALL_PERIPH(DMAy_Streamx));
  assert_param(IS_DMA_FLOW_CTRL(DMA_FlowCtrl));


  if(DMA_FlowCtrl != DMA_FlowCtrl_Memory)
  {

    DMAy_Streamx->CR |= (uint32_t)DMA_SxCR_PFCTRL;
  }
  else
  {

    DMAy_Streamx->CR &= ~(uint32_t)DMA_SxCR_PFCTRL;
  }
}


void DMA_SetCurrDataCounter(DMA_Stream_TypeDef* DMAy_Streamx, uint16_t Counter)
{

  assert_param(IS_DMA_ALL_PERIPH(DMAy_Streamx));


  DMAy_Streamx->NDTR = (uint16_t)Counter;
}


uint16_t DMA_GetCurrDataCounter(DMA_Stream_TypeDef* DMAy_Streamx)
{

  assert_param(IS_DMA_ALL_PERIPH(DMAy_Streamx));


  return ((uint16_t)(DMAy_Streamx->NDTR));
}


void DMA_DoubleBufferModeConfig(DMA_Stream_TypeDef* DMAy_Streamx, uint32_t Memory1BaseAddr,
                                uint32_t DMA_CurrentMemory)
{

  assert_param(IS_DMA_ALL_PERIPH(DMAy_Streamx));
  assert_param(IS_DMA_CURRENT_MEM(DMA_CurrentMemory));

  if (DMA_CurrentMemory != DMA_Memory_0)
  {

    DMAy_Streamx->CR |= (uint32_t)(DMA_SxCR_CT);
  }
  else
  {

    DMAy_Streamx->CR &= ~(uint32_t)(DMA_SxCR_CT);
  }


  DMAy_Streamx->M1AR = Memory1BaseAddr;
}


void DMA_DoubleBufferModeCmd(DMA_Stream_TypeDef* DMAy_Streamx, FunctionalState NewState)
{

  assert_param(IS_DMA_ALL_PERIPH(DMAy_Streamx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));


  if (NewState != DISABLE)
  {

    DMAy_Streamx->CR |= (uint32_t)DMA_SxCR_DBM;
  }
  else
  {

    DMAy_Streamx->CR &= ~(uint32_t)DMA_SxCR_DBM;
  }
}


void DMA_MemoryTargetConfig(DMA_Stream_TypeDef* DMAy_Streamx, uint32_t MemoryBaseAddr,
                           uint32_t DMA_MemoryTarget)
{

  assert_param(IS_DMA_ALL_PERIPH(DMAy_Streamx));
  assert_param(IS_DMA_CURRENT_MEM(DMA_MemoryTarget));


  if (DMA_MemoryTarget != DMA_Memory_0)
  {

    DMAy_Streamx->M1AR = MemoryBaseAddr;
  }
  else
  {

    DMAy_Streamx->M0AR = MemoryBaseAddr;
  }
}


uint32_t DMA_GetCurrentMemoryTarget(DMA_Stream_TypeDef* DMAy_Streamx)
{
  uint32_t tmp = 0;


  assert_param(IS_DMA_ALL_PERIPH(DMAy_Streamx));


  if ((DMAy_Streamx->CR & DMA_SxCR_CT) != 0)
  {

    tmp = 1;
  }
  else
  {

    tmp = 0;
  }
  return tmp;
}


FunctionalState DMA_GetCmdStatus(DMA_Stream_TypeDef* DMAy_Streamx)
{
  FunctionalState state = DISABLE;


  assert_param(IS_DMA_ALL_PERIPH(DMAy_Streamx));

  if ((DMAy_Streamx->CR & (uint32_t)DMA_SxCR_EN) != 0)
  {

    state = ENABLE;
  }
  else
  {

    state = DISABLE;
  }
  return state;
}


uint32_t DMA_GetFIFOStatus(DMA_Stream_TypeDef* DMAy_Streamx)
{
  uint32_t tmpreg = 0;


  assert_param(IS_DMA_ALL_PERIPH(DMAy_Streamx));


  tmpreg = (uint32_t)((DMAy_Streamx->FCR & DMA_SxFCR_FS));

  return tmpreg;
}


FlagStatus DMA_GetFlagStatus(DMA_Stream_TypeDef* DMAy_Streamx, uint32_t DMA_FLAG)
{
  FlagStatus bitstatus = RESET;
  DMA_TypeDef* DMAy;
  uint32_t tmpreg = 0;


  assert_param(IS_DMA_ALL_PERIPH(DMAy_Streamx));
  assert_param(IS_DMA_GET_FLAG(DMA_FLAG));


  if (DMAy_Streamx < DMA2_Stream0)
  {

    DMAy = DMA1;
  }
  else
  {

    DMAy = DMA2;
  }


  if ((DMA_FLAG & HIGH_ISR_MASK) != (uint32_t)RESET)
  {

    tmpreg = DMAy->HISR;
  }
  else
  {

    tmpreg = DMAy->LISR;
  }


  tmpreg &= (uint32_t)RESERVED_MASK;


  if ((tmpreg & DMA_FLAG) != (uint32_t)RESET)
  {

    bitstatus = SET;
  }
  else
  {

    bitstatus = RESET;
  }


  return  bitstatus;
}


void DMA_ClearFlag(DMA_Stream_TypeDef* DMAy_Streamx, uint32_t DMA_FLAG)
{
  DMA_TypeDef* DMAy;


  assert_param(IS_DMA_ALL_PERIPH(DMAy_Streamx));
  assert_param(IS_DMA_CLEAR_FLAG(DMA_FLAG));


  if (DMAy_Streamx < DMA2_Stream0)
  {

    DMAy = DMA1;
  }
  else
  {

    DMAy = DMA2;
  }


  if ((DMA_FLAG & HIGH_ISR_MASK) != (uint32_t)RESET)
  {

    DMAy->HIFCR = (uint32_t)(DMA_FLAG & RESERVED_MASK);
  }
  else
  {

    DMAy->LIFCR = (uint32_t)(DMA_FLAG & RESERVED_MASK);
  }
}


void DMA_ITConfig(DMA_Stream_TypeDef* DMAy_Streamx, uint32_t DMA_IT, FunctionalState NewState)
{

  assert_param(IS_DMA_ALL_PERIPH(DMAy_Streamx));
  assert_param(IS_DMA_CONFIG_IT(DMA_IT));
  assert_param(IS_FUNCTIONAL_STATE(NewState));


  if ((DMA_IT & DMA_IT_FE) != 0)
  {
    if (NewState != DISABLE)
    {

      DMAy_Streamx->FCR |= (uint32_t)DMA_IT_FE;
    }
    else
    {

      DMAy_Streamx->FCR &= ~(uint32_t)DMA_IT_FE;
    }
  }


  if (DMA_IT != DMA_IT_FE)
  {
    if (NewState != DISABLE)
    {

      DMAy_Streamx->CR |= (uint32_t)(DMA_IT  & TRANSFER_IT_ENABLE_MASK);
    }
    else
    {

      DMAy_Streamx->CR &= ~(uint32_t)(DMA_IT & TRANSFER_IT_ENABLE_MASK);
    }
  }
}


ITStatus DMA_GetITStatus(DMA_Stream_TypeDef* DMAy_Streamx, uint32_t DMA_IT)
{
  ITStatus bitstatus = RESET;
  DMA_TypeDef* DMAy;
  uint32_t tmpreg = 0, enablestatus = 0;


  assert_param(IS_DMA_ALL_PERIPH(DMAy_Streamx));
  assert_param(IS_DMA_GET_IT(DMA_IT));


  if (DMAy_Streamx < DMA2_Stream0)
  {

    DMAy = DMA1;
  }
  else
  {

    DMAy = DMA2;
  }


  if ((DMA_IT & TRANSFER_IT_MASK) != (uint32_t)RESET)
  {

    tmpreg = (uint32_t)((DMA_IT >> 11) & TRANSFER_IT_ENABLE_MASK);


    enablestatus = (uint32_t)(DMAy_Streamx->CR & tmpreg);
  }
  else
  {

    enablestatus = (uint32_t)(DMAy_Streamx->FCR & DMA_IT_FE);
  }


  if ((DMA_IT & HIGH_ISR_MASK) != (uint32_t)RESET)
  {

    tmpreg = DMAy->HISR ;
  }
  else
  {

    tmpreg = DMAy->LISR ;
  }


  tmpreg &= (uint32_t)RESERVED_MASK;


  if (((tmpreg & DMA_IT) != (uint32_t)RESET) && (enablestatus != (uint32_t)RESET))
  {

    bitstatus = SET;
  }
  else
  {

    bitstatus = RESET;
  }


  return  bitstatus;
}


void DMA_ClearITPendingBit(DMA_Stream_TypeDef* DMAy_Streamx, uint32_t DMA_IT)
{
  DMA_TypeDef* DMAy;


  assert_param(IS_DMA_ALL_PERIPH(DMAy_Streamx));
  assert_param(IS_DMA_CLEAR_IT(DMA_IT));


  if (DMAy_Streamx < DMA2_Stream0)
  {

    DMAy = DMA1;
  }
  else
  {

    DMAy = DMA2;
  }


  if ((DMA_IT & HIGH_ISR_MASK) != (uint32_t)RESET)
  {

    DMAy->HIFCR = (uint32_t)(DMA_IT & RESERVED_MASK);
  }
  else
  {

    DMAy->LIFCR = (uint32_t)(DMA_IT & RESERVED_MASK);
  }
}
