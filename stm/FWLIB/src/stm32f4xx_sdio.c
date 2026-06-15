/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        stm32f4xx_sdio.c
 *
 * Description:  stm32f4xx_sdio.c module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#include "stm32f4xx_sdio.h"
#include "stm32f4xx_rcc.h"


#define SDIO_OFFSET                (SDIO_BASE - PERIPH_BASE)


#define CLKCR_OFFSET              (SDIO_OFFSET + 0x04)
#define CLKEN_BitNumber           0x08
#define CLKCR_CLKEN_BB            (PERIPH_BB_BASE + (CLKCR_OFFSET * 32) + (CLKEN_BitNumber * 4))


#define CMD_OFFSET                (SDIO_OFFSET + 0x0C)
#define SDIOSUSPEND_BitNumber     0x0B
#define CMD_SDIOSUSPEND_BB        (PERIPH_BB_BASE + (CMD_OFFSET * 32) + (SDIOSUSPEND_BitNumber * 4))


#define ENCMDCOMPL_BitNumber      0x0C
#define CMD_ENCMDCOMPL_BB         (PERIPH_BB_BASE + (CMD_OFFSET * 32) + (ENCMDCOMPL_BitNumber * 4))


#define NIEN_BitNumber            0x0D
#define CMD_NIEN_BB               (PERIPH_BB_BASE + (CMD_OFFSET * 32) + (NIEN_BitNumber * 4))


#define ATACMD_BitNumber          0x0E
#define CMD_ATACMD_BB             (PERIPH_BB_BASE + (CMD_OFFSET * 32) + (ATACMD_BitNumber * 4))


#define DCTRL_OFFSET              (SDIO_OFFSET + 0x2C)
#define DMAEN_BitNumber           0x03
#define DCTRL_DMAEN_BB            (PERIPH_BB_BASE + (DCTRL_OFFSET * 32) + (DMAEN_BitNumber * 4))


#define RWSTART_BitNumber         0x08
#define DCTRL_RWSTART_BB          (PERIPH_BB_BASE + (DCTRL_OFFSET * 32) + (RWSTART_BitNumber * 4))


#define RWSTOP_BitNumber          0x09
#define DCTRL_RWSTOP_BB           (PERIPH_BB_BASE + (DCTRL_OFFSET * 32) + (RWSTOP_BitNumber * 4))


#define RWMOD_BitNumber           0x0A
#define DCTRL_RWMOD_BB            (PERIPH_BB_BASE + (DCTRL_OFFSET * 32) + (RWMOD_BitNumber * 4))


#define SDIOEN_BitNumber          0x0B
#define DCTRL_SDIOEN_BB           (PERIPH_BB_BASE + (DCTRL_OFFSET * 32) + (SDIOEN_BitNumber * 4))


#define CLKCR_CLEAR_MASK         ((uint32_t)0xFFFF8100)


#define PWR_PWRCTRL_MASK         ((uint32_t)0xFFFFFFFC)


#define DCTRL_CLEAR_MASK         ((uint32_t)0xFFFFFF08)


#define CMD_CLEAR_MASK           ((uint32_t)0xFFFFF800)


#define SDIO_RESP_ADDR           ((uint32_t)(SDIO_BASE + 0x14))


void SDIO_DeInit(void)
{
  RCC_APB2PeriphResetCmd(RCC_APB2Periph_SDIO, ENABLE);
  RCC_APB2PeriphResetCmd(RCC_APB2Periph_SDIO, DISABLE);
}


void SDIO_Init(SDIO_InitTypeDef* SDIO_InitStruct)
{
  uint32_t tmpreg = 0;


  assert_param(IS_SDIO_CLOCK_EDGE(SDIO_InitStruct->SDIO_ClockEdge));
  assert_param(IS_SDIO_CLOCK_BYPASS(SDIO_InitStruct->SDIO_ClockBypass));
  assert_param(IS_SDIO_CLOCK_POWER_SAVE(SDIO_InitStruct->SDIO_ClockPowerSave));
  assert_param(IS_SDIO_BUS_WIDE(SDIO_InitStruct->SDIO_BusWide));
  assert_param(IS_SDIO_HARDWARE_FLOW_CONTROL(SDIO_InitStruct->SDIO_HardwareFlowControl));


  tmpreg = SDIO->CLKCR;


  tmpreg &= CLKCR_CLEAR_MASK;


  tmpreg |= (SDIO_InitStruct->SDIO_ClockDiv  | SDIO_InitStruct->SDIO_ClockPowerSave |
             SDIO_InitStruct->SDIO_ClockBypass | SDIO_InitStruct->SDIO_BusWide |
             SDIO_InitStruct->SDIO_ClockEdge | SDIO_InitStruct->SDIO_HardwareFlowControl);


  SDIO->CLKCR = tmpreg;
}


void SDIO_StructInit(SDIO_InitTypeDef* SDIO_InitStruct)
{

  SDIO_InitStruct->SDIO_ClockDiv = 0x00;
  SDIO_InitStruct->SDIO_ClockEdge = SDIO_ClockEdge_Rising;
  SDIO_InitStruct->SDIO_ClockBypass = SDIO_ClockBypass_Disable;
  SDIO_InitStruct->SDIO_ClockPowerSave = SDIO_ClockPowerSave_Disable;
  SDIO_InitStruct->SDIO_BusWide = SDIO_BusWide_1b;
  SDIO_InitStruct->SDIO_HardwareFlowControl = SDIO_HardwareFlowControl_Disable;
}


void SDIO_ClockCmd(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));

  *(__IO uint32_t *) CLKCR_CLKEN_BB = (uint32_t)NewState;
}


void SDIO_SetPowerState(uint32_t SDIO_PowerState)
{

  assert_param(IS_SDIO_POWER_STATE(SDIO_PowerState));

  SDIO->POWER = SDIO_PowerState;
}


uint32_t SDIO_GetPowerState(void)
{
  return (SDIO->POWER & (~PWR_PWRCTRL_MASK));
}


void SDIO_SendCommand(SDIO_CmdInitTypeDef *SDIO_CmdInitStruct)
{
  uint32_t tmpreg = 0;


  assert_param(IS_SDIO_CMD_INDEX(SDIO_CmdInitStruct->SDIO_CmdIndex));
  assert_param(IS_SDIO_RESPONSE(SDIO_CmdInitStruct->SDIO_Response));
  assert_param(IS_SDIO_WAIT(SDIO_CmdInitStruct->SDIO_Wait));
  assert_param(IS_SDIO_CPSM(SDIO_CmdInitStruct->SDIO_CPSM));


  SDIO->ARG = SDIO_CmdInitStruct->SDIO_Argument;


  tmpreg = SDIO->CMD;

  tmpreg &= CMD_CLEAR_MASK;


  tmpreg |= (uint32_t)SDIO_CmdInitStruct->SDIO_CmdIndex | SDIO_CmdInitStruct->SDIO_Response
           | SDIO_CmdInitStruct->SDIO_Wait | SDIO_CmdInitStruct->SDIO_CPSM;


  SDIO->CMD = tmpreg;
}


void SDIO_CmdStructInit(SDIO_CmdInitTypeDef* SDIO_CmdInitStruct)
{

  SDIO_CmdInitStruct->SDIO_Argument = 0x00;
  SDIO_CmdInitStruct->SDIO_CmdIndex = 0x00;
  SDIO_CmdInitStruct->SDIO_Response = SDIO_Response_No;
  SDIO_CmdInitStruct->SDIO_Wait = SDIO_Wait_No;
  SDIO_CmdInitStruct->SDIO_CPSM = SDIO_CPSM_Disable;
}


uint8_t SDIO_GetCommandResponse(void)
{
  return (uint8_t)(SDIO->RESPCMD);
}


uint32_t SDIO_GetResponse(uint32_t SDIO_RESP)
{
  __IO uint32_t tmp = 0;


  assert_param(IS_SDIO_RESP(SDIO_RESP));

  tmp = SDIO_RESP_ADDR + SDIO_RESP;

  return (*(__IO uint32_t *) tmp);
}


void SDIO_DataConfig(SDIO_DataInitTypeDef* SDIO_DataInitStruct)
{
  uint32_t tmpreg = 0;


  assert_param(IS_SDIO_DATA_LENGTH(SDIO_DataInitStruct->SDIO_DataLength));
  assert_param(IS_SDIO_BLOCK_SIZE(SDIO_DataInitStruct->SDIO_DataBlockSize));
  assert_param(IS_SDIO_TRANSFER_DIR(SDIO_DataInitStruct->SDIO_TransferDir));
  assert_param(IS_SDIO_TRANSFER_MODE(SDIO_DataInitStruct->SDIO_TransferMode));
  assert_param(IS_SDIO_DPSM(SDIO_DataInitStruct->SDIO_DPSM));


  SDIO->DTIMER = SDIO_DataInitStruct->SDIO_DataTimeOut;


  SDIO->DLEN = SDIO_DataInitStruct->SDIO_DataLength;


  tmpreg = SDIO->DCTRL;

  tmpreg &= DCTRL_CLEAR_MASK;


  tmpreg |= (uint32_t)SDIO_DataInitStruct->SDIO_DataBlockSize | SDIO_DataInitStruct->SDIO_TransferDir
           | SDIO_DataInitStruct->SDIO_TransferMode | SDIO_DataInitStruct->SDIO_DPSM;


  SDIO->DCTRL = tmpreg;
}


void SDIO_DataStructInit(SDIO_DataInitTypeDef* SDIO_DataInitStruct)
{

  SDIO_DataInitStruct->SDIO_DataTimeOut = 0xFFFFFFFF;
  SDIO_DataInitStruct->SDIO_DataLength = 0x00;
  SDIO_DataInitStruct->SDIO_DataBlockSize = SDIO_DataBlockSize_1b;
  SDIO_DataInitStruct->SDIO_TransferDir = SDIO_TransferDir_ToCard;
  SDIO_DataInitStruct->SDIO_TransferMode = SDIO_TransferMode_Block;
  SDIO_DataInitStruct->SDIO_DPSM = SDIO_DPSM_Disable;
}


uint32_t SDIO_GetDataCounter(void)
{
  return SDIO->DCOUNT;
}


uint32_t SDIO_ReadData(void)
{
  return SDIO->FIFO;
}


void SDIO_WriteData(uint32_t Data)
{
  SDIO->FIFO = Data;
}


uint32_t SDIO_GetFIFOCount(void)
{
  return SDIO->FIFOCNT;
}


void SDIO_StartSDIOReadWait(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));

  *(__IO uint32_t *) DCTRL_RWSTART_BB = (uint32_t) NewState;
}


void SDIO_StopSDIOReadWait(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));

  *(__IO uint32_t *) DCTRL_RWSTOP_BB = (uint32_t) NewState;
}


void SDIO_SetSDIOReadWaitMode(uint32_t SDIO_ReadWaitMode)
{

  assert_param(IS_SDIO_READWAIT_MODE(SDIO_ReadWaitMode));

  *(__IO uint32_t *) DCTRL_RWMOD_BB = SDIO_ReadWaitMode;
}


void SDIO_SetSDIOOperation(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));

  *(__IO uint32_t *) DCTRL_SDIOEN_BB = (uint32_t)NewState;
}


void SDIO_SendSDIOSuspendCmd(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));

  *(__IO uint32_t *) CMD_SDIOSUSPEND_BB = (uint32_t)NewState;
}


void SDIO_CommandCompletionCmd(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));

  *(__IO uint32_t *) CMD_ENCMDCOMPL_BB = (uint32_t)NewState;
}


void SDIO_CEATAITCmd(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));

  *(__IO uint32_t *) CMD_NIEN_BB = (uint32_t)((~((uint32_t)NewState)) & ((uint32_t)0x1));
}


void SDIO_SendCEATACmd(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));

  *(__IO uint32_t *) CMD_ATACMD_BB = (uint32_t)NewState;
}


void SDIO_DMACmd(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));

  *(__IO uint32_t *) DCTRL_DMAEN_BB = (uint32_t)NewState;
}


void SDIO_ITConfig(uint32_t SDIO_IT, FunctionalState NewState)
{

  assert_param(IS_SDIO_IT(SDIO_IT));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    SDIO->MASK |= SDIO_IT;
  }
  else
  {

    SDIO->MASK &= ~SDIO_IT;
  }
}


FlagStatus SDIO_GetFlagStatus(uint32_t SDIO_FLAG)
{
  FlagStatus bitstatus = RESET;


  assert_param(IS_SDIO_FLAG(SDIO_FLAG));

  if ((SDIO->STA & SDIO_FLAG) != (uint32_t)RESET)
  {
    bitstatus = SET;
  }
  else
  {
    bitstatus = RESET;
  }
  return bitstatus;
}


void SDIO_ClearFlag(uint32_t SDIO_FLAG)
{

  assert_param(IS_SDIO_CLEAR_FLAG(SDIO_FLAG));

  SDIO->ICR = SDIO_FLAG;
}


ITStatus SDIO_GetITStatus(uint32_t SDIO_IT)
{
  ITStatus bitstatus = RESET;


  assert_param(IS_SDIO_GET_IT(SDIO_IT));
  if ((SDIO->STA & SDIO_IT) != (uint32_t)RESET)
  {
    bitstatus = SET;
  }
  else
  {
    bitstatus = RESET;
  }
  return bitstatus;
}


void SDIO_ClearITPendingBit(uint32_t SDIO_IT)
{

  assert_param(IS_SDIO_CLEAR_IT(SDIO_IT));

  SDIO->ICR = SDIO_IT;
}
