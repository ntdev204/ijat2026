/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        stm32f4xx_spi.c
 *
 * Description:  stm32f4xx_spi.c module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#include "stm32f4xx_spi.h"
#include "stm32f4xx_rcc.h"


#define CR1_CLEAR_MASK            ((uint16_t)0x3040)
#define I2SCFGR_CLEAR_MASK        ((uint16_t)0xF040)


#define PLLCFGR_PPLR_MASK         ((uint32_t)0x70000000)
#define PLLCFGR_PPLN_MASK         ((uint32_t)0x00007FC0)

#define SPI_CR2_FRF               ((uint16_t)0x0010)
#define SPI_SR_TIFRFE             ((uint16_t)0x0100)


void SPI_I2S_DeInit(SPI_TypeDef* SPIx)
{

  assert_param(IS_SPI_ALL_PERIPH(SPIx));

  if (SPIx == SPI1)
  {

    RCC_APB2PeriphResetCmd(RCC_APB2Periph_SPI1, ENABLE);

    RCC_APB2PeriphResetCmd(RCC_APB2Periph_SPI1, DISABLE);
  }
  else if (SPIx == SPI2)
  {

    RCC_APB1PeriphResetCmd(RCC_APB1Periph_SPI2, ENABLE);

    RCC_APB1PeriphResetCmd(RCC_APB1Periph_SPI2, DISABLE);
  }
  else if (SPIx == SPI3)
  {

    RCC_APB1PeriphResetCmd(RCC_APB1Periph_SPI3, ENABLE);

    RCC_APB1PeriphResetCmd(RCC_APB1Periph_SPI3, DISABLE);
  }
  else if (SPIx == SPI4)
  {

    RCC_APB2PeriphResetCmd(RCC_APB2Periph_SPI4, ENABLE);

    RCC_APB2PeriphResetCmd(RCC_APB2Periph_SPI4, DISABLE);
  }
  else if (SPIx == SPI5)
  {

    RCC_APB2PeriphResetCmd(RCC_APB2Periph_SPI5, ENABLE);

    RCC_APB2PeriphResetCmd(RCC_APB2Periph_SPI5, DISABLE);
  }
  else
  {
    if (SPIx == SPI6)
    {

      RCC_APB2PeriphResetCmd(RCC_APB2Periph_SPI6, ENABLE);

      RCC_APB2PeriphResetCmd(RCC_APB2Periph_SPI6, DISABLE);
    }
  }
}


void SPI_Init(SPI_TypeDef* SPIx, SPI_InitTypeDef* SPI_InitStruct)
{
  uint16_t tmpreg = 0;


  assert_param(IS_SPI_ALL_PERIPH(SPIx));


  assert_param(IS_SPI_DIRECTION_MODE(SPI_InitStruct->SPI_Direction));
  assert_param(IS_SPI_MODE(SPI_InitStruct->SPI_Mode));
  assert_param(IS_SPI_DATASIZE(SPI_InitStruct->SPI_DataSize));
  assert_param(IS_SPI_CPOL(SPI_InitStruct->SPI_CPOL));
  assert_param(IS_SPI_CPHA(SPI_InitStruct->SPI_CPHA));
  assert_param(IS_SPI_NSS(SPI_InitStruct->SPI_NSS));
  assert_param(IS_SPI_BAUDRATE_PRESCALER(SPI_InitStruct->SPI_BaudRatePrescaler));
  assert_param(IS_SPI_FIRST_BIT(SPI_InitStruct->SPI_FirstBit));
  assert_param(IS_SPI_CRC_POLYNOMIAL(SPI_InitStruct->SPI_CRCPolynomial));


  tmpreg = SPIx->CR1;

  tmpreg &= CR1_CLEAR_MASK;


  tmpreg |= (uint16_t)((uint32_t)SPI_InitStruct->SPI_Direction | SPI_InitStruct->SPI_Mode |
                  SPI_InitStruct->SPI_DataSize | SPI_InitStruct->SPI_CPOL |
                  SPI_InitStruct->SPI_CPHA | SPI_InitStruct->SPI_NSS |
                  SPI_InitStruct->SPI_BaudRatePrescaler | SPI_InitStruct->SPI_FirstBit);

  SPIx->CR1 = tmpreg;


  SPIx->I2SCFGR &= (uint16_t)~((uint16_t)SPI_I2SCFGR_I2SMOD);


  SPIx->CRCPR = SPI_InitStruct->SPI_CRCPolynomial;
}


void I2S_Init(SPI_TypeDef* SPIx, I2S_InitTypeDef* I2S_InitStruct)
{
  uint16_t tmpreg = 0, i2sdiv = 2, i2sodd = 0, packetlength = 1;
  uint32_t tmp = 0, i2sclk = 0;
#ifndef I2S_EXTERNAL_CLOCK_VAL
  uint32_t pllm = 0, plln = 0, pllr = 0;
#endif


  assert_param(IS_SPI_23_PERIPH(SPIx));
  assert_param(IS_I2S_MODE(I2S_InitStruct->I2S_Mode));
  assert_param(IS_I2S_STANDARD(I2S_InitStruct->I2S_Standard));
  assert_param(IS_I2S_DATA_FORMAT(I2S_InitStruct->I2S_DataFormat));
  assert_param(IS_I2S_MCLK_OUTPUT(I2S_InitStruct->I2S_MCLKOutput));
  assert_param(IS_I2S_AUDIO_FREQ(I2S_InitStruct->I2S_AudioFreq));
  assert_param(IS_I2S_CPOL(I2S_InitStruct->I2S_CPOL));


  SPIx->I2SCFGR &= I2SCFGR_CLEAR_MASK;
  SPIx->I2SPR = 0x0002;


  tmpreg = SPIx->I2SCFGR;


  if(I2S_InitStruct->I2S_AudioFreq == I2S_AudioFreq_Default)
  {
    i2sodd = (uint16_t)0;
    i2sdiv = (uint16_t)2;
  }

  else
  {

    if(I2S_InitStruct->I2S_DataFormat == I2S_DataFormat_16b)
    {

      packetlength = 1;
    }
    else
    {

      packetlength = 2;
    }


  #ifdef I2S_EXTERNAL_CLOCK_VAL

    if ((RCC->CFGR & RCC_CFGR_I2SSRC) == 0)
    {
      RCC->CFGR |= (uint32_t)RCC_CFGR_I2SSRC;
    }


    i2sclk = I2S_EXTERNAL_CLOCK_VAL;

  #else

    if ((RCC->CFGR & RCC_CFGR_I2SSRC) != 0)
    {
      RCC->CFGR &= ~(uint32_t)RCC_CFGR_I2SSRC;
    }


    plln = (uint32_t)(((RCC->PLLI2SCFGR & RCC_PLLI2SCFGR_PLLI2SN) >> 6) & \
                      (RCC_PLLI2SCFGR_PLLI2SN >> 6));


    pllr = (uint32_t)(((RCC->PLLI2SCFGR & RCC_PLLI2SCFGR_PLLI2SR) >> 28) & \
                      (RCC_PLLI2SCFGR_PLLI2SR >> 28));


    pllm = (uint32_t)(RCC->PLLCFGR & RCC_PLLCFGR_PLLM);

    if((RCC->PLLCFGR & RCC_PLLCFGR_PLLSRC) == RCC_PLLCFGR_PLLSRC_HSE)
    {

      i2sclk = (uint32_t)(((HSE_VALUE / pllm) * plln) / pllr);
    }
    else
    {
      i2sclk = (uint32_t)(((HSI_VALUE / pllm) * plln) / pllr);
    }
  #endif


    if(I2S_InitStruct->I2S_MCLKOutput == I2S_MCLKOutput_Enable)
    {

      tmp = (uint16_t)(((((i2sclk / 256) * 10) / I2S_InitStruct->I2S_AudioFreq)) + 5);
    }
    else
    {

      tmp = (uint16_t)(((((i2sclk / (32 * packetlength)) *10 ) / I2S_InitStruct->I2S_AudioFreq)) + 5);
    }


    tmp = tmp / 10;


    i2sodd = (uint16_t)(tmp & (uint16_t)0x0001);


    i2sdiv = (uint16_t)((tmp - i2sodd) / 2);


    i2sodd = (uint16_t) (i2sodd << 8);
  }


  if ((i2sdiv < 2) || (i2sdiv > 0xFF))
  {

    i2sdiv = 2;
    i2sodd = 0;
  }


  SPIx->I2SPR = (uint16_t)((uint16_t)i2sdiv | (uint16_t)(i2sodd | (uint16_t)I2S_InitStruct->I2S_MCLKOutput));


  tmpreg |= (uint16_t)((uint16_t)SPI_I2SCFGR_I2SMOD | (uint16_t)(I2S_InitStruct->I2S_Mode | \
                  (uint16_t)(I2S_InitStruct->I2S_Standard | (uint16_t)(I2S_InitStruct->I2S_DataFormat | \
                  (uint16_t)I2S_InitStruct->I2S_CPOL))));


  SPIx->I2SCFGR = tmpreg;
}


void SPI_StructInit(SPI_InitTypeDef* SPI_InitStruct)
{


  SPI_InitStruct->SPI_Direction = SPI_Direction_2Lines_FullDuplex;

  SPI_InitStruct->SPI_Mode = SPI_Mode_Slave;

  SPI_InitStruct->SPI_DataSize = SPI_DataSize_8b;

  SPI_InitStruct->SPI_CPOL = SPI_CPOL_Low;

  SPI_InitStruct->SPI_CPHA = SPI_CPHA_1Edge;

  SPI_InitStruct->SPI_NSS = SPI_NSS_Hard;

  SPI_InitStruct->SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;

  SPI_InitStruct->SPI_FirstBit = SPI_FirstBit_MSB;

  SPI_InitStruct->SPI_CRCPolynomial = 7;
}


void I2S_StructInit(I2S_InitTypeDef* I2S_InitStruct)
{


  I2S_InitStruct->I2S_Mode = I2S_Mode_SlaveTx;


  I2S_InitStruct->I2S_Standard = I2S_Standard_Phillips;


  I2S_InitStruct->I2S_DataFormat = I2S_DataFormat_16b;


  I2S_InitStruct->I2S_MCLKOutput = I2S_MCLKOutput_Disable;


  I2S_InitStruct->I2S_AudioFreq = I2S_AudioFreq_Default;


  I2S_InitStruct->I2S_CPOL = I2S_CPOL_Low;
}


void SPI_Cmd(SPI_TypeDef* SPIx, FunctionalState NewState)
{

  assert_param(IS_SPI_ALL_PERIPH(SPIx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {

    SPIx->CR1 |= SPI_CR1_SPE;
  }
  else
  {

    SPIx->CR1 &= (uint16_t)~((uint16_t)SPI_CR1_SPE);
  }
}


void I2S_Cmd(SPI_TypeDef* SPIx, FunctionalState NewState)
{

  assert_param(IS_SPI_23_PERIPH_EXT(SPIx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    SPIx->I2SCFGR |= SPI_I2SCFGR_I2SE;
  }
  else
  {

    SPIx->I2SCFGR &= (uint16_t)~((uint16_t)SPI_I2SCFGR_I2SE);
  }
}


void SPI_DataSizeConfig(SPI_TypeDef* SPIx, uint16_t SPI_DataSize)
{

  assert_param(IS_SPI_ALL_PERIPH(SPIx));
  assert_param(IS_SPI_DATASIZE(SPI_DataSize));

  SPIx->CR1 &= (uint16_t)~SPI_DataSize_16b;

  SPIx->CR1 |= SPI_DataSize;
}


void SPI_BiDirectionalLineConfig(SPI_TypeDef* SPIx, uint16_t SPI_Direction)
{

  assert_param(IS_SPI_ALL_PERIPH(SPIx));
  assert_param(IS_SPI_DIRECTION(SPI_Direction));
  if (SPI_Direction == SPI_Direction_Tx)
  {

    SPIx->CR1 |= SPI_Direction_Tx;
  }
  else
  {

    SPIx->CR1 &= SPI_Direction_Rx;
  }
}


void SPI_NSSInternalSoftwareConfig(SPI_TypeDef* SPIx, uint16_t SPI_NSSInternalSoft)
{

  assert_param(IS_SPI_ALL_PERIPH(SPIx));
  assert_param(IS_SPI_NSS_INTERNAL(SPI_NSSInternalSoft));
  if (SPI_NSSInternalSoft != SPI_NSSInternalSoft_Reset)
  {

    SPIx->CR1 |= SPI_NSSInternalSoft_Set;
  }
  else
  {

    SPIx->CR1 &= SPI_NSSInternalSoft_Reset;
  }
}


void SPI_SSOutputCmd(SPI_TypeDef* SPIx, FunctionalState NewState)
{

  assert_param(IS_SPI_ALL_PERIPH(SPIx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {

    SPIx->CR2 |= (uint16_t)SPI_CR2_SSOE;
  }
  else
  {

    SPIx->CR2 &= (uint16_t)~((uint16_t)SPI_CR2_SSOE);
  }
}


void SPI_TIModeCmd(SPI_TypeDef* SPIx, FunctionalState NewState)
{

  assert_param(IS_SPI_ALL_PERIPH(SPIx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    SPIx->CR2 |= SPI_CR2_FRF;
  }
  else
  {

    SPIx->CR2 &= (uint16_t)~SPI_CR2_FRF;
  }
}


void I2S_FullDuplexConfig(SPI_TypeDef* I2Sxext, I2S_InitTypeDef* I2S_InitStruct)
{
  uint16_t tmpreg = 0, tmp = 0;


  assert_param(IS_I2S_EXT_PERIPH(I2Sxext));
  assert_param(IS_I2S_MODE(I2S_InitStruct->I2S_Mode));
  assert_param(IS_I2S_STANDARD(I2S_InitStruct->I2S_Standard));
  assert_param(IS_I2S_DATA_FORMAT(I2S_InitStruct->I2S_DataFormat));
  assert_param(IS_I2S_CPOL(I2S_InitStruct->I2S_CPOL));


  I2Sxext->I2SCFGR &= I2SCFGR_CLEAR_MASK;
  I2Sxext->I2SPR = 0x0002;


  tmpreg = I2Sxext->I2SCFGR;


  if ((I2S_InitStruct->I2S_Mode == I2S_Mode_MasterTx) || (I2S_InitStruct->I2S_Mode == I2S_Mode_SlaveTx))
  {
    tmp = I2S_Mode_SlaveRx;
  }
  else
  {
    if ((I2S_InitStruct->I2S_Mode == I2S_Mode_MasterRx) || (I2S_InitStruct->I2S_Mode == I2S_Mode_SlaveRx))
    {
      tmp = I2S_Mode_SlaveTx;
    }
  }


  tmpreg |= (uint16_t)((uint16_t)SPI_I2SCFGR_I2SMOD | (uint16_t)(tmp | \
                  (uint16_t)(I2S_InitStruct->I2S_Standard | (uint16_t)(I2S_InitStruct->I2S_DataFormat | \
                  (uint16_t)I2S_InitStruct->I2S_CPOL))));


  I2Sxext->I2SCFGR = tmpreg;
}


uint16_t SPI_I2S_ReceiveData(SPI_TypeDef* SPIx)
{

  assert_param(IS_SPI_ALL_PERIPH_EXT(SPIx));


  return SPIx->DR;
}


void SPI_I2S_SendData(SPI_TypeDef* SPIx, uint16_t Data)
{

  assert_param(IS_SPI_ALL_PERIPH_EXT(SPIx));


  SPIx->DR = Data;
}


void SPI_CalculateCRC(SPI_TypeDef* SPIx, FunctionalState NewState)
{

  assert_param(IS_SPI_ALL_PERIPH(SPIx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {

    SPIx->CR1 |= SPI_CR1_CRCEN;
  }
  else
  {

    SPIx->CR1 &= (uint16_t)~((uint16_t)SPI_CR1_CRCEN);
  }
}


void SPI_TransmitCRC(SPI_TypeDef* SPIx)
{

  assert_param(IS_SPI_ALL_PERIPH(SPIx));


  SPIx->CR1 |= SPI_CR1_CRCNEXT;
}


uint16_t SPI_GetCRC(SPI_TypeDef* SPIx, uint8_t SPI_CRC)
{
  uint16_t crcreg = 0;

  assert_param(IS_SPI_ALL_PERIPH(SPIx));
  assert_param(IS_SPI_CRC(SPI_CRC));
  if (SPI_CRC != SPI_CRC_Rx)
  {

    crcreg = SPIx->TXCRCR;
  }
  else
  {

    crcreg = SPIx->RXCRCR;
  }

  return crcreg;
}


uint16_t SPI_GetCRCPolynomial(SPI_TypeDef* SPIx)
{

  assert_param(IS_SPI_ALL_PERIPH(SPIx));


  return SPIx->CRCPR;
}


void SPI_I2S_DMACmd(SPI_TypeDef* SPIx, uint16_t SPI_I2S_DMAReq, FunctionalState NewState)
{

  assert_param(IS_SPI_ALL_PERIPH_EXT(SPIx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  assert_param(IS_SPI_I2S_DMAREQ(SPI_I2S_DMAReq));

  if (NewState != DISABLE)
  {

    SPIx->CR2 |= SPI_I2S_DMAReq;
  }
  else
  {

    SPIx->CR2 &= (uint16_t)~SPI_I2S_DMAReq;
  }
}


void SPI_I2S_ITConfig(SPI_TypeDef* SPIx, uint8_t SPI_I2S_IT, FunctionalState NewState)
{
  uint16_t itpos = 0, itmask = 0 ;


  assert_param(IS_SPI_ALL_PERIPH_EXT(SPIx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  assert_param(IS_SPI_I2S_CONFIG_IT(SPI_I2S_IT));


  itpos = SPI_I2S_IT >> 4;


  itmask = (uint16_t)1 << (uint16_t)itpos;

  if (NewState != DISABLE)
  {

    SPIx->CR2 |= itmask;
  }
  else
  {

    SPIx->CR2 &= (uint16_t)~itmask;
  }
}


FlagStatus SPI_I2S_GetFlagStatus(SPI_TypeDef* SPIx, uint16_t SPI_I2S_FLAG)
{
  FlagStatus bitstatus = RESET;

  assert_param(IS_SPI_ALL_PERIPH_EXT(SPIx));
  assert_param(IS_SPI_I2S_GET_FLAG(SPI_I2S_FLAG));


  if ((SPIx->SR & SPI_I2S_FLAG) != (uint16_t)RESET)
  {

    bitstatus = SET;
  }
  else
  {

    bitstatus = RESET;
  }

  return  bitstatus;
}


void SPI_I2S_ClearFlag(SPI_TypeDef* SPIx, uint16_t SPI_I2S_FLAG)
{

  assert_param(IS_SPI_ALL_PERIPH_EXT(SPIx));
  assert_param(IS_SPI_I2S_CLEAR_FLAG(SPI_I2S_FLAG));


  SPIx->SR = (uint16_t)~SPI_I2S_FLAG;
}


ITStatus SPI_I2S_GetITStatus(SPI_TypeDef* SPIx, uint8_t SPI_I2S_IT)
{
  ITStatus bitstatus = RESET;
  uint16_t itpos = 0, itmask = 0, enablestatus = 0;


  assert_param(IS_SPI_ALL_PERIPH_EXT(SPIx));
  assert_param(IS_SPI_I2S_GET_IT(SPI_I2S_IT));


  itpos = 0x01 << (SPI_I2S_IT & 0x0F);


  itmask = SPI_I2S_IT >> 4;


  itmask = 0x01 << itmask;


  enablestatus = (SPIx->CR2 & itmask) ;


  if (((SPIx->SR & itpos) != (uint16_t)RESET) && enablestatus)
  {

    bitstatus = SET;
  }
  else
  {

    bitstatus = RESET;
  }

  return bitstatus;
}


void SPI_I2S_ClearITPendingBit(SPI_TypeDef* SPIx, uint8_t SPI_I2S_IT)
{
  uint16_t itpos = 0;

  assert_param(IS_SPI_ALL_PERIPH_EXT(SPIx));
  assert_param(IS_SPI_I2S_CLEAR_IT(SPI_I2S_IT));


  itpos = 0x01 << (SPI_I2S_IT & 0x0F);


  SPIx->SR = (uint16_t)~itpos;
}
