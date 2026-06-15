/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        stm32f4xx_can.h
 *
 * Description:  stm32f4xx_can.h module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#ifndef __STM32F4xx_CAN_H
#define __STM32F4xx_CAN_H

#ifdef __cplusplus
 extern "C" {
#endif


#include "stm32f4xx.h"


#define IS_CAN_ALL_PERIPH(PERIPH) (((PERIPH) == CAN1) || \
                                   ((PERIPH) == CAN2))


typedef struct
{
  uint16_t CAN_Prescaler;

  uint8_t CAN_Mode;

  uint8_t CAN_SJW;

  uint8_t CAN_BS1;

  uint8_t CAN_BS2;

  FunctionalState CAN_TTCM;

  FunctionalState CAN_ABOM;

  FunctionalState CAN_AWUM;

  FunctionalState CAN_NART;

  FunctionalState CAN_RFLM;

  FunctionalState CAN_TXFP;
} CAN_InitTypeDef;


typedef struct
{
  uint16_t CAN_FilterIdHigh;

  uint16_t CAN_FilterIdLow;

  uint16_t CAN_FilterMaskIdHigh;

  uint16_t CAN_FilterMaskIdLow;

  uint16_t CAN_FilterFIFOAssignment;

  uint8_t CAN_FilterNumber;

  uint8_t CAN_FilterMode;

  uint8_t CAN_FilterScale;

  FunctionalState CAN_FilterActivation;
} CAN_FilterInitTypeDef;


typedef struct
{
  uint32_t StdId;

  uint32_t ExtId;

  uint8_t IDE;

  uint8_t RTR;

  uint8_t DLC;

  uint8_t Data[8];
} CanTxMsg;


typedef struct
{
  uint32_t StdId;

  uint32_t ExtId;

  uint8_t IDE;

  uint8_t RTR;

  uint8_t DLC;

  uint8_t Data[8];

  uint8_t FMI;
} CanRxMsg;


#define CAN_InitStatus_Failed              ((uint8_t)0x00)
#define CAN_InitStatus_Success             ((uint8_t)0x01)


#define CANINITFAILED    CAN_InitStatus_Failed
#define CANINITOK        CAN_InitStatus_Success


#define CAN_Mode_Normal             ((uint8_t)0x00)
#define CAN_Mode_LoopBack           ((uint8_t)0x01)
#define CAN_Mode_Silent             ((uint8_t)0x02)
#define CAN_Mode_Silent_LoopBack    ((uint8_t)0x03)

#define IS_CAN_MODE(MODE) (((MODE) == CAN_Mode_Normal) || \
                           ((MODE) == CAN_Mode_LoopBack)|| \
                           ((MODE) == CAN_Mode_Silent) || \
                           ((MODE) == CAN_Mode_Silent_LoopBack))


#define CAN_OperatingMode_Initialization  ((uint8_t)0x00)
#define CAN_OperatingMode_Normal          ((uint8_t)0x01)
#define CAN_OperatingMode_Sleep           ((uint8_t)0x02)


#define IS_CAN_OPERATING_MODE(MODE) (((MODE) == CAN_OperatingMode_Initialization) ||\
                                    ((MODE) == CAN_OperatingMode_Normal)|| \
																		((MODE) == CAN_OperatingMode_Sleep))


#define CAN_ModeStatus_Failed    ((uint8_t)0x00)
#define CAN_ModeStatus_Success   ((uint8_t)!CAN_ModeStatus_Failed)


#define CAN_SJW_1tq                 ((uint8_t)0x00)
#define CAN_SJW_2tq                 ((uint8_t)0x01)
#define CAN_SJW_3tq                 ((uint8_t)0x02)
#define CAN_SJW_4tq                 ((uint8_t)0x03)

#define IS_CAN_SJW(SJW) (((SJW) == CAN_SJW_1tq) || ((SJW) == CAN_SJW_2tq)|| \
                         ((SJW) == CAN_SJW_3tq) || ((SJW) == CAN_SJW_4tq))


#define CAN_BS1_1tq                 ((uint8_t)0x00)
#define CAN_BS1_2tq                 ((uint8_t)0x01)
#define CAN_BS1_3tq                 ((uint8_t)0x02)
#define CAN_BS1_4tq                 ((uint8_t)0x03)
#define CAN_BS1_5tq                 ((uint8_t)0x04)
#define CAN_BS1_6tq                 ((uint8_t)0x05)
#define CAN_BS1_7tq                 ((uint8_t)0x06)
#define CAN_BS1_8tq                 ((uint8_t)0x07)
#define CAN_BS1_9tq                 ((uint8_t)0x08)
#define CAN_BS1_10tq                ((uint8_t)0x09)
#define CAN_BS1_11tq                ((uint8_t)0x0A)
#define CAN_BS1_12tq                ((uint8_t)0x0B)
#define CAN_BS1_13tq                ((uint8_t)0x0C)
#define CAN_BS1_14tq                ((uint8_t)0x0D)
#define CAN_BS1_15tq                ((uint8_t)0x0E)
#define CAN_BS1_16tq                ((uint8_t)0x0F)

#define IS_CAN_BS1(BS1) ((BS1) <= CAN_BS1_16tq)


#define CAN_BS2_1tq                 ((uint8_t)0x00)
#define CAN_BS2_2tq                 ((uint8_t)0x01)
#define CAN_BS2_3tq                 ((uint8_t)0x02)
#define CAN_BS2_4tq                 ((uint8_t)0x03)
#define CAN_BS2_5tq                 ((uint8_t)0x04)
#define CAN_BS2_6tq                 ((uint8_t)0x05)
#define CAN_BS2_7tq                 ((uint8_t)0x06)
#define CAN_BS2_8tq                 ((uint8_t)0x07)

#define IS_CAN_BS2(BS2) ((BS2) <= CAN_BS2_8tq)


#define IS_CAN_PRESCALER(PRESCALER) (((PRESCALER) >= 1) && ((PRESCALER) <= 1024))


#define IS_CAN_FILTER_NUMBER(NUMBER) ((NUMBER) <= 27)


#define CAN_FilterMode_IdMask       ((uint8_t)0x00)
#define CAN_FilterMode_IdList       ((uint8_t)0x01)

#define IS_CAN_FILTER_MODE(MODE) (((MODE) == CAN_FilterMode_IdMask) || \
                                  ((MODE) == CAN_FilterMode_IdList))


#define CAN_FilterScale_16bit       ((uint8_t)0x00)
#define CAN_FilterScale_32bit       ((uint8_t)0x01)

#define IS_CAN_FILTER_SCALE(SCALE) (((SCALE) == CAN_FilterScale_16bit) || \
                                    ((SCALE) == CAN_FilterScale_32bit))


#define CAN_Filter_FIFO0             ((uint8_t)0x00)
#define CAN_Filter_FIFO1             ((uint8_t)0x01)
#define IS_CAN_FILTER_FIFO(FIFO) (((FIFO) == CAN_FilterFIFO0) || \
                                  ((FIFO) == CAN_FilterFIFO1))


#define CAN_FilterFIFO0  CAN_Filter_FIFO0
#define CAN_FilterFIFO1  CAN_Filter_FIFO1


#define IS_CAN_BANKNUMBER(BANKNUMBER) (((BANKNUMBER) >= 1) && ((BANKNUMBER) <= 27))


#define IS_CAN_TRANSMITMAILBOX(TRANSMITMAILBOX) ((TRANSMITMAILBOX) <= ((uint8_t)0x02))
#define IS_CAN_STDID(STDID)   ((STDID) <= ((uint32_t)0x7FF))
#define IS_CAN_EXTID(EXTID)   ((EXTID) <= ((uint32_t)0x1FFFFFFF))
#define IS_CAN_DLC(DLC)       ((DLC) <= ((uint8_t)0x08))


#define CAN_Id_Standard             ((uint32_t)0x00000000)
#define CAN_Id_Extended             ((uint32_t)0x00000004)
#define IS_CAN_IDTYPE(IDTYPE) (((IDTYPE) == CAN_Id_Standard) || \
                               ((IDTYPE) == CAN_Id_Extended))


#define CAN_ID_STD      CAN_Id_Standard
#define CAN_ID_EXT      CAN_Id_Extended


#define CAN_RTR_Data                ((uint32_t)0x00000000)
#define CAN_RTR_Remote              ((uint32_t)0x00000002)
#define IS_CAN_RTR(RTR) (((RTR) == CAN_RTR_Data) || ((RTR) == CAN_RTR_Remote))


#define CAN_RTR_DATA     CAN_RTR_Data
#define CAN_RTR_REMOTE   CAN_RTR_Remote


#define CAN_TxStatus_Failed         ((uint8_t)0x00)
#define CAN_TxStatus_Ok             ((uint8_t)0x01)
#define CAN_TxStatus_Pending        ((uint8_t)0x02)
#define CAN_TxStatus_NoMailBox      ((uint8_t)0x04)

#define CANTXFAILED                  CAN_TxStatus_Failed
#define CANTXOK                      CAN_TxStatus_Ok
#define CANTXPENDING                 CAN_TxStatus_Pending
#define CAN_NO_MB                    CAN_TxStatus_NoMailBox


#define CAN_FIFO0                 ((uint8_t)0x00)
#define CAN_FIFO1                 ((uint8_t)0x01)

#define IS_CAN_FIFO(FIFO) (((FIFO) == CAN_FIFO0) || ((FIFO) == CAN_FIFO1))


#define CAN_Sleep_Failed     ((uint8_t)0x00)
#define CAN_Sleep_Ok         ((uint8_t)0x01)


#define CANSLEEPFAILED   CAN_Sleep_Failed
#define CANSLEEPOK       CAN_Sleep_Ok


#define CAN_WakeUp_Failed        ((uint8_t)0x00)
#define CAN_WakeUp_Ok            ((uint8_t)0x01)


#define CANWAKEUPFAILED   CAN_WakeUp_Failed
#define CANWAKEUPOK       CAN_WakeUp_Ok


#define CAN_ErrorCode_NoErr           ((uint8_t)0x00)
#define	CAN_ErrorCode_StuffErr        ((uint8_t)0x10)
#define	CAN_ErrorCode_FormErr         ((uint8_t)0x20)
#define	CAN_ErrorCode_ACKErr          ((uint8_t)0x30)
#define	CAN_ErrorCode_BitRecessiveErr ((uint8_t)0x40)
#define	CAN_ErrorCode_BitDominantErr  ((uint8_t)0x50)
#define	CAN_ErrorCode_CRCErr          ((uint8_t)0x60)
#define	CAN_ErrorCode_SoftwareSetErr  ((uint8_t)0x70)


#define CAN_FLAG_RQCP0             ((uint32_t)0x38000001)
#define CAN_FLAG_RQCP1             ((uint32_t)0x38000100)
#define CAN_FLAG_RQCP2             ((uint32_t)0x38010000)


#define CAN_FLAG_FMP0              ((uint32_t)0x12000003)
#define CAN_FLAG_FF0               ((uint32_t)0x32000008)
#define CAN_FLAG_FOV0              ((uint32_t)0x32000010)
#define CAN_FLAG_FMP1              ((uint32_t)0x14000003)
#define CAN_FLAG_FF1               ((uint32_t)0x34000008)
#define CAN_FLAG_FOV1              ((uint32_t)0x34000010)


#define CAN_FLAG_WKU               ((uint32_t)0x31000008)
#define CAN_FLAG_SLAK              ((uint32_t)0x31000012)


#define CAN_FLAG_EWG               ((uint32_t)0x10F00001)
#define CAN_FLAG_EPV               ((uint32_t)0x10F00002)
#define CAN_FLAG_BOF               ((uint32_t)0x10F00004)
#define CAN_FLAG_LEC               ((uint32_t)0x30F00070)

#define IS_CAN_GET_FLAG(FLAG) (((FLAG) == CAN_FLAG_LEC)  || ((FLAG) == CAN_FLAG_BOF)   || \
                               ((FLAG) == CAN_FLAG_EPV)  || ((FLAG) == CAN_FLAG_EWG)   || \
                               ((FLAG) == CAN_FLAG_WKU)  || ((FLAG) == CAN_FLAG_FOV0)  || \
                               ((FLAG) == CAN_FLAG_FF0)  || ((FLAG) == CAN_FLAG_FMP0)  || \
                               ((FLAG) == CAN_FLAG_FOV1) || ((FLAG) == CAN_FLAG_FF1)   || \
                               ((FLAG) == CAN_FLAG_FMP1) || ((FLAG) == CAN_FLAG_RQCP2) || \
                               ((FLAG) == CAN_FLAG_RQCP1)|| ((FLAG) == CAN_FLAG_RQCP0) || \
                               ((FLAG) == CAN_FLAG_SLAK ))

#define IS_CAN_CLEAR_FLAG(FLAG)(((FLAG) == CAN_FLAG_LEC) || ((FLAG) == CAN_FLAG_RQCP2) || \
                                ((FLAG) == CAN_FLAG_RQCP1)  || ((FLAG) == CAN_FLAG_RQCP0) || \
                                ((FLAG) == CAN_FLAG_FF0)  || ((FLAG) == CAN_FLAG_FOV0) ||\
                                ((FLAG) == CAN_FLAG_FF1) || ((FLAG) == CAN_FLAG_FOV1) || \
                                ((FLAG) == CAN_FLAG_WKU) || ((FLAG) == CAN_FLAG_SLAK))


#define CAN_IT_TME                  ((uint32_t)0x00000001)


#define CAN_IT_FMP0                 ((uint32_t)0x00000002)
#define CAN_IT_FF0                  ((uint32_t)0x00000004)
#define CAN_IT_FOV0                 ((uint32_t)0x00000008)
#define CAN_IT_FMP1                 ((uint32_t)0x00000010)
#define CAN_IT_FF1                  ((uint32_t)0x00000020)
#define CAN_IT_FOV1                 ((uint32_t)0x00000040)


#define CAN_IT_WKU                  ((uint32_t)0x00010000)
#define CAN_IT_SLK                  ((uint32_t)0x00020000)


#define CAN_IT_EWG                  ((uint32_t)0x00000100)
#define CAN_IT_EPV                  ((uint32_t)0x00000200)
#define CAN_IT_BOF                  ((uint32_t)0x00000400)
#define CAN_IT_LEC                  ((uint32_t)0x00000800)
#define CAN_IT_ERR                  ((uint32_t)0x00008000)


#define CAN_IT_RQCP0   CAN_IT_TME
#define CAN_IT_RQCP1   CAN_IT_TME
#define CAN_IT_RQCP2   CAN_IT_TME


#define IS_CAN_IT(IT)        (((IT) == CAN_IT_TME) || ((IT) == CAN_IT_FMP0)  ||\
                             ((IT) == CAN_IT_FF0)  || ((IT) == CAN_IT_FOV0)  ||\
                             ((IT) == CAN_IT_FMP1) || ((IT) == CAN_IT_FF1)   ||\
                             ((IT) == CAN_IT_FOV1) || ((IT) == CAN_IT_EWG)   ||\
                             ((IT) == CAN_IT_EPV)  || ((IT) == CAN_IT_BOF)   ||\
                             ((IT) == CAN_IT_LEC)  || ((IT) == CAN_IT_ERR)   ||\
                             ((IT) == CAN_IT_WKU)  || ((IT) == CAN_IT_SLK))

#define IS_CAN_CLEAR_IT(IT) (((IT) == CAN_IT_TME) || ((IT) == CAN_IT_FF0)    ||\
                             ((IT) == CAN_IT_FOV0)|| ((IT) == CAN_IT_FF1)    ||\
                             ((IT) == CAN_IT_FOV1)|| ((IT) == CAN_IT_EWG)    ||\
                             ((IT) == CAN_IT_EPV) || ((IT) == CAN_IT_BOF)    ||\
                             ((IT) == CAN_IT_LEC) || ((IT) == CAN_IT_ERR)    ||\
                             ((IT) == CAN_IT_WKU) || ((IT) == CAN_IT_SLK))


void CAN_DeInit(CAN_TypeDef* CANx);


uint8_t CAN_Init(CAN_TypeDef* CANx, CAN_InitTypeDef* CAN_InitStruct);
void CAN_FilterInit(CAN_FilterInitTypeDef* CAN_FilterInitStruct);
void CAN_StructInit(CAN_InitTypeDef* CAN_InitStruct);
void CAN_SlaveStartBank(uint8_t CAN_BankNumber);
void CAN_DBGFreeze(CAN_TypeDef* CANx, FunctionalState NewState);
void CAN_TTComModeCmd(CAN_TypeDef* CANx, FunctionalState NewState);


uint8_t CAN_Transmit(CAN_TypeDef* CANx, CanTxMsg* TxMessage);
uint8_t CAN_TransmitStatus(CAN_TypeDef* CANx, uint8_t TransmitMailbox);
void CAN_CancelTransmit(CAN_TypeDef* CANx, uint8_t Mailbox);


void CAN_Receive(CAN_TypeDef* CANx, uint8_t FIFONumber, CanRxMsg* RxMessage);
void CAN_FIFORelease(CAN_TypeDef* CANx, uint8_t FIFONumber);
uint8_t CAN_MessagePending(CAN_TypeDef* CANx, uint8_t FIFONumber);


uint8_t CAN_OperatingModeRequest(CAN_TypeDef* CANx, uint8_t CAN_OperatingMode);
uint8_t CAN_Sleep(CAN_TypeDef* CANx);
uint8_t CAN_WakeUp(CAN_TypeDef* CANx);


uint8_t CAN_GetLastErrorCode(CAN_TypeDef* CANx);
uint8_t CAN_GetReceiveErrorCounter(CAN_TypeDef* CANx);
uint8_t CAN_GetLSBTransmitErrorCounter(CAN_TypeDef* CANx);


void CAN_ITConfig(CAN_TypeDef* CANx, uint32_t CAN_IT, FunctionalState NewState);
FlagStatus CAN_GetFlagStatus(CAN_TypeDef* CANx, uint32_t CAN_FLAG);
void CAN_ClearFlag(CAN_TypeDef* CANx, uint32_t CAN_FLAG);
ITStatus CAN_GetITStatus(CAN_TypeDef* CANx, uint32_t CAN_IT);
void CAN_ClearITPendingBit(CAN_TypeDef* CANx, uint32_t CAN_IT);

#ifdef __cplusplus
}
#endif

#endif
