/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        stm32f4xx_fmc.c
 *
 * Description:  stm32f4xx_fmc.c module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#include "stm32f4xx_fmc.h"
#include "stm32f4xx_rcc.h"


const FMC_NORSRAMTimingInitTypeDef FMC_DefaultTimingStruct = {0x0F,
                                                              0x0F,
                                                              0xFF,
                                                              0x0F,
                                                              0x0F,
                                                              0x0F,
                                                              FMC_AccessMode_A
                                                              };


#define BCR_MBKEN_SET              ((uint32_t)0x00000001)
#define BCR_MBKEN_RESET            ((uint32_t)0x000FFFFE)
#define BCR_FACCEN_SET             ((uint32_t)0x00000040)


#define PCR_PBKEN_SET              ((uint32_t)0x00000004)
#define PCR_PBKEN_RESET            ((uint32_t)0x000FFFFB)
#define PCR_ECCEN_SET              ((uint32_t)0x00000040)
#define PCR_ECCEN_RESET            ((uint32_t)0x000FFFBF)
#define PCR_MEMORYTYPE_NAND        ((uint32_t)0x00000008)


#define SDCR_WriteProtection_RESET ((uint32_t)0x00007DFF)


#define SDCMR_CTB1_RESET           ((uint32_t)0x003FFFEF)
#define SDCMR_CTB2_RESET           ((uint32_t)0x003FFFF7)
#define SDCMR_CTB1_2_RESET         ((uint32_t)0x003FFFE7)


void FMC_NORSRAMDeInit(uint32_t FMC_Bank)
{

  assert_param(IS_FMC_NORSRAM_BANK(FMC_Bank));


  if(FMC_Bank == FMC_Bank1_NORSRAM1)
  {
    FMC_Bank1->BTCR[FMC_Bank] = 0x000030DB;
  }

  else
  {
    FMC_Bank1->BTCR[FMC_Bank] = 0x000030D2;
  }
  FMC_Bank1->BTCR[FMC_Bank + 1] = 0x0FFFFFFF;
  FMC_Bank1E->BWTR[FMC_Bank] = 0x0FFFFFFF;
}


void FMC_NORSRAMInit(FMC_NORSRAMInitTypeDef* FMC_NORSRAMInitStruct)
{
  uint32_t tmpr = 0;


  assert_param(IS_FMC_NORSRAM_BANK(FMC_NORSRAMInitStruct->FMC_Bank));
  assert_param(IS_FMC_MUX(FMC_NORSRAMInitStruct->FMC_DataAddressMux));
  assert_param(IS_FMC_MEMORY(FMC_NORSRAMInitStruct->FMC_MemoryType));
  assert_param(IS_FMC_NORSRAM_MEMORY_WIDTH(FMC_NORSRAMInitStruct->FMC_MemoryDataWidth));
  assert_param(IS_FMC_BURSTMODE(FMC_NORSRAMInitStruct->FMC_BurstAccessMode));
  assert_param(IS_FMC_WAIT_POLARITY(FMC_NORSRAMInitStruct->FMC_WaitSignalPolarity));
  assert_param(IS_FMC_WRAP_MODE(FMC_NORSRAMInitStruct->FMC_WrapMode));
  assert_param(IS_FMC_WAIT_SIGNAL_ACTIVE(FMC_NORSRAMInitStruct->FMC_WaitSignalActive));
  assert_param(IS_FMC_WRITE_OPERATION(FMC_NORSRAMInitStruct->FMC_WriteOperation));
  assert_param(IS_FMC_WAITE_SIGNAL(FMC_NORSRAMInitStruct->FMC_WaitSignal));
  assert_param(IS_FMC_EXTENDED_MODE(FMC_NORSRAMInitStruct->FMC_ExtendedMode));
  assert_param(IS_FMC_ASYNWAIT(FMC_NORSRAMInitStruct->FMC_AsynchronousWait));
  assert_param(IS_FMC_WRITE_BURST(FMC_NORSRAMInitStruct->FMC_WriteBurst));
  assert_param(IS_FMC_CONTINOUS_CLOCK(FMC_NORSRAMInitStruct->FMC_ContinousClock));
  assert_param(IS_FMC_ADDRESS_SETUP_TIME(FMC_NORSRAMInitStruct->FMC_ReadWriteTimingStruct->FMC_AddressSetupTime));
  assert_param(IS_FMC_ADDRESS_HOLD_TIME(FMC_NORSRAMInitStruct->FMC_ReadWriteTimingStruct->FMC_AddressHoldTime));
  assert_param(IS_FMC_DATASETUP_TIME(FMC_NORSRAMInitStruct->FMC_ReadWriteTimingStruct->FMC_DataSetupTime));
  assert_param(IS_FMC_TURNAROUND_TIME(FMC_NORSRAMInitStruct->FMC_ReadWriteTimingStruct->FMC_BusTurnAroundDuration));
  assert_param(IS_FMC_CLK_DIV(FMC_NORSRAMInitStruct->FMC_ReadWriteTimingStruct->FMC_CLKDivision));
  assert_param(IS_FMC_DATA_LATENCY(FMC_NORSRAMInitStruct->FMC_ReadWriteTimingStruct->FMC_DataLatency));
  assert_param(IS_FMC_ACCESS_MODE(FMC_NORSRAMInitStruct->FMC_ReadWriteTimingStruct->FMC_AccessMode));


  FMC_Bank1->BTCR[FMC_NORSRAMInitStruct->FMC_Bank] =
            (uint32_t)FMC_NORSRAMInitStruct->FMC_DataAddressMux |
            FMC_NORSRAMInitStruct->FMC_MemoryType |
            FMC_NORSRAMInitStruct->FMC_MemoryDataWidth |
            FMC_NORSRAMInitStruct->FMC_BurstAccessMode |
            FMC_NORSRAMInitStruct->FMC_WaitSignalPolarity |
            FMC_NORSRAMInitStruct->FMC_WrapMode |
            FMC_NORSRAMInitStruct->FMC_WaitSignalActive |
            FMC_NORSRAMInitStruct->FMC_WriteOperation |
            FMC_NORSRAMInitStruct->FMC_WaitSignal |
            FMC_NORSRAMInitStruct->FMC_ExtendedMode |
            FMC_NORSRAMInitStruct->FMC_AsynchronousWait |
            FMC_NORSRAMInitStruct->FMC_WriteBurst |
            FMC_NORSRAMInitStruct->FMC_ContinousClock;


  if(FMC_NORSRAMInitStruct->FMC_MemoryType == FMC_MemoryType_NOR)
  {
    FMC_Bank1->BTCR[FMC_NORSRAMInitStruct->FMC_Bank] |= (uint32_t)BCR_FACCEN_SET;
  }


  if((FMC_NORSRAMInitStruct->FMC_ContinousClock == FMC_CClock_SyncAsync) && (FMC_NORSRAMInitStruct->FMC_Bank != FMC_Bank1_NORSRAM1))
  {
    tmpr = (uint32_t)((FMC_Bank1->BTCR[FMC_Bank1_NORSRAM1+1]) & ~(((uint32_t)0x0F) << 20));

    FMC_Bank1->BTCR[FMC_Bank1_NORSRAM1]  |= FMC_NORSRAMInitStruct->FMC_ContinousClock;
    FMC_Bank1->BTCR[FMC_Bank1_NORSRAM1]  |= FMC_BurstAccessMode_Enable;
    FMC_Bank1->BTCR[FMC_Bank1_NORSRAM1+1] = (uint32_t)(tmpr | (((FMC_NORSRAMInitStruct->FMC_ReadWriteTimingStruct->FMC_CLKDivision)-1) << 20));
  }


  FMC_Bank1->BTCR[FMC_NORSRAMInitStruct->FMC_Bank+1] =
            (uint32_t)FMC_NORSRAMInitStruct->FMC_ReadWriteTimingStruct->FMC_AddressSetupTime |
            (FMC_NORSRAMInitStruct->FMC_ReadWriteTimingStruct->FMC_AddressHoldTime << 4) |
            (FMC_NORSRAMInitStruct->FMC_ReadWriteTimingStruct->FMC_DataSetupTime << 8) |
            (FMC_NORSRAMInitStruct->FMC_ReadWriteTimingStruct->FMC_BusTurnAroundDuration << 16) |
            ((FMC_NORSRAMInitStruct->FMC_ReadWriteTimingStruct->FMC_CLKDivision) << 20) |
            ((FMC_NORSRAMInitStruct->FMC_ReadWriteTimingStruct->FMC_DataLatency) << 24) |
             FMC_NORSRAMInitStruct->FMC_ReadWriteTimingStruct->FMC_AccessMode;


  if(FMC_NORSRAMInitStruct->FMC_ExtendedMode == FMC_ExtendedMode_Enable)
  {
    assert_param(IS_FMC_ADDRESS_SETUP_TIME(FMC_NORSRAMInitStruct->FMC_WriteTimingStruct->FMC_AddressSetupTime));
    assert_param(IS_FMC_ADDRESS_HOLD_TIME(FMC_NORSRAMInitStruct->FMC_WriteTimingStruct->FMC_AddressHoldTime));
    assert_param(IS_FMC_DATASETUP_TIME(FMC_NORSRAMInitStruct->FMC_WriteTimingStruct->FMC_DataSetupTime));
    assert_param(IS_FMC_CLK_DIV(FMC_NORSRAMInitStruct->FMC_WriteTimingStruct->FMC_CLKDivision));
    assert_param(IS_FMC_DATA_LATENCY(FMC_NORSRAMInitStruct->FMC_WriteTimingStruct->FMC_DataLatency));
    assert_param(IS_FMC_ACCESS_MODE(FMC_NORSRAMInitStruct->FMC_WriteTimingStruct->FMC_AccessMode));

    FMC_Bank1E->BWTR[FMC_NORSRAMInitStruct->FMC_Bank] =
               (uint32_t)FMC_NORSRAMInitStruct->FMC_WriteTimingStruct->FMC_AddressSetupTime |
               (FMC_NORSRAMInitStruct->FMC_WriteTimingStruct->FMC_AddressHoldTime << 4 )|
               (FMC_NORSRAMInitStruct->FMC_WriteTimingStruct->FMC_DataSetupTime << 8) |
               ((FMC_NORSRAMInitStruct->FMC_WriteTimingStruct->FMC_CLKDivision) << 20) |
               ((FMC_NORSRAMInitStruct->FMC_WriteTimingStruct->FMC_DataLatency) << 24) |
               FMC_NORSRAMInitStruct->FMC_WriteTimingStruct->FMC_AccessMode;
  }
  else
  {
    FMC_Bank1E->BWTR[FMC_NORSRAMInitStruct->FMC_Bank] = 0x0FFFFFFF;
  }

}


void FMC_NORSRAMStructInit(FMC_NORSRAMInitTypeDef* FMC_NORSRAMInitStruct)
{

  FMC_NORSRAMInitStruct->FMC_Bank = FMC_Bank1_NORSRAM1;
  FMC_NORSRAMInitStruct->FMC_DataAddressMux = FMC_DataAddressMux_Enable;
  FMC_NORSRAMInitStruct->FMC_MemoryType = FMC_MemoryType_SRAM;
  FMC_NORSRAMInitStruct->FMC_MemoryDataWidth = FMC_NORSRAM_MemoryDataWidth_16b;
  FMC_NORSRAMInitStruct->FMC_BurstAccessMode = FMC_BurstAccessMode_Disable;
  FMC_NORSRAMInitStruct->FMC_AsynchronousWait = FMC_AsynchronousWait_Disable;
  FMC_NORSRAMInitStruct->FMC_WaitSignalPolarity = FMC_WaitSignalPolarity_Low;
  FMC_NORSRAMInitStruct->FMC_WrapMode = FMC_WrapMode_Disable;
  FMC_NORSRAMInitStruct->FMC_WaitSignalActive = FMC_WaitSignalActive_BeforeWaitState;
  FMC_NORSRAMInitStruct->FMC_WriteOperation = FMC_WriteOperation_Enable;
  FMC_NORSRAMInitStruct->FMC_WaitSignal = FMC_WaitSignal_Enable;
  FMC_NORSRAMInitStruct->FMC_ExtendedMode = FMC_ExtendedMode_Disable;
  FMC_NORSRAMInitStruct->FMC_WriteBurst = FMC_WriteBurst_Disable;
  FMC_NORSRAMInitStruct->FMC_ContinousClock = FMC_CClock_SyncOnly;

  FMC_NORSRAMInitStruct->FMC_ReadWriteTimingStruct = (FMC_NORSRAMTimingInitTypeDef*)&FMC_DefaultTimingStruct;
  FMC_NORSRAMInitStruct->FMC_WriteTimingStruct = (FMC_NORSRAMTimingInitTypeDef*)&FMC_DefaultTimingStruct;
}


void FMC_NORSRAMCmd(uint32_t FMC_Bank, FunctionalState NewState)
{
  assert_param(IS_FMC_NORSRAM_BANK(FMC_Bank));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    FMC_Bank1->BTCR[FMC_Bank] |= BCR_MBKEN_SET;
  }
  else
  {

    FMC_Bank1->BTCR[FMC_Bank] &= BCR_MBKEN_RESET;
  }
}


void FMC_NANDDeInit(uint32_t FMC_Bank)
{

  assert_param(IS_FMC_NAND_BANK(FMC_Bank));

  if(FMC_Bank == FMC_Bank2_NAND)
  {

    FMC_Bank2->PCR2 = 0x00000018;
    FMC_Bank2->SR2 = 0x00000040;
    FMC_Bank2->PMEM2 = 0xFCFCFCFC;
    FMC_Bank2->PATT2 = 0xFCFCFCFC;
  }

  else
  {

    FMC_Bank3->PCR3 = 0x00000018;
    FMC_Bank3->SR3 = 0x00000040;
    FMC_Bank3->PMEM3 = 0xFCFCFCFC;
    FMC_Bank3->PATT3 = 0xFCFCFCFC;
  }
}


void FMC_NANDInit(FMC_NANDInitTypeDef* FMC_NANDInitStruct)
{
  uint32_t tmppcr = 0x00000000, tmppmem = 0x00000000, tmppatt = 0x00000000;


  assert_param(IS_FMC_NAND_BANK(FMC_NANDInitStruct->FMC_Bank));
  assert_param(IS_FMC_WAIT_FEATURE(FMC_NANDInitStruct->FMC_Waitfeature));
  assert_param(IS_FMC_NAND_MEMORY_WIDTH(FMC_NANDInitStruct->FMC_MemoryDataWidth));
  assert_param(IS_FMC_ECC_STATE(FMC_NANDInitStruct->FMC_ECC));
  assert_param(IS_FMC_ECCPAGE_SIZE(FMC_NANDInitStruct->FMC_ECCPageSize));
  assert_param(IS_FMC_TCLR_TIME(FMC_NANDInitStruct->FMC_TCLRSetupTime));
  assert_param(IS_FMC_TAR_TIME(FMC_NANDInitStruct->FMC_TARSetupTime));
  assert_param(IS_FMC_SETUP_TIME(FMC_NANDInitStruct->FMC_CommonSpaceTimingStruct->FMC_SetupTime));
  assert_param(IS_FMC_WAIT_TIME(FMC_NANDInitStruct->FMC_CommonSpaceTimingStruct->FMC_WaitSetupTime));
  assert_param(IS_FMC_HOLD_TIME(FMC_NANDInitStruct->FMC_CommonSpaceTimingStruct->FMC_HoldSetupTime));
  assert_param(IS_FMC_HIZ_TIME(FMC_NANDInitStruct->FMC_CommonSpaceTimingStruct->FMC_HiZSetupTime));
  assert_param(IS_FMC_SETUP_TIME(FMC_NANDInitStruct->FMC_AttributeSpaceTimingStruct->FMC_SetupTime));
  assert_param(IS_FMC_WAIT_TIME(FMC_NANDInitStruct->FMC_AttributeSpaceTimingStruct->FMC_WaitSetupTime));
  assert_param(IS_FMC_HOLD_TIME(FMC_NANDInitStruct->FMC_AttributeSpaceTimingStruct->FMC_HoldSetupTime));
  assert_param(IS_FMC_HIZ_TIME(FMC_NANDInitStruct->FMC_AttributeSpaceTimingStruct->FMC_HiZSetupTime));


  tmppcr = (uint32_t)FMC_NANDInitStruct->FMC_Waitfeature |
            PCR_MEMORYTYPE_NAND |
            FMC_NANDInitStruct->FMC_MemoryDataWidth |
            FMC_NANDInitStruct->FMC_ECC |
            FMC_NANDInitStruct->FMC_ECCPageSize |
            (FMC_NANDInitStruct->FMC_TCLRSetupTime << 9 )|
            (FMC_NANDInitStruct->FMC_TARSetupTime << 13);


  tmppmem = (uint32_t)FMC_NANDInitStruct->FMC_CommonSpaceTimingStruct->FMC_SetupTime |
            (FMC_NANDInitStruct->FMC_CommonSpaceTimingStruct->FMC_WaitSetupTime << 8) |
            (FMC_NANDInitStruct->FMC_CommonSpaceTimingStruct->FMC_HoldSetupTime << 16)|
            (FMC_NANDInitStruct->FMC_CommonSpaceTimingStruct->FMC_HiZSetupTime << 24);


  tmppatt = (uint32_t)FMC_NANDInitStruct->FMC_AttributeSpaceTimingStruct->FMC_SetupTime |
            (FMC_NANDInitStruct->FMC_AttributeSpaceTimingStruct->FMC_WaitSetupTime << 8) |
            (FMC_NANDInitStruct->FMC_AttributeSpaceTimingStruct->FMC_HoldSetupTime << 16)|
            (FMC_NANDInitStruct->FMC_AttributeSpaceTimingStruct->FMC_HiZSetupTime << 24);

  if(FMC_NANDInitStruct->FMC_Bank == FMC_Bank2_NAND)
  {

    FMC_Bank2->PCR2 = tmppcr;
    FMC_Bank2->PMEM2 = tmppmem;
    FMC_Bank2->PATT2 = tmppatt;
  }
  else
  {

    FMC_Bank3->PCR3 = tmppcr;
    FMC_Bank3->PMEM3 = tmppmem;
    FMC_Bank3->PATT3 = tmppatt;
  }
}


void FMC_NANDStructInit(FMC_NANDInitTypeDef* FMC_NANDInitStruct)
{

  FMC_NANDInitStruct->FMC_Bank = FMC_Bank2_NAND;
  FMC_NANDInitStruct->FMC_Waitfeature = FMC_Waitfeature_Disable;
  FMC_NANDInitStruct->FMC_MemoryDataWidth = FMC_NAND_MemoryDataWidth_16b;
  FMC_NANDInitStruct->FMC_ECC = FMC_ECC_Disable;
  FMC_NANDInitStruct->FMC_ECCPageSize = FMC_ECCPageSize_256Bytes;
  FMC_NANDInitStruct->FMC_TCLRSetupTime = 0x0;
  FMC_NANDInitStruct->FMC_TARSetupTime = 0x0;
  FMC_NANDInitStruct->FMC_CommonSpaceTimingStruct->FMC_SetupTime = 252;
  FMC_NANDInitStruct->FMC_CommonSpaceTimingStruct->FMC_WaitSetupTime = 252;
  FMC_NANDInitStruct->FMC_CommonSpaceTimingStruct->FMC_HoldSetupTime = 252;
  FMC_NANDInitStruct->FMC_CommonSpaceTimingStruct->FMC_HiZSetupTime = 252;
  FMC_NANDInitStruct->FMC_AttributeSpaceTimingStruct->FMC_SetupTime = 252;
  FMC_NANDInitStruct->FMC_AttributeSpaceTimingStruct->FMC_WaitSetupTime = 252;
  FMC_NANDInitStruct->FMC_AttributeSpaceTimingStruct->FMC_HoldSetupTime = 252;
  FMC_NANDInitStruct->FMC_AttributeSpaceTimingStruct->FMC_HiZSetupTime = 252;
}


void FMC_NANDCmd(uint32_t FMC_Bank, FunctionalState NewState)
{
  assert_param(IS_FMC_NAND_BANK(FMC_Bank));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    if(FMC_Bank == FMC_Bank2_NAND)
    {
      FMC_Bank2->PCR2 |= PCR_PBKEN_SET;
    }
    else
    {
      FMC_Bank3->PCR3 |= PCR_PBKEN_SET;
    }
  }
  else
  {

    if(FMC_Bank == FMC_Bank2_NAND)
    {
      FMC_Bank2->PCR2 &= PCR_PBKEN_RESET;
    }
    else
    {
      FMC_Bank3->PCR3 &= PCR_PBKEN_RESET;
    }
  }
}

void FMC_NANDECCCmd(uint32_t FMC_Bank, FunctionalState NewState)
{
  assert_param(IS_FMC_NAND_BANK(FMC_Bank));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    if(FMC_Bank == FMC_Bank2_NAND)
    {
      FMC_Bank2->PCR2 |= PCR_ECCEN_SET;
    }
    else
    {
      FMC_Bank3->PCR3 |= PCR_ECCEN_SET;
    }
  }
  else
  {

    if(FMC_Bank == FMC_Bank2_NAND)
    {
      FMC_Bank2->PCR2 &= PCR_ECCEN_RESET;
    }
    else
    {
      FMC_Bank3->PCR3 &= PCR_ECCEN_RESET;
    }
  }
}


uint32_t FMC_GetECC(uint32_t FMC_Bank)
{
  uint32_t eccval = 0x00000000;

  if(FMC_Bank == FMC_Bank2_NAND)
  {

    eccval = FMC_Bank2->ECCR2;
  }
  else
  {

    eccval = FMC_Bank3->ECCR3;
  }

  return(eccval);
}


void FMC_PCCARDDeInit(void)
{

  FMC_Bank4->PCR4 = 0x00000018;
  FMC_Bank4->SR4 = 0x00000000;
  FMC_Bank4->PMEM4 = 0xFCFCFCFC;
  FMC_Bank4->PATT4 = 0xFCFCFCFC;
  FMC_Bank4->PIO4 = 0xFCFCFCFC;
}


void FMC_PCCARDInit(FMC_PCCARDInitTypeDef* FMC_PCCARDInitStruct)
{

  assert_param(IS_FMC_WAIT_FEATURE(FMC_PCCARDInitStruct->FMC_Waitfeature));
  assert_param(IS_FMC_TCLR_TIME(FMC_PCCARDInitStruct->FMC_TCLRSetupTime));
  assert_param(IS_FMC_TAR_TIME(FMC_PCCARDInitStruct->FMC_TARSetupTime));

  assert_param(IS_FMC_SETUP_TIME(FMC_PCCARDInitStruct->FMC_CommonSpaceTimingStruct->FMC_SetupTime));
  assert_param(IS_FMC_WAIT_TIME(FMC_PCCARDInitStruct->FMC_CommonSpaceTimingStruct->FMC_WaitSetupTime));
  assert_param(IS_FMC_HOLD_TIME(FMC_PCCARDInitStruct->FMC_CommonSpaceTimingStruct->FMC_HoldSetupTime));
  assert_param(IS_FMC_HIZ_TIME(FMC_PCCARDInitStruct->FMC_CommonSpaceTimingStruct->FMC_HiZSetupTime));

  assert_param(IS_FMC_SETUP_TIME(FMC_PCCARDInitStruct->FMC_AttributeSpaceTimingStruct->FMC_SetupTime));
  assert_param(IS_FMC_WAIT_TIME(FMC_PCCARDInitStruct->FMC_AttributeSpaceTimingStruct->FMC_WaitSetupTime));
  assert_param(IS_FMC_HOLD_TIME(FMC_PCCARDInitStruct->FMC_AttributeSpaceTimingStruct->FMC_HoldSetupTime));
  assert_param(IS_FMC_HIZ_TIME(FMC_PCCARDInitStruct->FMC_AttributeSpaceTimingStruct->FMC_HiZSetupTime));
  assert_param(IS_FMC_SETUP_TIME(FMC_PCCARDInitStruct->FMC_IOSpaceTimingStruct->FMC_SetupTime));
  assert_param(IS_FMC_WAIT_TIME(FMC_PCCARDInitStruct->FMC_IOSpaceTimingStruct->FMC_WaitSetupTime));
  assert_param(IS_FMC_HOLD_TIME(FMC_PCCARDInitStruct->FMC_IOSpaceTimingStruct->FMC_HoldSetupTime));
  assert_param(IS_FMC_HIZ_TIME(FMC_PCCARDInitStruct->FMC_IOSpaceTimingStruct->FMC_HiZSetupTime));


  FMC_Bank4->PCR4 = (uint32_t)FMC_PCCARDInitStruct->FMC_Waitfeature |
                     FMC_NAND_MemoryDataWidth_16b |
                     (FMC_PCCARDInitStruct->FMC_TCLRSetupTime << 9) |
                     (FMC_PCCARDInitStruct->FMC_TARSetupTime << 13);


  FMC_Bank4->PMEM4 = (uint32_t)FMC_PCCARDInitStruct->FMC_CommonSpaceTimingStruct->FMC_SetupTime |
                      (FMC_PCCARDInitStruct->FMC_CommonSpaceTimingStruct->FMC_WaitSetupTime << 8) |
                      (FMC_PCCARDInitStruct->FMC_CommonSpaceTimingStruct->FMC_HoldSetupTime << 16)|
                      (FMC_PCCARDInitStruct->FMC_CommonSpaceTimingStruct->FMC_HiZSetupTime << 24);


  FMC_Bank4->PATT4 = (uint32_t)FMC_PCCARDInitStruct->FMC_AttributeSpaceTimingStruct->FMC_SetupTime |
                      (FMC_PCCARDInitStruct->FMC_AttributeSpaceTimingStruct->FMC_WaitSetupTime << 8) |
                      (FMC_PCCARDInitStruct->FMC_AttributeSpaceTimingStruct->FMC_HoldSetupTime << 16)|
                      (FMC_PCCARDInitStruct->FMC_AttributeSpaceTimingStruct->FMC_HiZSetupTime << 24);


  FMC_Bank4->PIO4 = (uint32_t)FMC_PCCARDInitStruct->FMC_IOSpaceTimingStruct->FMC_SetupTime |
                     (FMC_PCCARDInitStruct->FMC_IOSpaceTimingStruct->FMC_WaitSetupTime << 8) |
                     (FMC_PCCARDInitStruct->FMC_IOSpaceTimingStruct->FMC_HoldSetupTime << 16)|
                     (FMC_PCCARDInitStruct->FMC_IOSpaceTimingStruct->FMC_HiZSetupTime << 24);
}


void FMC_PCCARDStructInit(FMC_PCCARDInitTypeDef* FMC_PCCARDInitStruct)
{

  FMC_PCCARDInitStruct->FMC_Waitfeature = FMC_Waitfeature_Disable;
  FMC_PCCARDInitStruct->FMC_TCLRSetupTime = 0;
  FMC_PCCARDInitStruct->FMC_TARSetupTime = 0;
  FMC_PCCARDInitStruct->FMC_CommonSpaceTimingStruct->FMC_SetupTime = 252;
  FMC_PCCARDInitStruct->FMC_CommonSpaceTimingStruct->FMC_WaitSetupTime = 252;
  FMC_PCCARDInitStruct->FMC_CommonSpaceTimingStruct->FMC_HoldSetupTime = 252;
  FMC_PCCARDInitStruct->FMC_CommonSpaceTimingStruct->FMC_HiZSetupTime = 252;
  FMC_PCCARDInitStruct->FMC_AttributeSpaceTimingStruct->FMC_SetupTime = 252;
  FMC_PCCARDInitStruct->FMC_AttributeSpaceTimingStruct->FMC_WaitSetupTime = 252;
  FMC_PCCARDInitStruct->FMC_AttributeSpaceTimingStruct->FMC_HoldSetupTime = 252;
  FMC_PCCARDInitStruct->FMC_AttributeSpaceTimingStruct->FMC_HiZSetupTime = 252;
  FMC_PCCARDInitStruct->FMC_IOSpaceTimingStruct->FMC_SetupTime = 252;
  FMC_PCCARDInitStruct->FMC_IOSpaceTimingStruct->FMC_WaitSetupTime = 252;
  FMC_PCCARDInitStruct->FMC_IOSpaceTimingStruct->FMC_HoldSetupTime = 252;
  FMC_PCCARDInitStruct->FMC_IOSpaceTimingStruct->FMC_HiZSetupTime = 252;
}


void FMC_PCCARDCmd(FunctionalState NewState)
{
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    FMC_Bank4->PCR4 |= PCR_PBKEN_SET;
  }
  else
  {

    FMC_Bank4->PCR4 &= PCR_PBKEN_RESET;
  }
}


void FMC_SDRAMDeInit(uint32_t FMC_Bank)
{

  assert_param(IS_FMC_SDRAM_BANK(FMC_Bank));

  FMC_Bank5_6->SDCR[FMC_Bank] = 0x000002D0;
  FMC_Bank5_6->SDTR[FMC_Bank] = 0x0FFFFFFF;
  FMC_Bank5_6->SDCMR = 0x00000000;
  FMC_Bank5_6->SDRTR = 0x00000000;
  FMC_Bank5_6->SDSR = 0x00000000;
}


void FMC_SDRAMInit(FMC_SDRAMInitTypeDef* FMC_SDRAMInitStruct)
{

  uint32_t tmpr1 = 0;
  uint32_t tmpr2 = 0;
  uint32_t tmpr3 = 0;
  uint32_t tmpr4 = 0;


  assert_param(IS_FMC_SDRAM_BANK(FMC_SDRAMInitStruct->FMC_Bank));
  assert_param(IS_FMC_COLUMNBITS_NUMBER(FMC_SDRAMInitStruct->FMC_ColumnBitsNumber));
  assert_param(IS_FMC_ROWBITS_NUMBER(FMC_SDRAMInitStruct->FMC_RowBitsNumber));
  assert_param(IS_FMC_SDMEMORY_WIDTH(FMC_SDRAMInitStruct->FMC_SDMemoryDataWidth));
  assert_param(IS_FMC_INTERNALBANK_NUMBER(FMC_SDRAMInitStruct->FMC_InternalBankNumber));
  assert_param(IS_FMC_CAS_LATENCY(FMC_SDRAMInitStruct->FMC_CASLatency));
  assert_param(IS_FMC_WRITE_PROTECTION(FMC_SDRAMInitStruct->FMC_WriteProtection));
  assert_param(IS_FMC_SDCLOCK_PERIOD(FMC_SDRAMInitStruct->FMC_SDClockPeriod));
  assert_param(IS_FMC_READ_BURST(FMC_SDRAMInitStruct->FMC_ReadBurst));
  assert_param(IS_FMC_READPIPE_DELAY(FMC_SDRAMInitStruct->FMC_ReadPipeDelay));


  assert_param(IS_FMC_LOADTOACTIVE_DELAY(FMC_SDRAMInitStruct->FMC_SDRAMTimingStruct->FMC_LoadToActiveDelay));
  assert_param(IS_FMC_EXITSELFREFRESH_DELAY(FMC_SDRAMInitStruct->FMC_SDRAMTimingStruct->FMC_ExitSelfRefreshDelay));
  assert_param(IS_FMC_SELFREFRESH_TIME(FMC_SDRAMInitStruct->FMC_SDRAMTimingStruct->FMC_SelfRefreshTime));
  assert_param(IS_FMC_ROWCYCLE_DELAY(FMC_SDRAMInitStruct->FMC_SDRAMTimingStruct->FMC_RowCycleDelay));
  assert_param(IS_FMC_WRITE_RECOVERY_TIME(FMC_SDRAMInitStruct->FMC_SDRAMTimingStruct->FMC_WriteRecoveryTime));
  assert_param(IS_FMC_RP_DELAY(FMC_SDRAMInitStruct->FMC_SDRAMTimingStruct->FMC_RPDelay));
  assert_param(IS_FMC_RCD_DELAY(FMC_SDRAMInitStruct->FMC_SDRAMTimingStruct->FMC_RCDDelay));


  tmpr1 =   (uint32_t)FMC_SDRAMInitStruct->FMC_ColumnBitsNumber |
             FMC_SDRAMInitStruct->FMC_RowBitsNumber |
             FMC_SDRAMInitStruct->FMC_SDMemoryDataWidth |
             FMC_SDRAMInitStruct->FMC_InternalBankNumber |
             FMC_SDRAMInitStruct->FMC_CASLatency |
             FMC_SDRAMInitStruct->FMC_WriteProtection |
             FMC_SDRAMInitStruct->FMC_SDClockPeriod |
             FMC_SDRAMInitStruct->FMC_ReadBurst |
             FMC_SDRAMInitStruct->FMC_ReadPipeDelay;

  if(FMC_SDRAMInitStruct->FMC_Bank == FMC_Bank1_SDRAM )
  {
    FMC_Bank5_6->SDCR[FMC_SDRAMInitStruct->FMC_Bank] = tmpr1;
  }
  else
  {
    tmpr3 = (uint32_t)FMC_SDRAMInitStruct->FMC_SDClockPeriod |
             FMC_SDRAMInitStruct->FMC_ReadBurst |
             FMC_SDRAMInitStruct->FMC_ReadPipeDelay;

    FMC_Bank5_6->SDCR[FMC_Bank1_SDRAM] = tmpr3;
    FMC_Bank5_6->SDCR[FMC_SDRAMInitStruct->FMC_Bank] = tmpr1;
  }

  if(FMC_SDRAMInitStruct->FMC_Bank == FMC_Bank1_SDRAM )
  {
    tmpr2 =   (uint32_t)((FMC_SDRAMInitStruct->FMC_SDRAMTimingStruct->FMC_LoadToActiveDelay)-1) |
            (((FMC_SDRAMInitStruct->FMC_SDRAMTimingStruct->FMC_ExitSelfRefreshDelay)-1) << 4) |
            (((FMC_SDRAMInitStruct->FMC_SDRAMTimingStruct->FMC_SelfRefreshTime)-1) << 8) |
            (((FMC_SDRAMInitStruct->FMC_SDRAMTimingStruct->FMC_RowCycleDelay)-1) << 12) |
            (((FMC_SDRAMInitStruct->FMC_SDRAMTimingStruct->FMC_WriteRecoveryTime)-1) << 16) |
            (((FMC_SDRAMInitStruct->FMC_SDRAMTimingStruct->FMC_RPDelay)-1) << 20) |
            (((FMC_SDRAMInitStruct->FMC_SDRAMTimingStruct->FMC_RCDDelay)-1) << 24);

            FMC_Bank5_6->SDTR[FMC_SDRAMInitStruct->FMC_Bank] = tmpr2;
  }
  else
  {
    tmpr2 =   (uint32_t)((FMC_SDRAMInitStruct->FMC_SDRAMTimingStruct->FMC_LoadToActiveDelay)-1) |
            (((FMC_SDRAMInitStruct->FMC_SDRAMTimingStruct->FMC_ExitSelfRefreshDelay)-1) << 4) |
            (((FMC_SDRAMInitStruct->FMC_SDRAMTimingStruct->FMC_SelfRefreshTime)-1) << 8) |
            (((FMC_SDRAMInitStruct->FMC_SDRAMTimingStruct->FMC_WriteRecoveryTime)-1) << 16);

    tmpr4 =   (uint32_t)(((FMC_SDRAMInitStruct->FMC_SDRAMTimingStruct->FMC_RowCycleDelay)-1) << 12) |
            (((FMC_SDRAMInitStruct->FMC_SDRAMTimingStruct->FMC_RPDelay)-1) << 20);

            FMC_Bank5_6->SDTR[FMC_Bank1_SDRAM] = tmpr4;
            FMC_Bank5_6->SDTR[FMC_SDRAMInitStruct->FMC_Bank] = tmpr2;
  }

}


void FMC_SDRAMStructInit(FMC_SDRAMInitTypeDef* FMC_SDRAMInitStruct)
{

  FMC_SDRAMInitStruct->FMC_Bank = FMC_Bank1_SDRAM;
  FMC_SDRAMInitStruct->FMC_ColumnBitsNumber = FMC_ColumnBits_Number_8b;
  FMC_SDRAMInitStruct->FMC_RowBitsNumber = FMC_RowBits_Number_11b;
  FMC_SDRAMInitStruct->FMC_SDMemoryDataWidth = FMC_SDMemory_Width_16b;
  FMC_SDRAMInitStruct->FMC_InternalBankNumber = FMC_InternalBank_Number_4;
  FMC_SDRAMInitStruct->FMC_CASLatency = FMC_CAS_Latency_1;
  FMC_SDRAMInitStruct->FMC_WriteProtection = FMC_Write_Protection_Enable;
  FMC_SDRAMInitStruct->FMC_SDClockPeriod = FMC_SDClock_Disable;
  FMC_SDRAMInitStruct->FMC_ReadBurst = FMC_Read_Burst_Disable;
  FMC_SDRAMInitStruct->FMC_ReadPipeDelay = FMC_ReadPipe_Delay_0;

  FMC_SDRAMInitStruct->FMC_SDRAMTimingStruct->FMC_LoadToActiveDelay = 16;
  FMC_SDRAMInitStruct->FMC_SDRAMTimingStruct->FMC_ExitSelfRefreshDelay = 16;
  FMC_SDRAMInitStruct->FMC_SDRAMTimingStruct->FMC_SelfRefreshTime = 16;
  FMC_SDRAMInitStruct->FMC_SDRAMTimingStruct->FMC_RowCycleDelay = 16;
  FMC_SDRAMInitStruct->FMC_SDRAMTimingStruct->FMC_WriteRecoveryTime = 16;
  FMC_SDRAMInitStruct->FMC_SDRAMTimingStruct->FMC_RPDelay = 16;
  FMC_SDRAMInitStruct->FMC_SDRAMTimingStruct->FMC_RCDDelay = 16;

}


void FMC_SDRAMCmdConfig(FMC_SDRAMCommandTypeDef* FMC_SDRAMCommandStruct)
{
  uint32_t tmpr = 0x0;


  assert_param(IS_FMC_COMMAND_MODE(FMC_SDRAMCommandStruct->FMC_CommandMode));
  assert_param(IS_FMC_COMMAND_TARGET(FMC_SDRAMCommandStruct->FMC_CommandTarget));
  assert_param(IS_FMC_AUTOREFRESH_NUMBER(FMC_SDRAMCommandStruct->FMC_AutoRefreshNumber));
  assert_param(IS_FMC_MODE_REGISTER(FMC_SDRAMCommandStruct->FMC_ModeRegisterDefinition));

  tmpr =   (uint32_t)(FMC_SDRAMCommandStruct->FMC_CommandMode |
                      FMC_SDRAMCommandStruct->FMC_CommandTarget |
                     (((FMC_SDRAMCommandStruct->FMC_AutoRefreshNumber)-1)<<5) |
                     ((FMC_SDRAMCommandStruct->FMC_ModeRegisterDefinition)<<9));

  FMC_Bank5_6->SDCMR = tmpr;

}


uint32_t FMC_GetModeStatus(uint32_t SDRAM_Bank)
{
  uint32_t tmpreg = 0;


  assert_param(IS_FMC_SDRAM_BANK(SDRAM_Bank));


  if(SDRAM_Bank == FMC_Bank1_SDRAM)
  {
    tmpreg = (uint32_t)(FMC_Bank5_6->SDSR & FMC_SDSR_MODES1);
  }
  else
  {
    tmpreg = ((uint32_t)(FMC_Bank5_6->SDSR & FMC_SDSR_MODES2) >> 2);
  }


  return tmpreg;
}


void FMC_SetRefreshCount(uint32_t FMC_Count)
{

  assert_param(IS_FMC_REFRESH_COUNT(FMC_Count));

  FMC_Bank5_6->SDRTR |= (FMC_Count<<1);

}


void FMC_SetAutoRefresh_Number(uint32_t FMC_Number)
{

  assert_param(IS_FMC_AUTOREFRESH_NUMBER(FMC_Number));

  FMC_Bank5_6->SDCMR |= (FMC_Number << 5);
}


void FMC_SDRAMWriteProtectionConfig(uint32_t SDRAM_Bank, FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));
  assert_param(IS_FMC_SDRAM_BANK(SDRAM_Bank));

  if (NewState != DISABLE)
  {
    FMC_Bank5_6->SDCR[SDRAM_Bank] |= FMC_Write_Protection_Enable;
  }
  else
  {
    FMC_Bank5_6->SDCR[SDRAM_Bank] &= SDCR_WriteProtection_RESET;
  }

}


void FMC_ITConfig(uint32_t FMC_Bank, uint32_t FMC_IT, FunctionalState NewState)
{
  assert_param(IS_FMC_IT_BANK(FMC_Bank));
  assert_param(IS_FMC_IT(FMC_IT));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    if(FMC_Bank == FMC_Bank2_NAND)
    {
      FMC_Bank2->SR2 |= FMC_IT;
    }

    else if (FMC_Bank == FMC_Bank3_NAND)
    {
      FMC_Bank3->SR3 |= FMC_IT;
    }

    else if (FMC_Bank == FMC_Bank4_PCCARD)
    {
      FMC_Bank4->SR4 |= FMC_IT;
    }

    else
    {

      FMC_Bank5_6->SDRTR |= FMC_IT;
    }
  }
  else
  {

    if(FMC_Bank == FMC_Bank2_NAND)
    {

      FMC_Bank2->SR2 &= (uint32_t)~FMC_IT;
    }

    else if (FMC_Bank == FMC_Bank3_NAND)
    {
      FMC_Bank3->SR3 &= (uint32_t)~FMC_IT;
    }

    else if(FMC_Bank == FMC_Bank4_PCCARD)
    {
      FMC_Bank4->SR4 &= (uint32_t)~FMC_IT;
    }

    else
    {

      FMC_Bank5_6->SDRTR &= (uint32_t)~FMC_IT;
    }
  }
}


FlagStatus FMC_GetFlagStatus(uint32_t FMC_Bank, uint32_t FMC_FLAG)
{
  FlagStatus bitstatus = RESET;
  uint32_t tmpsr = 0x00000000;


  assert_param(IS_FMC_GETFLAG_BANK(FMC_Bank));
  assert_param(IS_FMC_GET_FLAG(FMC_FLAG));

  if(FMC_Bank == FMC_Bank2_NAND)
  {
    tmpsr = FMC_Bank2->SR2;
  }
  else if(FMC_Bank == FMC_Bank3_NAND)
  {
    tmpsr = FMC_Bank3->SR3;
  }
  else if(FMC_Bank == FMC_Bank4_PCCARD)
  {
    tmpsr = FMC_Bank4->SR4;
  }
  else
  {
    tmpsr = FMC_Bank5_6->SDSR;
  }


  if ((tmpsr & FMC_FLAG) != FMC_FLAG )
  {
    bitstatus = RESET;
  }
  else
  {
    bitstatus = SET;
  }

  return bitstatus;
}


void FMC_ClearFlag(uint32_t FMC_Bank, uint32_t FMC_FLAG)
{

  assert_param(IS_FMC_GETFLAG_BANK(FMC_Bank));
  assert_param(IS_FMC_CLEAR_FLAG(FMC_FLAG)) ;

  if(FMC_Bank == FMC_Bank2_NAND)
  {
    FMC_Bank2->SR2 &= (~FMC_FLAG);
  }
  else if(FMC_Bank == FMC_Bank3_NAND)
  {
    FMC_Bank3->SR3 &= (~FMC_FLAG);
  }
  else if(FMC_Bank == FMC_Bank4_PCCARD)
  {
    FMC_Bank4->SR4 &= (~FMC_FLAG);
  }

  else
  {
    FMC_Bank5_6->SDRTR &= (~FMC_FLAG);
  }

}


ITStatus FMC_GetITStatus(uint32_t FMC_Bank, uint32_t FMC_IT)
{
  ITStatus bitstatus = RESET;
  uint32_t tmpsr = 0x0;
  uint32_t tmpsr2 = 0x0;
  uint32_t itstatus = 0x0;
  uint32_t itenable = 0x0;


  assert_param(IS_FMC_IT_BANK(FMC_Bank));
  assert_param(IS_FMC_GET_IT(FMC_IT));

  if(FMC_Bank == FMC_Bank2_NAND)
  {
    tmpsr = FMC_Bank2->SR2;
  }
  else if(FMC_Bank == FMC_Bank3_NAND)
  {
    tmpsr = FMC_Bank3->SR3;
  }
  else if(FMC_Bank == FMC_Bank4_PCCARD)
  {
    tmpsr = FMC_Bank4->SR4;
  }

  else
  {
    tmpsr = FMC_Bank5_6->SDRTR;
    tmpsr2 = FMC_Bank5_6->SDSR;
  }


  itenable = tmpsr & FMC_IT;


  if((FMC_Bank == FMC_Bank1_SDRAM) || (FMC_Bank == FMC_Bank2_SDRAM))
  {
    itstatus = tmpsr2 & FMC_SDSR_RE;
  }
  else
  {
    itstatus = tmpsr & (FMC_IT >> 3);
  }

  if ((itstatus != (uint32_t)RESET)  && (itenable != (uint32_t)RESET))
  {
    bitstatus = SET;
  }
  else
  {
    bitstatus = RESET;
  }
  return bitstatus;
}


void FMC_ClearITPendingBit(uint32_t FMC_Bank, uint32_t FMC_IT)
{

  assert_param(IS_FMC_IT_BANK(FMC_Bank));
  assert_param(IS_FMC_IT(FMC_IT));

  if(FMC_Bank == FMC_Bank2_NAND)
  {
    FMC_Bank2->SR2 &= ~(FMC_IT >> 3);
  }
  else if(FMC_Bank == FMC_Bank3_NAND)
  {
    FMC_Bank3->SR3 &= ~(FMC_IT >> 3);
  }
  else if(FMC_Bank == FMC_Bank4_PCCARD)
  {
    FMC_Bank4->SR4 &= ~(FMC_IT >> 3);
  }

  else
  {
    FMC_Bank5_6->SDRTR |= FMC_SDRTR_CRE;
  }
}
