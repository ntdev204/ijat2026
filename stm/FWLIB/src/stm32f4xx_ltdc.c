/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        stm32f4xx_ltdc.c
 *
 * Description:  stm32f4xx_ltdc.c module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#include "stm32f4xx_ltdc.h"
#include "stm32f4xx_rcc.h"


#define GCR_MASK                     ((uint32_t)0x0FFE888F)


void LTDC_DeInit(void)
{

  RCC_APB2PeriphResetCmd(RCC_APB2Periph_LTDC, ENABLE);

  RCC_APB2PeriphResetCmd(RCC_APB2Periph_LTDC, DISABLE);
}


void LTDC_Init(LTDC_InitTypeDef* LTDC_InitStruct)
{
  uint32_t horizontalsync = 0;
  uint32_t accumulatedHBP = 0;
  uint32_t accumulatedactiveW = 0;
  uint32_t totalwidth = 0;
  uint32_t backgreen = 0;
  uint32_t backred = 0;


  assert_param(IS_LTDC_HSYNC(LTDC_InitStruct->LTDC_HorizontalSync));
  assert_param(IS_LTDC_VSYNC(LTDC_InitStruct->LTDC_VerticalSync));
  assert_param(IS_LTDC_AHBP(LTDC_InitStruct->LTDC_AccumulatedHBP));
  assert_param(IS_LTDC_AVBP(LTDC_InitStruct->LTDC_AccumulatedVBP));
  assert_param(IS_LTDC_AAH(LTDC_InitStruct->LTDC_AccumulatedActiveH));
  assert_param(IS_LTDC_AAW(LTDC_InitStruct->LTDC_AccumulatedActiveW));
  assert_param(IS_LTDC_TOTALH(LTDC_InitStruct->LTDC_TotalHeigh));
  assert_param(IS_LTDC_TOTALW(LTDC_InitStruct->LTDC_TotalWidth));
  assert_param(IS_LTDC_HSPOL(LTDC_InitStruct->LTDC_HSPolarity));
  assert_param(IS_LTDC_VSPOL(LTDC_InitStruct->LTDC_VSPolarity));
  assert_param(IS_LTDC_DEPOL(LTDC_InitStruct->LTDC_DEPolarity));
  assert_param(IS_LTDC_PCPOL(LTDC_InitStruct->LTDC_PCPolarity));
  assert_param(IS_LTDC_BackBlueValue(LTDC_InitStruct->LTDC_BackgroundBlueValue));
  assert_param(IS_LTDC_BackGreenValue(LTDC_InitStruct->LTDC_BackgroundGreenValue));
  assert_param(IS_LTDC_BackRedValue(LTDC_InitStruct->LTDC_BackgroundRedValue));


  LTDC->SSCR &= ~(LTDC_SSCR_VSH | LTDC_SSCR_HSW);
  horizontalsync = (LTDC_InitStruct->LTDC_HorizontalSync << 16);
  LTDC->SSCR |= (horizontalsync | LTDC_InitStruct->LTDC_VerticalSync);


  LTDC->BPCR &= ~(LTDC_BPCR_AVBP | LTDC_BPCR_AHBP);
  accumulatedHBP = (LTDC_InitStruct->LTDC_AccumulatedHBP << 16);
  LTDC->BPCR |= (accumulatedHBP | LTDC_InitStruct->LTDC_AccumulatedVBP);


  LTDC->AWCR &= ~(LTDC_AWCR_AAH | LTDC_AWCR_AAW);
  accumulatedactiveW = (LTDC_InitStruct->LTDC_AccumulatedActiveW << 16);
  LTDC->AWCR |= (accumulatedactiveW | LTDC_InitStruct->LTDC_AccumulatedActiveH);


  LTDC->TWCR &= ~(LTDC_TWCR_TOTALH | LTDC_TWCR_TOTALW);
  totalwidth = (LTDC_InitStruct->LTDC_TotalWidth << 16);
  LTDC->TWCR |= (totalwidth | LTDC_InitStruct->LTDC_TotalHeigh);

  LTDC->GCR &= (uint32_t)GCR_MASK;
  LTDC->GCR |=  (uint32_t)(LTDC_InitStruct->LTDC_HSPolarity | LTDC_InitStruct->LTDC_VSPolarity | \
                           LTDC_InitStruct->LTDC_DEPolarity | LTDC_InitStruct->LTDC_PCPolarity);


  backgreen = (LTDC_InitStruct->LTDC_BackgroundGreenValue << 8);
  backred = (LTDC_InitStruct->LTDC_BackgroundRedValue << 16);

  LTDC->BCCR &= ~(LTDC_BCCR_BCBLUE | LTDC_BCCR_BCGREEN | LTDC_BCCR_BCRED);
  LTDC->BCCR |= (backred | backgreen | LTDC_InitStruct->LTDC_BackgroundBlueValue);
}


void LTDC_StructInit(LTDC_InitTypeDef* LTDC_InitStruct)
{

  LTDC_InitStruct->LTDC_HSPolarity = LTDC_HSPolarity_AL;
  LTDC_InitStruct->LTDC_VSPolarity = LTDC_VSPolarity_AL;
  LTDC_InitStruct->LTDC_DEPolarity = LTDC_DEPolarity_AL;
  LTDC_InitStruct->LTDC_PCPolarity = LTDC_PCPolarity_IPC;
  LTDC_InitStruct->LTDC_HorizontalSync = 0x00;
  LTDC_InitStruct->LTDC_VerticalSync = 0x00;
  LTDC_InitStruct->LTDC_AccumulatedHBP = 0x00;
  LTDC_InitStruct->LTDC_AccumulatedVBP = 0x00;
  LTDC_InitStruct->LTDC_AccumulatedActiveW = 0x00;
  LTDC_InitStruct->LTDC_AccumulatedActiveH = 0x00;
  LTDC_InitStruct->LTDC_TotalWidth = 0x00;
  LTDC_InitStruct->LTDC_TotalHeigh = 0x00;
  LTDC_InitStruct->LTDC_BackgroundRedValue = 0x00;
  LTDC_InitStruct->LTDC_BackgroundGreenValue = 0x00;
  LTDC_InitStruct->LTDC_BackgroundBlueValue = 0x00;
}


void LTDC_Cmd(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    LTDC->GCR |= (uint32_t)LTDC_GCR_LTDCEN;
  }
  else
  {

    LTDC->GCR &= ~(uint32_t)LTDC_GCR_LTDCEN;
  }
}


void LTDC_DitherCmd(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    LTDC->GCR |= (uint32_t)LTDC_GCR_DTEN;
  }
  else
  {

    LTDC->GCR &= ~(uint32_t)LTDC_GCR_DTEN;
  }
}


LTDC_RGBTypeDef LTDC_GetRGBWidth(void)
{
  LTDC_RGBTypeDef LTDC_RGB_InitStruct;

  LTDC->GCR &= (uint32_t)GCR_MASK;

  LTDC_RGB_InitStruct.LTDC_BlueWidth = (uint32_t)((LTDC->GCR >> 4) & 0x7);
  LTDC_RGB_InitStruct.LTDC_GreenWidth = (uint32_t)((LTDC->GCR >> 8) & 0x7);
  LTDC_RGB_InitStruct.LTDC_RedWidth = (uint32_t)((LTDC->GCR >> 12) & 0x7);

  return LTDC_RGB_InitStruct;
}


void LTDC_RGBStructInit(LTDC_RGBTypeDef* LTDC_RGB_InitStruct)
{
  LTDC_RGB_InitStruct->LTDC_BlueWidth = 0x02;
  LTDC_RGB_InitStruct->LTDC_GreenWidth = 0x02;
  LTDC_RGB_InitStruct->LTDC_RedWidth = 0x02;
}


void LTDC_LIPConfig(uint32_t LTDC_LIPositionConfig)
{

  assert_param(IS_LTDC_LIPOS(LTDC_LIPositionConfig));


  LTDC->LIPCR = (uint32_t)LTDC_LIPositionConfig;
}


void LTDC_ReloadConfig(uint32_t LTDC_Reload)
{

  assert_param(IS_LTDC_RELOAD(LTDC_Reload));


  LTDC->SRCR = (uint32_t)LTDC_Reload;
}


void LTDC_LayerInit(LTDC_Layer_TypeDef* LTDC_Layerx, LTDC_Layer_InitTypeDef* LTDC_Layer_InitStruct)
{

  uint32_t whsppos = 0;
  uint32_t wvsppos = 0;
  uint32_t dcgreen = 0;
  uint32_t dcred = 0;
  uint32_t dcalpha = 0;
  uint32_t cfbp = 0;


  assert_param(IS_LTDC_Pixelformat(LTDC_Layer_InitStruct->LTDC_PixelFormat));
  assert_param(IS_LTDC_BlendingFactor1(LTDC_Layer_InitStruct->LTDC_BlendingFactor_1));
  assert_param(IS_LTDC_BlendingFactor2(LTDC_Layer_InitStruct->LTDC_BlendingFactor_2));
  assert_param(IS_LTDC_HCONFIGST(LTDC_Layer_InitStruct->LTDC_HorizontalStart));
  assert_param(IS_LTDC_HCONFIGSP(LTDC_Layer_InitStruct->LTDC_HorizontalStop));
  assert_param(IS_LTDC_VCONFIGST(LTDC_Layer_InitStruct->LTDC_VerticalStart));
  assert_param(IS_LTDC_VCONFIGSP(LTDC_Layer_InitStruct->LTDC_VerticalStop));
  assert_param(IS_LTDC_DEFAULTCOLOR(LTDC_Layer_InitStruct->LTDC_DefaultColorBlue));
  assert_param(IS_LTDC_DEFAULTCOLOR(LTDC_Layer_InitStruct->LTDC_DefaultColorGreen));
  assert_param(IS_LTDC_DEFAULTCOLOR(LTDC_Layer_InitStruct->LTDC_DefaultColorRed));
  assert_param(IS_LTDC_DEFAULTCOLOR(LTDC_Layer_InitStruct->LTDC_DefaultColorAlpha));
  assert_param(IS_LTDC_CFBP(LTDC_Layer_InitStruct->LTDC_CFBPitch));
  assert_param(IS_LTDC_CFBLL(LTDC_Layer_InitStruct->LTDC_CFBLineLength));
  assert_param(IS_LTDC_CFBLNBR(LTDC_Layer_InitStruct->LTDC_CFBLineNumber));


  whsppos = LTDC_Layer_InitStruct->LTDC_HorizontalStop << 16;
  LTDC_Layerx->WHPCR &= ~(LTDC_LxWHPCR_WHSTPOS | LTDC_LxWHPCR_WHSPPOS);
  LTDC_Layerx->WHPCR = (LTDC_Layer_InitStruct->LTDC_HorizontalStart | whsppos);


  wvsppos = LTDC_Layer_InitStruct->LTDC_VerticalStop << 16;
  LTDC_Layerx->WVPCR &= ~(LTDC_LxWVPCR_WVSTPOS | LTDC_LxWVPCR_WVSPPOS);
  LTDC_Layerx->WVPCR  = (LTDC_Layer_InitStruct->LTDC_VerticalStart | wvsppos);


  LTDC_Layerx->PFCR &= ~(LTDC_LxPFCR_PF);
  LTDC_Layerx->PFCR = (LTDC_Layer_InitStruct->LTDC_PixelFormat);


  dcgreen = (LTDC_Layer_InitStruct->LTDC_DefaultColorGreen << 8);
  dcred = (LTDC_Layer_InitStruct->LTDC_DefaultColorRed << 16);
  dcalpha = (LTDC_Layer_InitStruct->LTDC_DefaultColorAlpha << 24);
  LTDC_Layerx->DCCR &=  ~(LTDC_LxDCCR_DCBLUE | LTDC_LxDCCR_DCGREEN | LTDC_LxDCCR_DCRED | LTDC_LxDCCR_DCALPHA);
  LTDC_Layerx->DCCR = (LTDC_Layer_InitStruct->LTDC_DefaultColorBlue | dcgreen | \
                        dcred | dcalpha);


  LTDC_Layerx->CACR &= ~(LTDC_LxCACR_CONSTA);
  LTDC_Layerx->CACR = (LTDC_Layer_InitStruct->LTDC_ConstantAlpha);


  LTDC_Layerx->BFCR &= ~(LTDC_LxBFCR_BF2 | LTDC_LxBFCR_BF1);
  LTDC_Layerx->BFCR = (LTDC_Layer_InitStruct->LTDC_BlendingFactor_1 | LTDC_Layer_InitStruct->LTDC_BlendingFactor_2);


  LTDC_Layerx->CFBAR &= ~(LTDC_LxCFBAR_CFBADD);
  LTDC_Layerx->CFBAR = (LTDC_Layer_InitStruct->LTDC_CFBStartAdress);


  cfbp = (LTDC_Layer_InitStruct->LTDC_CFBPitch << 16);
  LTDC_Layerx->CFBLR  &= ~(LTDC_LxCFBLR_CFBLL | LTDC_LxCFBLR_CFBP);
  LTDC_Layerx->CFBLR  = (LTDC_Layer_InitStruct->LTDC_CFBLineLength | cfbp);


  LTDC_Layerx->CFBLNR  &= ~(LTDC_LxCFBLNR_CFBLNBR);
  LTDC_Layerx->CFBLNR  = (LTDC_Layer_InitStruct->LTDC_CFBLineNumber);

}


void LTDC_LayerStructInit(LTDC_Layer_InitTypeDef * LTDC_Layer_InitStruct)
{


  LTDC_Layer_InitStruct->LTDC_HorizontalStart = 0x00;
  LTDC_Layer_InitStruct->LTDC_HorizontalStop = 0x00;


  LTDC_Layer_InitStruct->LTDC_VerticalStart = 0x00;
  LTDC_Layer_InitStruct->LTDC_VerticalStop = 0x00;


  LTDC_Layer_InitStruct->LTDC_PixelFormat = LTDC_Pixelformat_ARGB8888;


  LTDC_Layer_InitStruct->LTDC_ConstantAlpha = 0xFF;


  LTDC_Layer_InitStruct->LTDC_DefaultColorBlue = 0x00;
  LTDC_Layer_InitStruct->LTDC_DefaultColorGreen = 0x00;
  LTDC_Layer_InitStruct->LTDC_DefaultColorRed = 0x00;
  LTDC_Layer_InitStruct->LTDC_DefaultColorAlpha = 0x00;


  LTDC_Layer_InitStruct->LTDC_BlendingFactor_1 = LTDC_BlendingFactor1_PAxCA;
  LTDC_Layer_InitStruct->LTDC_BlendingFactor_2 = LTDC_BlendingFactor2_PAxCA;


  LTDC_Layer_InitStruct->LTDC_CFBStartAdress = 0x00;


  LTDC_Layer_InitStruct->LTDC_CFBLineLength = 0x00;
  LTDC_Layer_InitStruct->LTDC_CFBPitch = 0x00;


  LTDC_Layer_InitStruct->LTDC_CFBLineNumber = 0x00;
}


void LTDC_LayerCmd(LTDC_Layer_TypeDef* LTDC_Layerx, FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    LTDC_Layerx->CR |= (uint32_t)LTDC_LxCR_LEN;
  }
  else
  {

    LTDC_Layerx->CR &= ~(uint32_t)LTDC_LxCR_LEN;
  }
}


LTDC_PosTypeDef LTDC_GetPosStatus(void)
{
  LTDC_PosTypeDef LTDC_Pos_InitStruct;

  LTDC->CPSR &= ~(LTDC_CPSR_CYPOS | LTDC_CPSR_CXPOS);

  LTDC_Pos_InitStruct.LTDC_POSX = (uint32_t)(LTDC->CPSR >> 16);
  LTDC_Pos_InitStruct.LTDC_POSY = (uint32_t)(LTDC->CPSR & 0xFFFF);

  return LTDC_Pos_InitStruct;
}


void LTDC_PosStructInit(LTDC_PosTypeDef* LTDC_Pos_InitStruct)
{
  LTDC_Pos_InitStruct->LTDC_POSX = 0x00;
  LTDC_Pos_InitStruct->LTDC_POSY = 0x00;
}


FlagStatus LTDC_GetCDStatus(uint32_t LTDC_CD)
{
  FlagStatus bitstatus;


  assert_param(IS_LTDC_GET_CD(LTDC_CD));

  if ((LTDC->CDSR & LTDC_CD) != (uint32_t)RESET)
  {
    bitstatus = SET;
  }
  else
  {
    bitstatus = RESET;
  }
  return bitstatus;
}


void LTDC_ColorKeyingConfig(LTDC_Layer_TypeDef* LTDC_Layerx, LTDC_ColorKeying_InitTypeDef* LTDC_colorkeying_InitStruct, FunctionalState NewState)
{
  uint32_t ckgreen = 0;
  uint32_t ckred = 0;


  assert_param(IS_FUNCTIONAL_STATE(NewState));
  assert_param(IS_LTDC_CKEYING(LTDC_colorkeying_InitStruct->LTDC_ColorKeyBlue));
  assert_param(IS_LTDC_CKEYING(LTDC_colorkeying_InitStruct->LTDC_ColorKeyGreen));
  assert_param(IS_LTDC_CKEYING(LTDC_colorkeying_InitStruct->LTDC_ColorKeyRed));

  if (NewState != DISABLE)
  {

    LTDC_Layerx->CR |= (uint32_t)LTDC_LxCR_COLKEN;


    ckgreen = (LTDC_colorkeying_InitStruct->LTDC_ColorKeyGreen << 8);
    ckred = (LTDC_colorkeying_InitStruct->LTDC_ColorKeyRed << 16);
    LTDC_Layerx->CKCR  &= ~(LTDC_LxCKCR_CKBLUE | LTDC_LxCKCR_CKGREEN | LTDC_LxCKCR_CKRED);
    LTDC_Layerx->CKCR |= (LTDC_colorkeying_InitStruct->LTDC_ColorKeyBlue | ckgreen | ckred);
  }
  else
  {

    LTDC_Layerx->CR &= ~(uint32_t)LTDC_LxCR_COLKEN;
  }


  LTDC->SRCR = LTDC_IMReload;
}


void LTDC_ColorKeyingStructInit(LTDC_ColorKeying_InitTypeDef* LTDC_colorkeying_InitStruct)
{

  LTDC_colorkeying_InitStruct->LTDC_ColorKeyBlue = 0x00;
  LTDC_colorkeying_InitStruct->LTDC_ColorKeyGreen = 0x00;
  LTDC_colorkeying_InitStruct->LTDC_ColorKeyRed = 0x00;
}


void LTDC_CLUTCmd(LTDC_Layer_TypeDef* LTDC_Layerx, FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    LTDC_Layerx->CR |= (uint32_t)LTDC_LxCR_CLUTEN;
  }
  else
  {

    LTDC_Layerx->CR &= ~(uint32_t)LTDC_LxCR_CLUTEN;
  }


  LTDC->SRCR = LTDC_IMReload;
}


void LTDC_CLUTInit(LTDC_Layer_TypeDef* LTDC_Layerx, LTDC_CLUT_InitTypeDef* LTDC_CLUT_InitStruct)
{
  uint32_t green = 0;
  uint32_t red = 0;
  uint32_t clutadd = 0;


  assert_param(IS_LTDC_CLUTWR(LTDC_CLUT_InitStruct->LTDC_CLUTAdress));
  assert_param(IS_LTDC_CLUTWR(LTDC_CLUT_InitStruct->LTDC_RedValue));
  assert_param(IS_LTDC_CLUTWR(LTDC_CLUT_InitStruct->LTDC_GreenValue));
  assert_param(IS_LTDC_CLUTWR(LTDC_CLUT_InitStruct->LTDC_BlueValue));


  green = (LTDC_CLUT_InitStruct->LTDC_GreenValue << 8);
  red = (LTDC_CLUT_InitStruct->LTDC_RedValue << 16);
  clutadd = (LTDC_CLUT_InitStruct->LTDC_CLUTAdress << 24);
  LTDC_Layerx->CLUTWR  = (clutadd | LTDC_CLUT_InitStruct->LTDC_BlueValue | \
                              green | red);
}


void LTDC_CLUTStructInit(LTDC_CLUT_InitTypeDef* LTDC_CLUT_InitStruct)
{

  LTDC_CLUT_InitStruct->LTDC_CLUTAdress = 0x00;
  LTDC_CLUT_InitStruct->LTDC_BlueValue = 0x00;
  LTDC_CLUT_InitStruct->LTDC_GreenValue = 0x00;
  LTDC_CLUT_InitStruct->LTDC_RedValue = 0x00;
}


void LTDC_LayerPosition(LTDC_Layer_TypeDef* LTDC_Layerx, uint16_t OffsetX, uint16_t OffsetY)
{

  uint32_t tempreg, temp;
  uint32_t horizontal_start;
  uint32_t horizontal_stop;
  uint32_t vertical_start;
  uint32_t vertical_stop;

  LTDC_Layerx->WHPCR &= ~(LTDC_LxWHPCR_WHSTPOS | LTDC_LxWHPCR_WHSPPOS);
  LTDC_Layerx->WVPCR &= ~(LTDC_LxWVPCR_WVSTPOS | LTDC_LxWVPCR_WVSPPOS);


  tempreg = LTDC->BPCR;
  horizontal_start = (tempreg >> 16) + 1 + OffsetX;
  vertical_start = (tempreg & 0xFFFF) + 1 + OffsetY;


  tempreg = LTDC_Layerx->PFCR;

  if (tempreg == LTDC_Pixelformat_ARGB8888)
  {
    temp = 4;
  }
  else if (tempreg == LTDC_Pixelformat_RGB888)
  {
    temp = 3;
  }
  else if ((tempreg == LTDC_Pixelformat_ARGB4444) ||
          (tempreg == LTDC_Pixelformat_RGB565)    ||
          (tempreg == LTDC_Pixelformat_ARGB1555)  ||
          (tempreg == LTDC_Pixelformat_AL88))
  {
    temp = 2;
  }
  else
  {
    temp = 1;
  }

  tempreg = LTDC_Layerx->CFBLR;
  horizontal_stop = (((tempreg & 0x1FFF) - 3)/temp) + horizontal_start - 1;

  tempreg = LTDC_Layerx->CFBLNR;
  vertical_stop = (tempreg & 0x7FF) + vertical_start - 1;

  LTDC_Layerx->WHPCR = horizontal_start | (horizontal_stop << 16);
  LTDC_Layerx->WVPCR = vertical_start | (vertical_stop << 16);
}


void LTDC_LayerAlpha(LTDC_Layer_TypeDef* LTDC_Layerx, uint8_t ConstantAlpha)
{

  LTDC_Layerx->CACR = ConstantAlpha;
}


void LTDC_LayerAddress(LTDC_Layer_TypeDef* LTDC_Layerx, uint32_t Address)
{

  LTDC_Layerx->CFBAR = Address;
}


void LTDC_LayerSize(LTDC_Layer_TypeDef* LTDC_Layerx, uint32_t Width, uint32_t Height)
{

  uint8_t temp;
  uint32_t tempreg;
  uint32_t horizontal_start;
  uint32_t horizontal_stop;
  uint32_t vertical_start;
  uint32_t vertical_stop;

  tempreg = LTDC_Layerx->PFCR;

  if (tempreg == LTDC_Pixelformat_ARGB8888)
  {
    temp = 4;
  }
  else if (tempreg == LTDC_Pixelformat_RGB888)
  {
    temp = 3;
  }
  else if ((tempreg == LTDC_Pixelformat_ARGB4444) || \
          (tempreg == LTDC_Pixelformat_RGB565)    || \
          (tempreg == LTDC_Pixelformat_ARGB1555)  || \
          (tempreg == LTDC_Pixelformat_AL88))
  {
    temp = 2;
  }
  else
  {
    temp = 1;
  }


  tempreg = LTDC_Layerx->WHPCR;
  horizontal_start = (tempreg & 0x1FFF);
  horizontal_stop = Width + horizontal_start - 1;

  tempreg = LTDC_Layerx->WVPCR;
  vertical_start = (tempreg & 0x1FFF);
  vertical_stop = Height + vertical_start - 1;

  LTDC_Layerx->WHPCR = horizontal_start | (horizontal_stop << 16);
  LTDC_Layerx->WVPCR = vertical_start | (vertical_stop << 16);


  LTDC_Layerx->CFBLR  = ((Width * temp) << 16) | ((Width * temp) + 3);


  LTDC_Layerx->CFBLNR  = Height;

}


void LTDC_LayerPixelFormat(LTDC_Layer_TypeDef* LTDC_Layerx, uint32_t PixelFormat)
{

  uint8_t temp;
  uint32_t tempreg;

  tempreg = LTDC_Layerx->PFCR;

  if (tempreg == LTDC_Pixelformat_ARGB8888)
  {
    temp = 4;
  }
  else if (tempreg == LTDC_Pixelformat_RGB888)
  {
    temp = 3;
  }
  else if ((tempreg == LTDC_Pixelformat_ARGB4444) || \
          (tempreg == LTDC_Pixelformat_RGB565)    || \
          (tempreg == LTDC_Pixelformat_ARGB1555)  || \
          (tempreg == LTDC_Pixelformat_AL88))
  {
    temp = 2;
  }
  else
  {
    temp = 1;
  }

  tempreg = (LTDC_Layerx->CFBLR >> 16);
  tempreg = (tempreg / temp);

  if (PixelFormat == LTDC_Pixelformat_ARGB8888)
  {
    temp = 4;
  }
  else if (PixelFormat == LTDC_Pixelformat_RGB888)
  {
    temp = 3;
  }
  else if ((PixelFormat == LTDC_Pixelformat_ARGB4444) || \
          (PixelFormat == LTDC_Pixelformat_RGB565)    || \
          (PixelFormat == LTDC_Pixelformat_ARGB1555)  || \
          (PixelFormat == LTDC_Pixelformat_AL88))
  {
    temp = 2;
  }
  else
  {
    temp = 1;
  }


  LTDC_Layerx->CFBLR  = ((tempreg * temp) << 16) | ((tempreg * temp) + 3);


  LTDC_Layerx->PFCR = PixelFormat;

}


void LTDC_ITConfig(uint32_t LTDC_IT, FunctionalState NewState)
{

  assert_param(IS_LTDC_IT(LTDC_IT));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {
    LTDC->IER |= LTDC_IT;
  }
  else
  {
    LTDC->IER &= (uint32_t)~LTDC_IT;
  }
}


FlagStatus LTDC_GetFlagStatus(uint32_t LTDC_FLAG)
{
  FlagStatus bitstatus = RESET;


  assert_param(IS_LTDC_FLAG(LTDC_FLAG));

  if ((LTDC->ISR & LTDC_FLAG) != (uint32_t)RESET)
  {
    bitstatus = SET;
  }
  else
  {
    bitstatus = RESET;
  }
  return bitstatus;
}


void LTDC_ClearFlag(uint32_t LTDC_FLAG)
{

  assert_param(IS_LTDC_FLAG(LTDC_FLAG));


  LTDC->ICR = (uint32_t)LTDC_FLAG;
}


ITStatus LTDC_GetITStatus(uint32_t LTDC_IT)
{
  ITStatus bitstatus = RESET;


  assert_param(IS_LTDC_IT(LTDC_IT));

  if ((LTDC->ISR & LTDC_IT) != (uint32_t)RESET)
  {
    bitstatus = SET;
  }
  else
  {
    bitstatus = RESET;
  }

  if (((LTDC->IER & LTDC_IT) != (uint32_t)RESET) && (bitstatus != (uint32_t)RESET))
  {
    bitstatus = SET;
  }
  else
  {
    bitstatus = RESET;
  }
  return bitstatus;
}


void LTDC_ClearITPendingBit(uint32_t LTDC_IT)
{

  assert_param(IS_LTDC_IT(LTDC_IT));


  LTDC->ICR = (uint32_t)LTDC_IT;
}
