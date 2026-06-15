/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        stm32f4xx_can.c
 *
 * Description:  stm32f4xx_can.c module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#include "stm32f4xx_can.h"
#include "stm32f4xx_rcc.h"


#define MCR_DBF           ((uint32_t)0x00010000)


#define TMIDxR_TXRQ       ((uint32_t)0x00000001)


#define FMR_FINIT         ((uint32_t)0x00000001)


#define INAK_TIMEOUT      ((uint32_t)0x0000FFFF)

#define SLAK_TIMEOUT      ((uint32_t)0x0000FFFF)


#define CAN_FLAGS_TSR     ((uint32_t)0x08000000)

#define CAN_FLAGS_RF1R    ((uint32_t)0x04000000)

#define CAN_FLAGS_RF0R    ((uint32_t)0x02000000)

#define CAN_FLAGS_MSR     ((uint32_t)0x01000000)

#define CAN_FLAGS_ESR     ((uint32_t)0x00F00000)


#define CAN_TXMAILBOX_0   ((uint8_t)0x00)
#define CAN_TXMAILBOX_1   ((uint8_t)0x01)
#define CAN_TXMAILBOX_2   ((uint8_t)0x02)

#define CAN_MODE_MASK     ((uint32_t) 0x00000003)


static ITStatus CheckITStatus(uint32_t CAN_Reg, uint32_t It_Bit);


void CAN_DeInit(CAN_TypeDef* CANx)
{

  assert_param(IS_CAN_ALL_PERIPH(CANx));

  if (CANx == CAN1)
  {

    RCC_APB1PeriphResetCmd(RCC_APB1Periph_CAN1, ENABLE);

    RCC_APB1PeriphResetCmd(RCC_APB1Periph_CAN1, DISABLE);
  }
  else
  {

    RCC_APB1PeriphResetCmd(RCC_APB1Periph_CAN2, ENABLE);

    RCC_APB1PeriphResetCmd(RCC_APB1Periph_CAN2, DISABLE);
  }
}


uint8_t CAN_Init(CAN_TypeDef* CANx, CAN_InitTypeDef* CAN_InitStruct)
{
  uint8_t InitStatus = CAN_InitStatus_Failed;
  uint32_t wait_ack = 0x00000000;

  assert_param(IS_CAN_ALL_PERIPH(CANx));
  assert_param(IS_FUNCTIONAL_STATE(CAN_InitStruct->CAN_TTCM));
  assert_param(IS_FUNCTIONAL_STATE(CAN_InitStruct->CAN_ABOM));
  assert_param(IS_FUNCTIONAL_STATE(CAN_InitStruct->CAN_AWUM));
  assert_param(IS_FUNCTIONAL_STATE(CAN_InitStruct->CAN_NART));
  assert_param(IS_FUNCTIONAL_STATE(CAN_InitStruct->CAN_RFLM));
  assert_param(IS_FUNCTIONAL_STATE(CAN_InitStruct->CAN_TXFP));
  assert_param(IS_CAN_MODE(CAN_InitStruct->CAN_Mode));
  assert_param(IS_CAN_SJW(CAN_InitStruct->CAN_SJW));
  assert_param(IS_CAN_BS1(CAN_InitStruct->CAN_BS1));
  assert_param(IS_CAN_BS2(CAN_InitStruct->CAN_BS2));
  assert_param(IS_CAN_PRESCALER(CAN_InitStruct->CAN_Prescaler));


  CANx->MCR &= (~(uint32_t)CAN_MCR_SLEEP);


  CANx->MCR |= CAN_MCR_INRQ ;


  while (((CANx->MSR & CAN_MSR_INAK) != CAN_MSR_INAK) && (wait_ack != INAK_TIMEOUT))
  {
    wait_ack++;
  }


  if ((CANx->MSR & CAN_MSR_INAK) != CAN_MSR_INAK)
  {
    InitStatus = CAN_InitStatus_Failed;
  }
  else
  {

    if (CAN_InitStruct->CAN_TTCM == ENABLE)
    {
      CANx->MCR |= CAN_MCR_TTCM;
    }
    else
    {
      CANx->MCR &= ~(uint32_t)CAN_MCR_TTCM;
    }


    if (CAN_InitStruct->CAN_ABOM == ENABLE)
    {
      CANx->MCR |= CAN_MCR_ABOM;
    }
    else
    {
      CANx->MCR &= ~(uint32_t)CAN_MCR_ABOM;
    }


    if (CAN_InitStruct->CAN_AWUM == ENABLE)
    {
      CANx->MCR |= CAN_MCR_AWUM;
    }
    else
    {
      CANx->MCR &= ~(uint32_t)CAN_MCR_AWUM;
    }


    if (CAN_InitStruct->CAN_NART == ENABLE)
    {
      CANx->MCR |= CAN_MCR_NART;
    }
    else
    {
      CANx->MCR &= ~(uint32_t)CAN_MCR_NART;
    }


    if (CAN_InitStruct->CAN_RFLM == ENABLE)
    {
      CANx->MCR |= CAN_MCR_RFLM;
    }
    else
    {
      CANx->MCR &= ~(uint32_t)CAN_MCR_RFLM;
    }


    if (CAN_InitStruct->CAN_TXFP == ENABLE)
    {
      CANx->MCR |= CAN_MCR_TXFP;
    }
    else
    {
      CANx->MCR &= ~(uint32_t)CAN_MCR_TXFP;
    }


    CANx->BTR = (uint32_t)((uint32_t)CAN_InitStruct->CAN_Mode << 30) | \
                ((uint32_t)CAN_InitStruct->CAN_SJW << 24) | \
                ((uint32_t)CAN_InitStruct->CAN_BS1 << 16) | \
                ((uint32_t)CAN_InitStruct->CAN_BS2 << 20) | \
               ((uint32_t)CAN_InitStruct->CAN_Prescaler - 1);


    CANx->MCR &= ~(uint32_t)CAN_MCR_INRQ;


   wait_ack = 0;

   while (((CANx->MSR & CAN_MSR_INAK) == CAN_MSR_INAK) && (wait_ack != INAK_TIMEOUT))
   {
     wait_ack++;
   }


    if ((CANx->MSR & CAN_MSR_INAK) == CAN_MSR_INAK)
    {
      InitStatus = CAN_InitStatus_Failed;
    }
    else
    {
      InitStatus = CAN_InitStatus_Success ;
    }
  }


  return InitStatus;
}


void CAN_FilterInit(CAN_FilterInitTypeDef* CAN_FilterInitStruct)
{
  uint32_t filter_number_bit_pos = 0;

  assert_param(IS_CAN_FILTER_NUMBER(CAN_FilterInitStruct->CAN_FilterNumber));
  assert_param(IS_CAN_FILTER_MODE(CAN_FilterInitStruct->CAN_FilterMode));
  assert_param(IS_CAN_FILTER_SCALE(CAN_FilterInitStruct->CAN_FilterScale));
  assert_param(IS_CAN_FILTER_FIFO(CAN_FilterInitStruct->CAN_FilterFIFOAssignment));
  assert_param(IS_FUNCTIONAL_STATE(CAN_FilterInitStruct->CAN_FilterActivation));

  filter_number_bit_pos = ((uint32_t)1) << CAN_FilterInitStruct->CAN_FilterNumber;


  CAN1->FMR |= FMR_FINIT;


  CAN1->FA1R &= ~(uint32_t)filter_number_bit_pos;


  if (CAN_FilterInitStruct->CAN_FilterScale == CAN_FilterScale_16bit)
  {

    CAN1->FS1R &= ~(uint32_t)filter_number_bit_pos;


    CAN1->sFilterRegister[CAN_FilterInitStruct->CAN_FilterNumber].FR1 =
       ((0x0000FFFF & (uint32_t)CAN_FilterInitStruct->CAN_FilterMaskIdLow) << 16) |
        (0x0000FFFF & (uint32_t)CAN_FilterInitStruct->CAN_FilterIdLow);


    CAN1->sFilterRegister[CAN_FilterInitStruct->CAN_FilterNumber].FR2 =
       ((0x0000FFFF & (uint32_t)CAN_FilterInitStruct->CAN_FilterMaskIdHigh) << 16) |
        (0x0000FFFF & (uint32_t)CAN_FilterInitStruct->CAN_FilterIdHigh);
  }

  if (CAN_FilterInitStruct->CAN_FilterScale == CAN_FilterScale_32bit)
  {

    CAN1->FS1R |= filter_number_bit_pos;

    CAN1->sFilterRegister[CAN_FilterInitStruct->CAN_FilterNumber].FR1 =
       ((0x0000FFFF & (uint32_t)CAN_FilterInitStruct->CAN_FilterIdHigh) << 16) |
        (0x0000FFFF & (uint32_t)CAN_FilterInitStruct->CAN_FilterIdLow);

    CAN1->sFilterRegister[CAN_FilterInitStruct->CAN_FilterNumber].FR2 =
       ((0x0000FFFF & (uint32_t)CAN_FilterInitStruct->CAN_FilterMaskIdHigh) << 16) |
        (0x0000FFFF & (uint32_t)CAN_FilterInitStruct->CAN_FilterMaskIdLow);
  }


  if (CAN_FilterInitStruct->CAN_FilterMode == CAN_FilterMode_IdMask)
  {

    CAN1->FM1R &= ~(uint32_t)filter_number_bit_pos;
  }
  else
  {

    CAN1->FM1R |= (uint32_t)filter_number_bit_pos;
  }


  if (CAN_FilterInitStruct->CAN_FilterFIFOAssignment == CAN_Filter_FIFO0)
  {

    CAN1->FFA1R &= ~(uint32_t)filter_number_bit_pos;
  }

  if (CAN_FilterInitStruct->CAN_FilterFIFOAssignment == CAN_Filter_FIFO1)
  {

    CAN1->FFA1R |= (uint32_t)filter_number_bit_pos;
  }


  if (CAN_FilterInitStruct->CAN_FilterActivation == ENABLE)
  {
    CAN1->FA1R |= filter_number_bit_pos;
  }


  CAN1->FMR &= ~FMR_FINIT;
}


void CAN_StructInit(CAN_InitTypeDef* CAN_InitStruct)
{


  CAN_InitStruct->CAN_TTCM = DISABLE;


  CAN_InitStruct->CAN_ABOM = DISABLE;


  CAN_InitStruct->CAN_AWUM = DISABLE;


  CAN_InitStruct->CAN_NART = DISABLE;


  CAN_InitStruct->CAN_RFLM = DISABLE;


  CAN_InitStruct->CAN_TXFP = DISABLE;


  CAN_InitStruct->CAN_Mode = CAN_Mode_Normal;


  CAN_InitStruct->CAN_SJW = CAN_SJW_1tq;


  CAN_InitStruct->CAN_BS1 = CAN_BS1_4tq;


  CAN_InitStruct->CAN_BS2 = CAN_BS2_3tq;


  CAN_InitStruct->CAN_Prescaler = 1;
}


void CAN_SlaveStartBank(uint8_t CAN_BankNumber)
{

  assert_param(IS_CAN_BANKNUMBER(CAN_BankNumber));


  CAN1->FMR |= FMR_FINIT;


  CAN1->FMR &= (uint32_t)0xFFFFC0F1 ;
  CAN1->FMR |= (uint32_t)(CAN_BankNumber)<<8;


  CAN1->FMR &= ~FMR_FINIT;
}


void CAN_DBGFreeze(CAN_TypeDef* CANx, FunctionalState NewState)
{

  assert_param(IS_CAN_ALL_PERIPH(CANx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    CANx->MCR |= MCR_DBF;
  }
  else
  {

    CANx->MCR &= ~MCR_DBF;
  }
}


void CAN_TTComModeCmd(CAN_TypeDef* CANx, FunctionalState NewState)
{

  assert_param(IS_CAN_ALL_PERIPH(CANx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {

    CANx->MCR |= CAN_MCR_TTCM;


    CANx->sTxMailBox[0].TDTR |= ((uint32_t)CAN_TDT0R_TGT);
    CANx->sTxMailBox[1].TDTR |= ((uint32_t)CAN_TDT1R_TGT);
    CANx->sTxMailBox[2].TDTR |= ((uint32_t)CAN_TDT2R_TGT);
  }
  else
  {

    CANx->MCR &= (uint32_t)(~(uint32_t)CAN_MCR_TTCM);


    CANx->sTxMailBox[0].TDTR &= ((uint32_t)~CAN_TDT0R_TGT);
    CANx->sTxMailBox[1].TDTR &= ((uint32_t)~CAN_TDT1R_TGT);
    CANx->sTxMailBox[2].TDTR &= ((uint32_t)~CAN_TDT2R_TGT);
  }
}


uint8_t CAN_Transmit(CAN_TypeDef* CANx, CanTxMsg* TxMessage)
{
  uint8_t transmit_mailbox = 0;

  assert_param(IS_CAN_ALL_PERIPH(CANx));
  assert_param(IS_CAN_IDTYPE(TxMessage->IDE));
  assert_param(IS_CAN_RTR(TxMessage->RTR));
  assert_param(IS_CAN_DLC(TxMessage->DLC));


  if ((CANx->TSR&CAN_TSR_TME0) == CAN_TSR_TME0)
  {
    transmit_mailbox = 0;
  }
  else if ((CANx->TSR&CAN_TSR_TME1) == CAN_TSR_TME1)
  {
    transmit_mailbox = 1;
  }
  else if ((CANx->TSR&CAN_TSR_TME2) == CAN_TSR_TME2)
  {
    transmit_mailbox = 2;
  }
  else
  {
    transmit_mailbox = CAN_TxStatus_NoMailBox;
  }

  if (transmit_mailbox != CAN_TxStatus_NoMailBox)
  {

    CANx->sTxMailBox[transmit_mailbox].TIR &= TMIDxR_TXRQ;
    if (TxMessage->IDE == CAN_Id_Standard)
    {
      assert_param(IS_CAN_STDID(TxMessage->StdId));
      CANx->sTxMailBox[transmit_mailbox].TIR |= ((TxMessage->StdId << 21) | \
                                                  TxMessage->RTR);
    }
    else
    {
      assert_param(IS_CAN_EXTID(TxMessage->ExtId));
      CANx->sTxMailBox[transmit_mailbox].TIR |= ((TxMessage->ExtId << 3) | \
                                                  TxMessage->IDE | \
                                                  TxMessage->RTR);
    }


    TxMessage->DLC &= (uint8_t)0x0000000F;
    CANx->sTxMailBox[transmit_mailbox].TDTR &= (uint32_t)0xFFFFFFF0;
    CANx->sTxMailBox[transmit_mailbox].TDTR |= TxMessage->DLC;


    CANx->sTxMailBox[transmit_mailbox].TDLR = (((uint32_t)TxMessage->Data[3] << 24) |
                                             ((uint32_t)TxMessage->Data[2] << 16) |
                                             ((uint32_t)TxMessage->Data[1] << 8) |
                                             ((uint32_t)TxMessage->Data[0]));
    CANx->sTxMailBox[transmit_mailbox].TDHR = (((uint32_t)TxMessage->Data[7] << 24) |
                                             ((uint32_t)TxMessage->Data[6] << 16) |
                                             ((uint32_t)TxMessage->Data[5] << 8) |
                                             ((uint32_t)TxMessage->Data[4]));

    CANx->sTxMailBox[transmit_mailbox].TIR |= TMIDxR_TXRQ;
  }
  return transmit_mailbox;
}


uint8_t CAN_TransmitStatus(CAN_TypeDef* CANx, uint8_t TransmitMailbox)
{
  uint32_t state = 0;


  assert_param(IS_CAN_ALL_PERIPH(CANx));
  assert_param(IS_CAN_TRANSMITMAILBOX(TransmitMailbox));

  switch (TransmitMailbox)
  {
    case (CAN_TXMAILBOX_0):
      state =   CANx->TSR &  (CAN_TSR_RQCP0 | CAN_TSR_TXOK0 | CAN_TSR_TME0);
      break;
    case (CAN_TXMAILBOX_1):
      state =   CANx->TSR &  (CAN_TSR_RQCP1 | CAN_TSR_TXOK1 | CAN_TSR_TME1);
      break;
    case (CAN_TXMAILBOX_2):
      state =   CANx->TSR &  (CAN_TSR_RQCP2 | CAN_TSR_TXOK2 | CAN_TSR_TME2);
      break;
    default:
      state = CAN_TxStatus_Failed;
      break;
  }
  switch (state)
  {

    case (0x0): state = CAN_TxStatus_Pending;
      break;

     case (CAN_TSR_RQCP0 | CAN_TSR_TME0): state = CAN_TxStatus_Failed;
      break;
     case (CAN_TSR_RQCP1 | CAN_TSR_TME1): state = CAN_TxStatus_Failed;
      break;
     case (CAN_TSR_RQCP2 | CAN_TSR_TME2): state = CAN_TxStatus_Failed;
      break;

    case (CAN_TSR_RQCP0 | CAN_TSR_TXOK0 | CAN_TSR_TME0):state = CAN_TxStatus_Ok;
      break;
    case (CAN_TSR_RQCP1 | CAN_TSR_TXOK1 | CAN_TSR_TME1):state = CAN_TxStatus_Ok;
      break;
    case (CAN_TSR_RQCP2 | CAN_TSR_TXOK2 | CAN_TSR_TME2):state = CAN_TxStatus_Ok;
      break;
    default: state = CAN_TxStatus_Failed;
      break;
  }
  return (uint8_t) state;
}


void CAN_CancelTransmit(CAN_TypeDef* CANx, uint8_t Mailbox)
{

  assert_param(IS_CAN_ALL_PERIPH(CANx));
  assert_param(IS_CAN_TRANSMITMAILBOX(Mailbox));

  switch (Mailbox)
  {
    case (CAN_TXMAILBOX_0): CANx->TSR |= CAN_TSR_ABRQ0;
      break;
    case (CAN_TXMAILBOX_1): CANx->TSR |= CAN_TSR_ABRQ1;
      break;
    case (CAN_TXMAILBOX_2): CANx->TSR |= CAN_TSR_ABRQ2;
      break;
    default:
      break;
  }
}


void CAN_Receive(CAN_TypeDef* CANx, uint8_t FIFONumber, CanRxMsg* RxMessage)
{

  assert_param(IS_CAN_ALL_PERIPH(CANx));
  assert_param(IS_CAN_FIFO(FIFONumber));

  RxMessage->IDE = (uint8_t)0x04 & CANx->sFIFOMailBox[FIFONumber].RIR;
  if (RxMessage->IDE == CAN_Id_Standard)
  {
    RxMessage->StdId = (uint32_t)0x000007FF & (CANx->sFIFOMailBox[FIFONumber].RIR >> 21);
  }
  else
  {
    RxMessage->ExtId = (uint32_t)0x1FFFFFFF & (CANx->sFIFOMailBox[FIFONumber].RIR >> 3);
  }

  RxMessage->RTR = (uint8_t)0x02 & CANx->sFIFOMailBox[FIFONumber].RIR;

  RxMessage->DLC = (uint8_t)0x0F & CANx->sFIFOMailBox[FIFONumber].RDTR;

  RxMessage->FMI = (uint8_t)0xFF & (CANx->sFIFOMailBox[FIFONumber].RDTR >> 8);

  RxMessage->Data[0] = (uint8_t)0xFF & CANx->sFIFOMailBox[FIFONumber].RDLR;
  RxMessage->Data[1] = (uint8_t)0xFF & (CANx->sFIFOMailBox[FIFONumber].RDLR >> 8);
  RxMessage->Data[2] = (uint8_t)0xFF & (CANx->sFIFOMailBox[FIFONumber].RDLR >> 16);
  RxMessage->Data[3] = (uint8_t)0xFF & (CANx->sFIFOMailBox[FIFONumber].RDLR >> 24);
  RxMessage->Data[4] = (uint8_t)0xFF & CANx->sFIFOMailBox[FIFONumber].RDHR;
  RxMessage->Data[5] = (uint8_t)0xFF & (CANx->sFIFOMailBox[FIFONumber].RDHR >> 8);
  RxMessage->Data[6] = (uint8_t)0xFF & (CANx->sFIFOMailBox[FIFONumber].RDHR >> 16);
  RxMessage->Data[7] = (uint8_t)0xFF & (CANx->sFIFOMailBox[FIFONumber].RDHR >> 24);


  if (FIFONumber == CAN_FIFO0)
  {
    CANx->RF0R |= CAN_RF0R_RFOM0;
  }

  else
  {
    CANx->RF1R |= CAN_RF1R_RFOM1;
  }
}


void CAN_FIFORelease(CAN_TypeDef* CANx, uint8_t FIFONumber)
{

  assert_param(IS_CAN_ALL_PERIPH(CANx));
  assert_param(IS_CAN_FIFO(FIFONumber));

  if (FIFONumber == CAN_FIFO0)
  {
    CANx->RF0R |= CAN_RF0R_RFOM0;
  }

  else
  {
    CANx->RF1R |= CAN_RF1R_RFOM1;
  }
}


uint8_t CAN_MessagePending(CAN_TypeDef* CANx, uint8_t FIFONumber)
{
  uint8_t message_pending=0;

  assert_param(IS_CAN_ALL_PERIPH(CANx));
  assert_param(IS_CAN_FIFO(FIFONumber));
  if (FIFONumber == CAN_FIFO0)
  {
    message_pending = (uint8_t)(CANx->RF0R&(uint32_t)0x03);
  }
  else if (FIFONumber == CAN_FIFO1)
  {
    message_pending = (uint8_t)(CANx->RF1R&(uint32_t)0x03);
  }
  else
  {
    message_pending = 0;
  }
  return message_pending;
}


uint8_t CAN_OperatingModeRequest(CAN_TypeDef* CANx, uint8_t CAN_OperatingMode)
{
  uint8_t status = CAN_ModeStatus_Failed;


  uint32_t timeout = INAK_TIMEOUT;


  assert_param(IS_CAN_ALL_PERIPH(CANx));
  assert_param(IS_CAN_OPERATING_MODE(CAN_OperatingMode));

  if (CAN_OperatingMode == CAN_OperatingMode_Initialization)
  {

    CANx->MCR = (uint32_t)((CANx->MCR & (uint32_t)(~(uint32_t)CAN_MCR_SLEEP)) | CAN_MCR_INRQ);


    while (((CANx->MSR & CAN_MODE_MASK) != CAN_MSR_INAK) && (timeout != 0))
    {
      timeout--;
    }
    if ((CANx->MSR & CAN_MODE_MASK) != CAN_MSR_INAK)
    {
      status = CAN_ModeStatus_Failed;
    }
    else
    {
      status = CAN_ModeStatus_Success;
    }
  }
  else  if (CAN_OperatingMode == CAN_OperatingMode_Normal)
  {

    CANx->MCR &= (uint32_t)(~(CAN_MCR_SLEEP|CAN_MCR_INRQ));


    while (((CANx->MSR & CAN_MODE_MASK) != 0) && (timeout!=0))
    {
      timeout--;
    }
    if ((CANx->MSR & CAN_MODE_MASK) != 0)
    {
      status = CAN_ModeStatus_Failed;
    }
    else
    {
      status = CAN_ModeStatus_Success;
    }
  }
  else  if (CAN_OperatingMode == CAN_OperatingMode_Sleep)
  {

    CANx->MCR = (uint32_t)((CANx->MCR & (uint32_t)(~(uint32_t)CAN_MCR_INRQ)) | CAN_MCR_SLEEP);


    while (((CANx->MSR & CAN_MODE_MASK) != CAN_MSR_SLAK) && (timeout!=0))
    {
      timeout--;
    }
    if ((CANx->MSR & CAN_MODE_MASK) != CAN_MSR_SLAK)
    {
      status = CAN_ModeStatus_Failed;
    }
    else
    {
      status = CAN_ModeStatus_Success;
    }
  }
  else
  {
    status = CAN_ModeStatus_Failed;
  }

  return  (uint8_t) status;
}


uint8_t CAN_Sleep(CAN_TypeDef* CANx)
{
  uint8_t sleepstatus = CAN_Sleep_Failed;


  assert_param(IS_CAN_ALL_PERIPH(CANx));


   CANx->MCR = (((CANx->MCR) & (uint32_t)(~(uint32_t)CAN_MCR_INRQ)) | CAN_MCR_SLEEP);


  if ((CANx->MSR & (CAN_MSR_SLAK|CAN_MSR_INAK)) == CAN_MSR_SLAK)
  {

    sleepstatus =  CAN_Sleep_Ok;
  }

   return (uint8_t)sleepstatus;
}


uint8_t CAN_WakeUp(CAN_TypeDef* CANx)
{
  uint32_t wait_slak = SLAK_TIMEOUT;
  uint8_t wakeupstatus = CAN_WakeUp_Failed;


  assert_param(IS_CAN_ALL_PERIPH(CANx));


  CANx->MCR &= ~(uint32_t)CAN_MCR_SLEEP;


  while(((CANx->MSR & CAN_MSR_SLAK) == CAN_MSR_SLAK)&&(wait_slak!=0x00))
  {
   wait_slak--;
  }
  if((CANx->MSR & CAN_MSR_SLAK) != CAN_MSR_SLAK)
  {

    wakeupstatus = CAN_WakeUp_Ok;
  }

  return (uint8_t)wakeupstatus;
}


uint8_t CAN_GetLastErrorCode(CAN_TypeDef* CANx)
{
  uint8_t errorcode=0;


  assert_param(IS_CAN_ALL_PERIPH(CANx));


  errorcode = (((uint8_t)CANx->ESR) & (uint8_t)CAN_ESR_LEC);


  return errorcode;
}


uint8_t CAN_GetReceiveErrorCounter(CAN_TypeDef* CANx)
{
  uint8_t counter=0;


  assert_param(IS_CAN_ALL_PERIPH(CANx));


  counter = (uint8_t)((CANx->ESR & CAN_ESR_REC)>> 24);


  return counter;
}


uint8_t CAN_GetLSBTransmitErrorCounter(CAN_TypeDef* CANx)
{
  uint8_t counter=0;


  assert_param(IS_CAN_ALL_PERIPH(CANx));


  counter = (uint8_t)((CANx->ESR & CAN_ESR_TEC)>> 16);


  return counter;
}


void CAN_ITConfig(CAN_TypeDef* CANx, uint32_t CAN_IT, FunctionalState NewState)
{

  assert_param(IS_CAN_ALL_PERIPH(CANx));
  assert_param(IS_CAN_IT(CAN_IT));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    CANx->IER |= CAN_IT;
  }
  else
  {

    CANx->IER &= ~CAN_IT;
  }
}

FlagStatus CAN_GetFlagStatus(CAN_TypeDef* CANx, uint32_t CAN_FLAG)
{
  FlagStatus bitstatus = RESET;


  assert_param(IS_CAN_ALL_PERIPH(CANx));
  assert_param(IS_CAN_GET_FLAG(CAN_FLAG));


  if((CAN_FLAG & CAN_FLAGS_ESR) != (uint32_t)RESET)
  {

    if ((CANx->ESR & (CAN_FLAG & 0x000FFFFF)) != (uint32_t)RESET)
    {

      bitstatus = SET;
    }
    else
    {

      bitstatus = RESET;
    }
  }
  else if((CAN_FLAG & CAN_FLAGS_MSR) != (uint32_t)RESET)
  {

    if ((CANx->MSR & (CAN_FLAG & 0x000FFFFF)) != (uint32_t)RESET)
    {

      bitstatus = SET;
    }
    else
    {

      bitstatus = RESET;
    }
  }
  else if((CAN_FLAG & CAN_FLAGS_TSR) != (uint32_t)RESET)
  {

    if ((CANx->TSR & (CAN_FLAG & 0x000FFFFF)) != (uint32_t)RESET)
    {

      bitstatus = SET;
    }
    else
    {

      bitstatus = RESET;
    }
  }
  else if((CAN_FLAG & CAN_FLAGS_RF0R) != (uint32_t)RESET)
  {

    if ((CANx->RF0R & (CAN_FLAG & 0x000FFFFF)) != (uint32_t)RESET)
    {

      bitstatus = SET;
    }
    else
    {

      bitstatus = RESET;
    }
  }
  else
  {

    if ((uint32_t)(CANx->RF1R & (CAN_FLAG & 0x000FFFFF)) != (uint32_t)RESET)
    {

      bitstatus = SET;
    }
    else
    {

      bitstatus = RESET;
    }
  }

  return  bitstatus;
}


void CAN_ClearFlag(CAN_TypeDef* CANx, uint32_t CAN_FLAG)
{
  uint32_t flagtmp=0;

  assert_param(IS_CAN_ALL_PERIPH(CANx));
  assert_param(IS_CAN_CLEAR_FLAG(CAN_FLAG));

  if (CAN_FLAG == CAN_FLAG_LEC)
  {

    CANx->ESR = (uint32_t)RESET;
  }
  else
  {
    flagtmp = CAN_FLAG & 0x000FFFFF;

    if ((CAN_FLAG & CAN_FLAGS_RF0R)!=(uint32_t)RESET)
    {

      CANx->RF0R = (uint32_t)(flagtmp);
    }
    else if ((CAN_FLAG & CAN_FLAGS_RF1R)!=(uint32_t)RESET)
    {

      CANx->RF1R = (uint32_t)(flagtmp);
    }
    else if ((CAN_FLAG & CAN_FLAGS_TSR)!=(uint32_t)RESET)
    {

      CANx->TSR = (uint32_t)(flagtmp);
    }
    else
    {

      CANx->MSR = (uint32_t)(flagtmp);
    }
  }
}


ITStatus CAN_GetITStatus(CAN_TypeDef* CANx, uint32_t CAN_IT)
{
  ITStatus itstatus = RESET;

  assert_param(IS_CAN_ALL_PERIPH(CANx));
  assert_param(IS_CAN_IT(CAN_IT));


 if((CANx->IER & CAN_IT) != RESET)
 {

    switch (CAN_IT)
    {
      case CAN_IT_TME:

        itstatus = CheckITStatus(CANx->TSR, CAN_TSR_RQCP0|CAN_TSR_RQCP1|CAN_TSR_RQCP2);
        break;
      case CAN_IT_FMP0:

        itstatus = CheckITStatus(CANx->RF0R, CAN_RF0R_FMP0);
        break;
      case CAN_IT_FF0:

        itstatus = CheckITStatus(CANx->RF0R, CAN_RF0R_FULL0);
        break;
      case CAN_IT_FOV0:

        itstatus = CheckITStatus(CANx->RF0R, CAN_RF0R_FOVR0);
        break;
      case CAN_IT_FMP1:

        itstatus = CheckITStatus(CANx->RF1R, CAN_RF1R_FMP1);
        break;
      case CAN_IT_FF1:

        itstatus = CheckITStatus(CANx->RF1R, CAN_RF1R_FULL1);
        break;
      case CAN_IT_FOV1:

        itstatus = CheckITStatus(CANx->RF1R, CAN_RF1R_FOVR1);
        break;
      case CAN_IT_WKU:

        itstatus = CheckITStatus(CANx->MSR, CAN_MSR_WKUI);
        break;
      case CAN_IT_SLK:

        itstatus = CheckITStatus(CANx->MSR, CAN_MSR_SLAKI);
        break;
      case CAN_IT_EWG:

        itstatus = CheckITStatus(CANx->ESR, CAN_ESR_EWGF);
        break;
      case CAN_IT_EPV:

        itstatus = CheckITStatus(CANx->ESR, CAN_ESR_EPVF);
        break;
      case CAN_IT_BOF:

        itstatus = CheckITStatus(CANx->ESR, CAN_ESR_BOFF);
        break;
      case CAN_IT_LEC:

        itstatus = CheckITStatus(CANx->ESR, CAN_ESR_LEC);
        break;
      case CAN_IT_ERR:

        itstatus = CheckITStatus(CANx->MSR, CAN_MSR_ERRI);
        break;
      default:

        itstatus = RESET;
        break;
    }
  }
  else
  {

    itstatus  = RESET;
  }


  return  itstatus;
}


void CAN_ClearITPendingBit(CAN_TypeDef* CANx, uint32_t CAN_IT)
{

  assert_param(IS_CAN_ALL_PERIPH(CANx));
  assert_param(IS_CAN_CLEAR_IT(CAN_IT));

  switch (CAN_IT)
  {
    case CAN_IT_TME:

      CANx->TSR = CAN_TSR_RQCP0|CAN_TSR_RQCP1|CAN_TSR_RQCP2;
      break;
    case CAN_IT_FF0:

      CANx->RF0R = CAN_RF0R_FULL0;
      break;
    case CAN_IT_FOV0:

      CANx->RF0R = CAN_RF0R_FOVR0;
      break;
    case CAN_IT_FF1:

      CANx->RF1R = CAN_RF1R_FULL1;
      break;
    case CAN_IT_FOV1:

      CANx->RF1R = CAN_RF1R_FOVR1;
      break;
    case CAN_IT_WKU:

      CANx->MSR = CAN_MSR_WKUI;
      break;
    case CAN_IT_SLK:

      CANx->MSR = CAN_MSR_SLAKI;
      break;
    case CAN_IT_EWG:

      CANx->MSR = CAN_MSR_ERRI;

      break;
    case CAN_IT_EPV:

      CANx->MSR = CAN_MSR_ERRI;

      break;
    case CAN_IT_BOF:

      CANx->MSR = CAN_MSR_ERRI;

       break;
    case CAN_IT_LEC:

      CANx->ESR = RESET;

      CANx->MSR = CAN_MSR_ERRI;
      break;
    case CAN_IT_ERR:

      CANx->ESR = RESET;

      CANx->MSR = CAN_MSR_ERRI;

       break;
    default:
       break;
   }
}


static ITStatus CheckITStatus(uint32_t CAN_Reg, uint32_t It_Bit)
{
  ITStatus pendingbitstatus = RESET;

  if ((CAN_Reg & It_Bit) != (uint32_t)RESET)
  {

    pendingbitstatus = SET;
  }
  else
  {

    pendingbitstatus = RESET;
  }
  return pendingbitstatus;
}
