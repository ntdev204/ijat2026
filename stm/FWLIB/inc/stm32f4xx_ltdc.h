/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        stm32f4xx_ltdc.h
 *
 * Description:  stm32f4xx_ltdc.h module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#ifndef __STM32F4xx_LTDC_H
#define __STM32F4xx_LTDC_H

#ifdef __cplusplus
 extern "C" {
#endif


#include "stm32f4xx.h"


typedef struct
{
  uint32_t LTDC_HSPolarity;

  uint32_t LTDC_VSPolarity;

  uint32_t LTDC_DEPolarity;

  uint32_t LTDC_PCPolarity;

  uint32_t LTDC_HorizontalSync;

  uint32_t LTDC_VerticalSync;

  uint32_t LTDC_AccumulatedHBP;

  uint32_t LTDC_AccumulatedVBP;

  uint32_t LTDC_AccumulatedActiveW;

  uint32_t LTDC_AccumulatedActiveH;

  uint32_t LTDC_TotalWidth;

  uint32_t LTDC_TotalHeigh;

  uint32_t LTDC_BackgroundRedValue;

  uint32_t LTDC_BackgroundGreenValue;

   uint32_t LTDC_BackgroundBlueValue;
} LTDC_InitTypeDef;


typedef struct
{
  uint32_t LTDC_HorizontalStart;

  uint32_t LTDC_HorizontalStop;

  uint32_t LTDC_VerticalStart;

  uint32_t LTDC_VerticalStop;

  uint32_t LTDC_PixelFormat;

  uint32_t LTDC_ConstantAlpha;

  uint32_t LTDC_DefaultColorBlue;

  uint32_t LTDC_DefaultColorGreen;

  uint32_t LTDC_DefaultColorRed;

  uint32_t LTDC_DefaultColorAlpha;

  uint32_t LTDC_BlendingFactor_1;

  uint32_t LTDC_BlendingFactor_2;

  uint32_t LTDC_CFBStartAdress;

  uint32_t LTDC_CFBLineLength;

  uint32_t LTDC_CFBPitch;

  uint32_t LTDC_CFBLineNumber;
} LTDC_Layer_InitTypeDef;


typedef struct
{
  uint32_t LTDC_POSX;
  uint32_t LTDC_POSY;
} LTDC_PosTypeDef;

typedef struct
{
  uint32_t LTDC_BlueWidth;
  uint32_t LTDC_GreenWidth;
  uint32_t LTDC_RedWidth;
} LTDC_RGBTypeDef;

typedef struct
{
  uint32_t LTDC_ColorKeyBlue;

  uint32_t LTDC_ColorKeyGreen;

  uint32_t LTDC_ColorKeyRed;
} LTDC_ColorKeying_InitTypeDef;

typedef struct
{
  uint32_t LTDC_CLUTAdress;

  uint32_t LTDC_BlueValue;

  uint32_t LTDC_GreenValue;

  uint32_t LTDC_RedValue;
} LTDC_CLUT_InitTypeDef;


#define LTDC_HorizontalSYNC               ((uint32_t)0x00000FFF)
#define LTDC_VerticalSYNC                 ((uint32_t)0x000007FF)

#define IS_LTDC_HSYNC(HSYNC) ((HSYNC) <= LTDC_HorizontalSYNC)
#define IS_LTDC_VSYNC(VSYNC) ((VSYNC) <= LTDC_VerticalSYNC)
#define IS_LTDC_AHBP(AHBP)  ((AHBP) <= LTDC_HorizontalSYNC)
#define IS_LTDC_AVBP(AVBP) ((AVBP) <= LTDC_VerticalSYNC)
#define IS_LTDC_AAW(AAW)   ((AAW) <= LTDC_HorizontalSYNC)
#define IS_LTDC_AAH(AAH) ((AAH) <= LTDC_VerticalSYNC)
#define IS_LTDC_TOTALW(TOTALW) ((TOTALW) <= LTDC_HorizontalSYNC)
#define IS_LTDC_TOTALH(TOTALH) ((TOTALH) <= LTDC_VerticalSYNC)


#define LTDC_HSPolarity_AL                ((uint32_t)0x00000000)
#define LTDC_HSPolarity_AH                LTDC_GCR_HSPOL

#define IS_LTDC_HSPOL(HSPOL) (((HSPOL) == LTDC_HSPolarity_AL) || \
                              ((HSPOL) == LTDC_HSPolarity_AH))


#define LTDC_VSPolarity_AL                ((uint32_t)0x00000000)
#define LTDC_VSPolarity_AH                LTDC_GCR_VSPOL

#define IS_LTDC_VSPOL(VSPOL) (((VSPOL) == LTDC_VSPolarity_AL) || \
                              ((VSPOL) == LTDC_VSPolarity_AH))


#define LTDC_DEPolarity_AL                ((uint32_t)0x00000000)
#define LTDC_DEPolarity_AH                LTDC_GCR_DEPOL

#define IS_LTDC_DEPOL(DEPOL) (((DEPOL) ==  LTDC_VSPolarity_AL) || \
                              ((DEPOL) ==  LTDC_DEPolarity_AH))


#define LTDC_PCPolarity_IPC               ((uint32_t)0x00000000)
#define LTDC_PCPolarity_IIPC              LTDC_GCR_PCPOL

#define IS_LTDC_PCPOL(PCPOL) (((PCPOL) ==  LTDC_PCPolarity_IPC) || \
                              ((PCPOL) ==  LTDC_PCPolarity_IIPC))


#define LTDC_IMReload                     LTDC_SRCR_IMR
#define LTDC_VBReload                     LTDC_SRCR_VBR

#define IS_LTDC_RELOAD(RELOAD) (((RELOAD) == LTDC_IMReload) || \
                                ((RELOAD) == LTDC_VBReload))


#define LTDC_Back_Color                   ((uint32_t)0x000000FF)

#define IS_LTDC_BackBlueValue(BBLUE)    ((BBLUE) <= LTDC_Back_Color)
#define IS_LTDC_BackGreenValue(BGREEN)  ((BGREEN) <= LTDC_Back_Color)
#define IS_LTDC_BackRedValue(BRED)      ((BRED) <= LTDC_Back_Color)


#define LTDC_POS_CY                       LTDC_CPSR_CYPOS
#define LTDC_POS_CX                       LTDC_CPSR_CXPOS

#define IS_LTDC_GET_POS(POS) (((POS) <= LTDC_POS_CY))


#define IS_LTDC_LIPOS(LIPOS) ((LIPOS) <= 0x7FF)


#define LTDC_CD_VDES                     LTDC_CDSR_VDES
#define LTDC_CD_HDES                     LTDC_CDSR_HDES
#define LTDC_CD_VSYNC                    LTDC_CDSR_VSYNCS
#define LTDC_CD_HSYNC                    LTDC_CDSR_HSYNCS


#define IS_LTDC_GET_CD(CD) (((CD) == LTDC_CD_VDES) || ((CD) == LTDC_CD_HDES) || \
                              ((CD) == LTDC_CD_VSYNC) || ((CD) == LTDC_CD_HSYNC))


#define LTDC_IT_LI                      LTDC_IER_LIE
#define LTDC_IT_FU                      LTDC_IER_FUIE
#define LTDC_IT_TERR                    LTDC_IER_TERRIE
#define LTDC_IT_RR                      LTDC_IER_RRIE

#define IS_LTDC_IT(IT) ((((IT) & (uint32_t)0xFFFFFFF0) == 0x00) && ((IT) != 0x00))


#define LTDC_FLAG_LI                     LTDC_ISR_LIF
#define LTDC_FLAG_FU                     LTDC_ISR_FUIF
#define LTDC_FLAG_TERR                   LTDC_ISR_TERRIF
#define LTDC_FLAG_RR                     LTDC_ISR_RRIF


#define IS_LTDC_FLAG(FLAG) (((FLAG) == LTDC_FLAG_LI) || ((FLAG) == LTDC_FLAG_FU) || \
                               ((FLAG) == LTDC_FLAG_TERR) || ((FLAG) == LTDC_FLAG_RR))


#define LTDC_Pixelformat_ARGB8888                  ((uint32_t)0x00000000)
#define LTDC_Pixelformat_RGB888                    ((uint32_t)0x00000001)
#define LTDC_Pixelformat_RGB565                    ((uint32_t)0x00000002)
#define LTDC_Pixelformat_ARGB1555                  ((uint32_t)0x00000003)
#define LTDC_Pixelformat_ARGB4444                  ((uint32_t)0x00000004)
#define LTDC_Pixelformat_L8                        ((uint32_t)0x00000005)
#define LTDC_Pixelformat_AL44                      ((uint32_t)0x00000006)
#define LTDC_Pixelformat_AL88                      ((uint32_t)0x00000007)

#define IS_LTDC_Pixelformat(Pixelformat) (((Pixelformat) == LTDC_Pixelformat_ARGB8888) || ((Pixelformat) == LTDC_Pixelformat_RGB888)   || \
                        ((Pixelformat) == LTDC_Pixelformat_RGB565)   || ((Pixelformat) == LTDC_Pixelformat_ARGB1555) || \
                        ((Pixelformat) == LTDC_Pixelformat_ARGB4444) || ((Pixelformat) == LTDC_Pixelformat_L8)       || \
                        ((Pixelformat) == LTDC_Pixelformat_AL44)     || ((Pixelformat) == LTDC_Pixelformat_AL88))


#define LTDC_BlendingFactor1_CA                       ((uint32_t)0x00000400)
#define LTDC_BlendingFactor1_PAxCA                    ((uint32_t)0x00000600)

#define IS_LTDC_BlendingFactor1(BlendingFactor1) (((BlendingFactor1) == LTDC_BlendingFactor1_CA) || ((BlendingFactor1) == LTDC_BlendingFactor1_PAxCA))


#define LTDC_BlendingFactor2_CA                       ((uint32_t)0x00000005)
#define LTDC_BlendingFactor2_PAxCA                    ((uint32_t)0x00000007)

#define IS_LTDC_BlendingFactor2(BlendingFactor2) (((BlendingFactor2) == LTDC_BlendingFactor2_CA) || ((BlendingFactor2) == LTDC_BlendingFactor2_PAxCA))


#define LTDC_STOPPosition                 ((uint32_t)0x0000FFFF)
#define LTDC_STARTPosition                ((uint32_t)0x00000FFF)

#define LTDC_DefaultColorConfig           ((uint32_t)0x000000FF)
#define LTDC_ColorFrameBuffer             ((uint32_t)0x00001FFF)
#define LTDC_LineNumber                   ((uint32_t)0x000007FF)

#define IS_LTDC_HCONFIGST(HCONFIGST) ((HCONFIGST) <= LTDC_STARTPosition)
#define IS_LTDC_HCONFIGSP(HCONFIGSP) ((HCONFIGSP) <= LTDC_STOPPosition)
#define IS_LTDC_VCONFIGST(VCONFIGST)  ((VCONFIGST) <= LTDC_STARTPosition)
#define IS_LTDC_VCONFIGSP(VCONFIGSP) ((VCONFIGSP) <= LTDC_STOPPosition)

#define IS_LTDC_DEFAULTCOLOR(DEFAULTCOLOR) ((DEFAULTCOLOR) <= LTDC_DefaultColorConfig)

#define IS_LTDC_CFBP(CFBP) ((CFBP) <= LTDC_ColorFrameBuffer)
#define IS_LTDC_CFBLL(CFBLL) ((CFBLL) <= LTDC_ColorFrameBuffer)

#define IS_LTDC_CFBLNBR(CFBLNBR) ((CFBLNBR) <= LTDC_LineNumber)


#define LTDC_colorkeyingConfig            ((uint32_t)0x000000FF)

#define IS_LTDC_CKEYING(CKEYING) ((CKEYING) <= LTDC_colorkeyingConfig)


#define LTDC_CLUTWR                       ((uint32_t)0x000000FF)

#define IS_LTDC_CLUTWR(CLUTWR)  ((CLUTWR) <= LTDC_CLUTWR)


void LTDC_DeInit(void);


void LTDC_Init(LTDC_InitTypeDef* LTDC_InitStruct);
void LTDC_StructInit(LTDC_InitTypeDef* LTDC_InitStruct);
void LTDC_Cmd(FunctionalState NewState);
void LTDC_DitherCmd(FunctionalState NewState);
LTDC_RGBTypeDef LTDC_GetRGBWidth(void);
void LTDC_RGBStructInit(LTDC_RGBTypeDef* LTDC_RGB_InitStruct);
void LTDC_LIPConfig(uint32_t LTDC_LIPositionConfig);
void LTDC_ReloadConfig(uint32_t LTDC_Reload);
void LTDC_LayerInit(LTDC_Layer_TypeDef* LTDC_Layerx, LTDC_Layer_InitTypeDef* LTDC_Layer_InitStruct);
void LTDC_LayerStructInit(LTDC_Layer_InitTypeDef * LTDC_Layer_InitStruct);
void LTDC_LayerCmd(LTDC_Layer_TypeDef* LTDC_Layerx, FunctionalState NewState);
LTDC_PosTypeDef LTDC_GetPosStatus(void);
void LTDC_PosStructInit(LTDC_PosTypeDef* LTDC_Pos_InitStruct);
FlagStatus LTDC_GetCDStatus(uint32_t LTDC_CD);
void LTDC_ColorKeyingConfig(LTDC_Layer_TypeDef* LTDC_Layerx, LTDC_ColorKeying_InitTypeDef* LTDC_colorkeying_InitStruct, FunctionalState NewState);
void LTDC_ColorKeyingStructInit(LTDC_ColorKeying_InitTypeDef* LTDC_colorkeying_InitStruct);
void LTDC_CLUTCmd(LTDC_Layer_TypeDef* LTDC_Layerx, FunctionalState NewState);
void LTDC_CLUTInit(LTDC_Layer_TypeDef* LTDC_Layerx, LTDC_CLUT_InitTypeDef* LTDC_CLUT_InitStruct);
void LTDC_CLUTStructInit(LTDC_CLUT_InitTypeDef* LTDC_CLUT_InitStruct);
void LTDC_LayerPosition(LTDC_Layer_TypeDef* LTDC_Layerx, uint16_t OffsetX, uint16_t OffsetY);
void LTDC_LayerAlpha(LTDC_Layer_TypeDef* LTDC_Layerx, uint8_t ConstantAlpha);
void LTDC_LayerAddress(LTDC_Layer_TypeDef* LTDC_Layerx, uint32_t Address);
void LTDC_LayerSize(LTDC_Layer_TypeDef* LTDC_Layerx, uint32_t Width, uint32_t Height);
void LTDC_LayerPixelFormat(LTDC_Layer_TypeDef* LTDC_Layerx, uint32_t PixelFormat);


void LTDC_ITConfig(uint32_t LTDC_IT, FunctionalState NewState);
FlagStatus LTDC_GetFlagStatus(uint32_t LTDC_FLAG);
void LTDC_ClearFlag(uint32_t LTDC_FLAG);
ITStatus LTDC_GetITStatus(uint32_t LTDC_IT);
void LTDC_ClearITPendingBit(uint32_t LTDC_IT);

#ifdef __cplusplus
}
#endif

#endif
