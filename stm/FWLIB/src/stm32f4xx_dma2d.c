/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        stm32f4xx_dma2d.c
 *
 * Description:  stm32f4xx_dma2d.c module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#include "stm32f4xx_dma2d.h"
#include "stm32f4xx_rcc.h"


#define CR_MASK                     ((uint32_t)0xFFFCE0FC)
#define PFCCR_MASK                  ((uint32_t)0x00FC00C0)
#define DEAD_MASK                   ((uint32_t)0xFFFF00FE)


void DMA2D_DeInit(void)
{

  RCC_AHB1PeriphResetCmd(RCC_AHB1Periph_DMA2D, ENABLE);

  RCC_AHB1PeriphResetCmd(RCC_AHB1Periph_DMA2D, DISABLE);
}


void DMA2D_Init(DMA2D_InitTypeDef* DMA2D_InitStruct)
{

  uint32_t outgreen = 0;
  uint32_t outred   = 0;
  uint32_t outalpha = 0;
  uint32_t pixline  = 0;


  assert_param(IS_DMA2D_MODE(DMA2D_InitStruct->DMA2D_Mode));
  assert_param(IS_DMA2D_CMODE(DMA2D_InitStruct->DMA2D_CMode));
  assert_param(IS_DMA2D_OGREEN(DMA2D_InitStruct->DMA2D_OutputGreen));
  assert_param(IS_DMA2D_ORED(DMA2D_InitStruct->DMA2D_OutputRed));
  assert_param(IS_DMA2D_OBLUE(DMA2D_InitStruct->DMA2D_OutputBlue));
  assert_param(IS_DMA2D_OALPHA(DMA2D_InitStruct->DMA2D_OutputAlpha));
  assert_param(IS_DMA2D_OUTPUT_OFFSET(DMA2D_InitStruct->DMA2D_OutputOffset));
  assert_param(IS_DMA2D_LINE(DMA2D_InitStruct->DMA2D_NumberOfLine));
  assert_param(IS_DMA2D_PIXEL(DMA2D_InitStruct->DMA2D_PixelPerLine));


  DMA2D->CR &= (uint32_t)CR_MASK;
  DMA2D->CR |= (DMA2D_InitStruct->DMA2D_Mode);


  DMA2D->OPFCCR &= ~(uint32_t)DMA2D_OPFCCR_CM;
  DMA2D->OPFCCR |= (DMA2D_InitStruct->DMA2D_CMode);


  if (DMA2D_InitStruct->DMA2D_CMode == DMA2D_ARGB8888)
  {
    outgreen = DMA2D_InitStruct->DMA2D_OutputGreen << 8;
    outred = DMA2D_InitStruct->DMA2D_OutputRed << 16;
    outalpha = DMA2D_InitStruct->DMA2D_OutputAlpha << 24;
  }
  else

    if (DMA2D_InitStruct->DMA2D_CMode == DMA2D_RGB888)
    {
      outgreen = DMA2D_InitStruct->DMA2D_OutputGreen << 8;
      outred = DMA2D_InitStruct->DMA2D_OutputRed << 16;
      outalpha = (uint32_t)0x00000000;
    }

  else

    if (DMA2D_InitStruct->DMA2D_CMode == DMA2D_RGB565)
    {
      outgreen = DMA2D_InitStruct->DMA2D_OutputGreen << 5;
      outred = DMA2D_InitStruct->DMA2D_OutputRed << 11;
      outalpha = (uint32_t)0x00000000;
    }

  else

    if (DMA2D_InitStruct->DMA2D_CMode == DMA2D_ARGB1555)
    {
      outgreen = DMA2D_InitStruct->DMA2D_OutputGreen << 5;
      outred = DMA2D_InitStruct->DMA2D_OutputRed << 10;
      outalpha = DMA2D_InitStruct->DMA2D_OutputAlpha << 15;
    }

  else
  {
    outgreen = DMA2D_InitStruct->DMA2D_OutputGreen << 4;
    outred = DMA2D_InitStruct->DMA2D_OutputRed << 8;
    outalpha = DMA2D_InitStruct->DMA2D_OutputAlpha << 12;
  }
  DMA2D->OCOLR |= ((outgreen) | (outred) | (DMA2D_InitStruct->DMA2D_OutputBlue) | (outalpha));


  DMA2D->OMAR = (DMA2D_InitStruct->DMA2D_OutputMemoryAdd);


  DMA2D->OOR &= ~(uint32_t)DMA2D_OOR_LO;
  DMA2D->OOR |= (DMA2D_InitStruct->DMA2D_OutputOffset);


  pixline = DMA2D_InitStruct->DMA2D_PixelPerLine << 16;
  DMA2D->NLR &= ~(DMA2D_NLR_NL | DMA2D_NLR_PL);
  DMA2D->NLR |= ((DMA2D_InitStruct->DMA2D_NumberOfLine) | (pixline));


}
void DMA2D_StructInit(DMA2D_InitTypeDef* DMA2D_InitStruct)
{

  DMA2D_InitStruct->DMA2D_Mode = DMA2D_M2M;


  DMA2D_InitStruct->DMA2D_CMode = DMA2D_ARGB8888;


  DMA2D_InitStruct->DMA2D_OutputGreen = 0x00;
  DMA2D_InitStruct->DMA2D_OutputBlue = 0x00;
  DMA2D_InitStruct->DMA2D_OutputRed = 0x00;
  DMA2D_InitStruct->DMA2D_OutputAlpha = 0x00;


  DMA2D_InitStruct->DMA2D_OutputMemoryAdd = 0x00;


  DMA2D_InitStruct->DMA2D_OutputOffset = 0x00;


  DMA2D_InitStruct->DMA2D_NumberOfLine = 0x00;
  DMA2D_InitStruct->DMA2D_PixelPerLine = 0x00;
}


void DMA2D_StartTransfer(void)
{

    DMA2D->CR |= (uint32_t)DMA2D_CR_START;
}


void DMA2D_AbortTransfer(void)
{

    DMA2D->CR |= (uint32_t)DMA2D_CR_ABORT;

}


void DMA2D_Suspend(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    DMA2D->CR |= (uint32_t)DMA2D_CR_SUSP;
  }
  else
  {

    DMA2D->CR &= ~(uint32_t)DMA2D_CR_SUSP;
  }
}


void DMA2D_FGConfig(DMA2D_FG_InitTypeDef* DMA2D_FG_InitStruct)
{

  uint32_t fg_clutcolormode = 0;
  uint32_t fg_clutsize = 0;
  uint32_t fg_alpha_mode = 0;
  uint32_t fg_alphavalue = 0;
  uint32_t fg_colorgreen = 0;
  uint32_t fg_colorred = 0;

  assert_param(IS_DMA2D_FGO(DMA2D_FG_InitStruct->DMA2D_FGO));
  assert_param(IS_DMA2D_FGCM(DMA2D_FG_InitStruct->DMA2D_FGCM));
  assert_param(IS_DMA2D_FG_CLUT_CM(DMA2D_FG_InitStruct->DMA2D_FG_CLUT_CM));
  assert_param(IS_DMA2D_FG_CLUT_SIZE(DMA2D_FG_InitStruct->DMA2D_FG_CLUT_SIZE));
  assert_param(IS_DMA2D_FG_ALPHA_MODE(DMA2D_FG_InitStruct->DMA2D_FGPFC_ALPHA_MODE));
  assert_param(IS_DMA2D_FG_ALPHA_VALUE(DMA2D_FG_InitStruct->DMA2D_FGPFC_ALPHA_VALUE));
  assert_param(IS_DMA2D_FGC_BLUE(DMA2D_FG_InitStruct->DMA2D_FGC_BLUE));
  assert_param(IS_DMA2D_FGC_GREEN(DMA2D_FG_InitStruct->DMA2D_FGC_GREEN));
  assert_param(IS_DMA2D_FGC_RED(DMA2D_FG_InitStruct->DMA2D_FGC_RED));


  DMA2D->FGMAR = (DMA2D_FG_InitStruct->DMA2D_FGMA);


  DMA2D->FGOR &= ~(uint32_t)DMA2D_FGOR_LO;
  DMA2D->FGOR |= (DMA2D_FG_InitStruct->DMA2D_FGO);


  DMA2D->FGPFCCR &= (uint32_t)PFCCR_MASK;
  fg_clutcolormode = DMA2D_FG_InitStruct->DMA2D_FG_CLUT_CM << 4;
  fg_clutsize = DMA2D_FG_InitStruct->DMA2D_FG_CLUT_SIZE << 8;
  fg_alpha_mode = DMA2D_FG_InitStruct->DMA2D_FGPFC_ALPHA_MODE << 16;
  fg_alphavalue = DMA2D_FG_InitStruct->DMA2D_FGPFC_ALPHA_VALUE << 24;
  DMA2D->FGPFCCR |= (DMA2D_FG_InitStruct->DMA2D_FGCM | fg_clutcolormode | fg_clutsize | \
                    fg_alpha_mode | fg_alphavalue);


  DMA2D->FGCOLR &= ~(DMA2D_FGCOLR_BLUE | DMA2D_FGCOLR_GREEN | DMA2D_FGCOLR_RED);
  fg_colorgreen = DMA2D_FG_InitStruct->DMA2D_FGC_GREEN << 8;
  fg_colorred = DMA2D_FG_InitStruct->DMA2D_FGC_RED << 16;
  DMA2D->FGCOLR |= (DMA2D_FG_InitStruct->DMA2D_FGC_BLUE | fg_colorgreen | fg_colorred);


  DMA2D->FGCMAR = DMA2D_FG_InitStruct->DMA2D_FGCMAR;
}


void DMA2D_FG_StructInit(DMA2D_FG_InitTypeDef* DMA2D_FG_InitStruct)
{

  DMA2D_FG_InitStruct->DMA2D_FGMA = 0x00;


  DMA2D_FG_InitStruct->DMA2D_FGO = 0x00;


  DMA2D_FG_InitStruct->DMA2D_FGCM = CM_ARGB8888;


  DMA2D_FG_InitStruct->DMA2D_FG_CLUT_CM = CLUT_CM_ARGB8888;


  DMA2D_FG_InitStruct->DMA2D_FG_CLUT_SIZE = 0x00;


  DMA2D_FG_InitStruct->DMA2D_FGPFC_ALPHA_MODE = NO_MODIF_ALPHA_VALUE;


  DMA2D_FG_InitStruct->DMA2D_FGPFC_ALPHA_VALUE = 0x00;


  DMA2D_FG_InitStruct->DMA2D_FGC_BLUE = 0x00;


  DMA2D_FG_InitStruct->DMA2D_FGC_GREEN = 0x00;


  DMA2D_FG_InitStruct->DMA2D_FGC_RED = 0x00;


  DMA2D_FG_InitStruct->DMA2D_FGCMAR = 0x00;
}


void DMA2D_BGConfig(DMA2D_BG_InitTypeDef* DMA2D_BG_InitStruct)
{

  uint32_t bg_clutcolormode = 0;
  uint32_t bg_clutsize = 0;
  uint32_t bg_alpha_mode = 0;
  uint32_t bg_alphavalue = 0;
  uint32_t bg_colorgreen = 0;
  uint32_t bg_colorred = 0;

  assert_param(IS_DMA2D_BGO(DMA2D_BG_InitStruct->DMA2D_BGO));
  assert_param(IS_DMA2D_BGCM(DMA2D_BG_InitStruct->DMA2D_BGCM));
  assert_param(IS_DMA2D_BG_CLUT_CM(DMA2D_BG_InitStruct->DMA2D_BG_CLUT_CM));
  assert_param(IS_DMA2D_BG_CLUT_SIZE(DMA2D_BG_InitStruct->DMA2D_BG_CLUT_SIZE));
  assert_param(IS_DMA2D_BG_ALPHA_MODE(DMA2D_BG_InitStruct->DMA2D_BGPFC_ALPHA_MODE));
  assert_param(IS_DMA2D_BG_ALPHA_VALUE(DMA2D_BG_InitStruct->DMA2D_BGPFC_ALPHA_VALUE));
  assert_param(IS_DMA2D_BGC_BLUE(DMA2D_BG_InitStruct->DMA2D_BGC_BLUE));
  assert_param(IS_DMA2D_BGC_GREEN(DMA2D_BG_InitStruct->DMA2D_BGC_GREEN));
  assert_param(IS_DMA2D_BGC_RED(DMA2D_BG_InitStruct->DMA2D_BGC_RED));


  DMA2D->BGMAR = (DMA2D_BG_InitStruct->DMA2D_BGMA);


  DMA2D->BGOR &= ~(uint32_t)DMA2D_BGOR_LO;
  DMA2D->BGOR |= (DMA2D_BG_InitStruct->DMA2D_BGO);


  DMA2D->BGPFCCR &= (uint32_t)PFCCR_MASK;
  bg_clutcolormode = DMA2D_BG_InitStruct->DMA2D_BG_CLUT_CM << 4;
  bg_clutsize = DMA2D_BG_InitStruct->DMA2D_BG_CLUT_SIZE << 8;
  bg_alpha_mode = DMA2D_BG_InitStruct->DMA2D_BGPFC_ALPHA_MODE << 16;
  bg_alphavalue = DMA2D_BG_InitStruct->DMA2D_BGPFC_ALPHA_VALUE << 24;
  DMA2D->BGPFCCR |= (DMA2D_BG_InitStruct->DMA2D_BGCM | bg_clutcolormode | bg_clutsize | \
                    bg_alpha_mode | bg_alphavalue);


  DMA2D->BGCOLR &= ~(DMA2D_BGCOLR_BLUE | DMA2D_BGCOLR_GREEN | DMA2D_BGCOLR_RED);
  bg_colorgreen = DMA2D_BG_InitStruct->DMA2D_BGC_GREEN << 8;
  bg_colorred = DMA2D_BG_InitStruct->DMA2D_BGC_RED << 16;
  DMA2D->BGCOLR |= (DMA2D_BG_InitStruct->DMA2D_BGC_BLUE | bg_colorgreen | bg_colorred);


  DMA2D->BGCMAR = DMA2D_BG_InitStruct->DMA2D_BGCMAR;

}


void DMA2D_BG_StructInit(DMA2D_BG_InitTypeDef* DMA2D_BG_InitStruct)
{

  DMA2D_BG_InitStruct->DMA2D_BGMA = 0x00;


  DMA2D_BG_InitStruct->DMA2D_BGO = 0x00;


  DMA2D_BG_InitStruct->DMA2D_BGCM = CM_ARGB8888;


  DMA2D_BG_InitStruct->DMA2D_BG_CLUT_CM = CLUT_CM_ARGB8888;


  DMA2D_BG_InitStruct->DMA2D_BG_CLUT_SIZE = 0x00;


  DMA2D_BG_InitStruct->DMA2D_BGPFC_ALPHA_MODE = NO_MODIF_ALPHA_VALUE;


  DMA2D_BG_InitStruct->DMA2D_BGPFC_ALPHA_VALUE = 0x00;


  DMA2D_BG_InitStruct->DMA2D_BGC_BLUE = 0x00;


  DMA2D_BG_InitStruct->DMA2D_BGC_GREEN = 0x00;


  DMA2D_BG_InitStruct->DMA2D_BGC_RED = 0x00;


  DMA2D_BG_InitStruct->DMA2D_BGCMAR = 0x00;
}


void DMA2D_FGStart(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    DMA2D->FGPFCCR |= DMA2D_FGPFCCR_START;
  }
  else
  {

    DMA2D->FGPFCCR &= (uint32_t)~DMA2D_FGPFCCR_START;
  }
}


void DMA2D_BGStart(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    DMA2D->BGPFCCR |= DMA2D_BGPFCCR_START;
  }
  else
  {

    DMA2D->BGPFCCR &= (uint32_t)~DMA2D_BGPFCCR_START;
  }
}


void DMA2D_DeadTimeConfig(uint32_t DMA2D_DeadTime, FunctionalState NewState)
{
   uint32_t DeadTime;


  assert_param(IS_DMA2D_DEAD_TIME(DMA2D_DeadTime));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    DMA2D->AMTCR &= (uint32_t)DEAD_MASK;
    DeadTime = DMA2D_DeadTime << 8;
    DMA2D->AMTCR |= (DeadTime | DMA2D_AMTCR_EN);
  }
  else
  {
     DMA2D->AMTCR &= ~(uint32_t)DMA2D_AMTCR_EN;
  }
}


void DMA2D_LineWatermarkConfig(uint32_t DMA2D_LWatermarkConfig)
{

  assert_param(IS_DMA2D_LineWatermark(DMA2D_LWatermarkConfig));


  DMA2D->LWR = (uint32_t)DMA2D_LWatermarkConfig;
}


void DMA2D_ITConfig(uint32_t DMA2D_IT, FunctionalState NewState)
{

  assert_param(IS_DMA2D_IT(DMA2D_IT));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    DMA2D->CR |= DMA2D_IT;
  }
  else
  {

    DMA2D->CR &= (uint32_t)~DMA2D_IT;
  }
}


FlagStatus DMA2D_GetFlagStatus(uint32_t DMA2D_FLAG)
{
  FlagStatus bitstatus = RESET;


  assert_param(IS_DMA2D_GET_FLAG(DMA2D_FLAG));


  if (((DMA2D->ISR) & DMA2D_FLAG) != (uint32_t)RESET)
  {

    bitstatus = SET;
  }
  else
  {

    bitstatus = RESET;
  }

  return bitstatus;
}


void DMA2D_ClearFlag(uint32_t DMA2D_FLAG)
{

  assert_param(IS_DMA2D_GET_FLAG(DMA2D_FLAG));


  DMA2D->IFCR = (uint32_t)DMA2D_FLAG;
}


ITStatus DMA2D_GetITStatus(uint32_t DMA2D_IT)
{
  ITStatus bitstatus = RESET;
  uint32_t DMA2D_IT_FLAG = DMA2D_IT >> 8;


  assert_param(IS_DMA2D_IT(DMA2D_IT));

  if ((DMA2D->ISR & DMA2D_IT_FLAG) != (uint32_t)RESET)
  {
    bitstatus = SET;
  }
  else
  {
    bitstatus = RESET;
  }

  if (((DMA2D->CR & DMA2D_IT) != (uint32_t)RESET) && (bitstatus != (uint32_t)RESET))
  {
    bitstatus = SET;
  }
  else
  {
    bitstatus = RESET;
  }
  return bitstatus;
}


void DMA2D_ClearITPendingBit(uint32_t DMA2D_IT)
{

  assert_param(IS_DMA2D_IT(DMA2D_IT));
  DMA2D_IT = DMA2D_IT >> 8;


  DMA2D->IFCR = (uint32_t)DMA2D_IT;
}
