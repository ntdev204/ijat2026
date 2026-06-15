/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        stm32f4xx_flash.h
 *
 * Description:  stm32f4xx_flash.h module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#ifndef __STM32F4xx_FLASH_H
#define __STM32F4xx_FLASH_H

#ifdef __cplusplus
 extern "C" {
#endif


#include "stm32f4xx.h"


typedef enum
{
  FLASH_BUSY = 1,
  FLASH_ERROR_RD,
  FLASH_ERROR_PGS,
  FLASH_ERROR_PGP,
  FLASH_ERROR_PGA,
  FLASH_ERROR_WRP,
  FLASH_ERROR_PROGRAM,
  FLASH_ERROR_OPERATION,
  FLASH_COMPLETE
}FLASH_Status;


#define FLASH_Latency_0                ((uint8_t)0x0000)
#define FLASH_Latency_1                ((uint8_t)0x0001)
#define FLASH_Latency_2                ((uint8_t)0x0002)
#define FLASH_Latency_3                ((uint8_t)0x0003)
#define FLASH_Latency_4                ((uint8_t)0x0004)
#define FLASH_Latency_5                ((uint8_t)0x0005)
#define FLASH_Latency_6                ((uint8_t)0x0006)
#define FLASH_Latency_7                ((uint8_t)0x0007)
#define FLASH_Latency_8                ((uint8_t)0x0008)
#define FLASH_Latency_9                ((uint8_t)0x0009)
#define FLASH_Latency_10               ((uint8_t)0x000A)
#define FLASH_Latency_11               ((uint8_t)0x000B)
#define FLASH_Latency_12               ((uint8_t)0x000C)
#define FLASH_Latency_13               ((uint8_t)0x000D)
#define FLASH_Latency_14               ((uint8_t)0x000E)
#define FLASH_Latency_15               ((uint8_t)0x000F)


#define IS_FLASH_LATENCY(LATENCY) (((LATENCY) == FLASH_Latency_0)  || \
                                   ((LATENCY) == FLASH_Latency_1)  || \
                                   ((LATENCY) == FLASH_Latency_2)  || \
                                   ((LATENCY) == FLASH_Latency_3)  || \
                                   ((LATENCY) == FLASH_Latency_4)  || \
                                   ((LATENCY) == FLASH_Latency_5)  || \
                                   ((LATENCY) == FLASH_Latency_6)  || \
                                   ((LATENCY) == FLASH_Latency_7)  || \
                                   ((LATENCY) == FLASH_Latency_8)  || \
                                   ((LATENCY) == FLASH_Latency_9)  || \
                                   ((LATENCY) == FLASH_Latency_10) || \
                                   ((LATENCY) == FLASH_Latency_11) || \
                                   ((LATENCY) == FLASH_Latency_12) || \
                                   ((LATENCY) == FLASH_Latency_13) || \
                                   ((LATENCY) == FLASH_Latency_14) || \
                                   ((LATENCY) == FLASH_Latency_15))


#define VoltageRange_1        ((uint8_t)0x00)
#define VoltageRange_2        ((uint8_t)0x01)
#define VoltageRange_3        ((uint8_t)0x02)
#define VoltageRange_4        ((uint8_t)0x03)

#define IS_VOLTAGERANGE(RANGE)(((RANGE) == VoltageRange_1) || \
                               ((RANGE) == VoltageRange_2) || \
                               ((RANGE) == VoltageRange_3) || \
                               ((RANGE) == VoltageRange_4))


#define FLASH_Sector_0     ((uint16_t)0x0000)
#define FLASH_Sector_1     ((uint16_t)0x0008)
#define FLASH_Sector_2     ((uint16_t)0x0010)
#define FLASH_Sector_3     ((uint16_t)0x0018)
#define FLASH_Sector_4     ((uint16_t)0x0020)
#define FLASH_Sector_5     ((uint16_t)0x0028)
#define FLASH_Sector_6     ((uint16_t)0x0030)
#define FLASH_Sector_7     ((uint16_t)0x0038)
#define FLASH_Sector_8     ((uint16_t)0x0040)
#define FLASH_Sector_9     ((uint16_t)0x0048)
#define FLASH_Sector_10    ((uint16_t)0x0050)
#define FLASH_Sector_11    ((uint16_t)0x0058)
#define FLASH_Sector_12    ((uint16_t)0x0080)
#define FLASH_Sector_13    ((uint16_t)0x0088)
#define FLASH_Sector_14    ((uint16_t)0x0090)
#define FLASH_Sector_15    ((uint16_t)0x0098)
#define FLASH_Sector_16    ((uint16_t)0x00A0)
#define FLASH_Sector_17    ((uint16_t)0x00A8)
#define FLASH_Sector_18    ((uint16_t)0x00B0)
#define FLASH_Sector_19    ((uint16_t)0x00B8)
#define FLASH_Sector_20    ((uint16_t)0x00C0)
#define FLASH_Sector_21    ((uint16_t)0x00C8)
#define FLASH_Sector_22    ((uint16_t)0x00D0)
#define FLASH_Sector_23    ((uint16_t)0x00D8)

#define IS_FLASH_SECTOR(SECTOR) (((SECTOR) == FLASH_Sector_0)   || ((SECTOR) == FLASH_Sector_1)   ||\
                                 ((SECTOR) == FLASH_Sector_2)   || ((SECTOR) == FLASH_Sector_3)   ||\
                                 ((SECTOR) == FLASH_Sector_4)   || ((SECTOR) == FLASH_Sector_5)   ||\
                                 ((SECTOR) == FLASH_Sector_6)   || ((SECTOR) == FLASH_Sector_7)   ||\
                                 ((SECTOR) == FLASH_Sector_8)   || ((SECTOR) == FLASH_Sector_9)   ||\
                                 ((SECTOR) == FLASH_Sector_10)  || ((SECTOR) == FLASH_Sector_11)  ||\
                                 ((SECTOR) == FLASH_Sector_12)  || ((SECTOR) == FLASH_Sector_13)  ||\
                                 ((SECTOR) == FLASH_Sector_14)  || ((SECTOR) == FLASH_Sector_15)  ||\
                                 ((SECTOR) == FLASH_Sector_16)  || ((SECTOR) == FLASH_Sector_17)  ||\
                                 ((SECTOR) == FLASH_Sector_18)  || ((SECTOR) == FLASH_Sector_19)  ||\
                                 ((SECTOR) == FLASH_Sector_20)  || ((SECTOR) == FLASH_Sector_21)  ||\
                                 ((SECTOR) == FLASH_Sector_22)  || ((SECTOR) == FLASH_Sector_23))

#if defined (STM32F427_437xx) || defined (STM32F429_439xx)
#define IS_FLASH_ADDRESS(ADDRESS) ((((ADDRESS) >= 0x08000000) && ((ADDRESS) <= 0x081FFFFF)) ||\
                                   (((ADDRESS) >= 0x1FFF7800) && ((ADDRESS) <= 0x1FFF7A0F)))
#endif

#if defined (STM32F40_41xxx)
#define IS_FLASH_ADDRESS(ADDRESS) ((((ADDRESS) >= 0x08000000) && ((ADDRESS) <= 0x080FFFFF)) ||\
                                   (((ADDRESS) >= 0x1FFF7800) && ((ADDRESS) <= 0x1FFF7A0F)))
#endif

#if defined (STM32F401xx)
#define IS_FLASH_ADDRESS(ADDRESS) ((((ADDRESS) >= 0x08000000) && ((ADDRESS) <= 0x0803FFFF)) ||\
                                   (((ADDRESS) >= 0x1FFF7800) && ((ADDRESS) <= 0x1FFF7A0F)))
#endif

#if defined (STM32F411xE)
#define IS_FLASH_ADDRESS(ADDRESS) ((((ADDRESS) >= 0x08000000) && ((ADDRESS) <= 0x0807FFFF)) ||\
                                   (((ADDRESS) >= 0x1FFF7800) && ((ADDRESS) <= 0x1FFF7A0F)))
#endif


#define OB_WRP_Sector_0       ((uint32_t)0x00000001)
#define OB_WRP_Sector_1       ((uint32_t)0x00000002)
#define OB_WRP_Sector_2       ((uint32_t)0x00000004)
#define OB_WRP_Sector_3       ((uint32_t)0x00000008)
#define OB_WRP_Sector_4       ((uint32_t)0x00000010)
#define OB_WRP_Sector_5       ((uint32_t)0x00000020)
#define OB_WRP_Sector_6       ((uint32_t)0x00000040)
#define OB_WRP_Sector_7       ((uint32_t)0x00000080)
#define OB_WRP_Sector_8       ((uint32_t)0x00000100)
#define OB_WRP_Sector_9       ((uint32_t)0x00000200)
#define OB_WRP_Sector_10      ((uint32_t)0x00000400)
#define OB_WRP_Sector_11      ((uint32_t)0x00000800)
#define OB_WRP_Sector_12      ((uint32_t)0x00000001)
#define OB_WRP_Sector_13      ((uint32_t)0x00000002)
#define OB_WRP_Sector_14      ((uint32_t)0x00000004)
#define OB_WRP_Sector_15      ((uint32_t)0x00000008)
#define OB_WRP_Sector_16      ((uint32_t)0x00000010)
#define OB_WRP_Sector_17      ((uint32_t)0x00000020)
#define OB_WRP_Sector_18      ((uint32_t)0x00000040)
#define OB_WRP_Sector_19      ((uint32_t)0x00000080)
#define OB_WRP_Sector_20      ((uint32_t)0x00000100)
#define OB_WRP_Sector_21      ((uint32_t)0x00000200)
#define OB_WRP_Sector_22      ((uint32_t)0x00000400)
#define OB_WRP_Sector_23      ((uint32_t)0x00000800)
#define OB_WRP_Sector_All     ((uint32_t)0x00000FFF)

#define IS_OB_WRP(SECTOR)((((SECTOR) & (uint32_t)0xFFFFF000) == 0x00000000) && ((SECTOR) != 0x00000000))


#define OB_PcROP_Disable   ((uint8_t)0x00)
#define OB_PcROP_Enable    ((uint8_t)0x80)
#define IS_OB_PCROP_SELECT(PCROP) (((PCROP) == OB_PcROP_Disable) || ((PCROP) == OB_PcROP_Enable))


#define OB_PCROP_Sector_0        ((uint32_t)0x00000001)
#define OB_PCROP_Sector_1        ((uint32_t)0x00000002)
#define OB_PCROP_Sector_2        ((uint32_t)0x00000004)
#define OB_PCROP_Sector_3        ((uint32_t)0x00000008)
#define OB_PCROP_Sector_4        ((uint32_t)0x00000010)
#define OB_PCROP_Sector_5        ((uint32_t)0x00000020)
#define OB_PCROP_Sector_6        ((uint32_t)0x00000040)
#define OB_PCROP_Sector_7        ((uint32_t)0x00000080)
#define OB_PCROP_Sector_8        ((uint32_t)0x00000100)
#define OB_PCROP_Sector_9        ((uint32_t)0x00000200)
#define OB_PCROP_Sector_10       ((uint32_t)0x00000400)
#define OB_PCROP_Sector_11       ((uint32_t)0x00000800)
#define OB_PCROP_Sector_12       ((uint32_t)0x00000001)
#define OB_PCROP_Sector_13       ((uint32_t)0x00000002)
#define OB_PCROP_Sector_14       ((uint32_t)0x00000004)
#define OB_PCROP_Sector_15       ((uint32_t)0x00000008)
#define OB_PCROP_Sector_16       ((uint32_t)0x00000010)
#define OB_PCROP_Sector_17       ((uint32_t)0x00000020)
#define OB_PCROP_Sector_18       ((uint32_t)0x00000040)
#define OB_PCROP_Sector_19       ((uint32_t)0x00000080)
#define OB_PCROP_Sector_20       ((uint32_t)0x00000100)
#define OB_PCROP_Sector_21       ((uint32_t)0x00000200)
#define OB_PCROP_Sector_22       ((uint32_t)0x00000400)
#define OB_PCROP_Sector_23       ((uint32_t)0x00000800)
#define OB_PCROP_Sector_All      ((uint32_t)0x00000FFF)

#define IS_OB_PCROP(SECTOR)((((SECTOR) & (uint32_t)0xFFFFF000) == 0x00000000) && ((SECTOR) != 0x00000000))


#define OB_RDP_Level_0   ((uint8_t)0xAA)
#define OB_RDP_Level_1   ((uint8_t)0x55)

#define IS_OB_RDP(LEVEL) (((LEVEL) == OB_RDP_Level_0)||\
                          ((LEVEL) == OB_RDP_Level_1))


#define OB_IWDG_SW                     ((uint8_t)0x20)
#define OB_IWDG_HW                     ((uint8_t)0x00)
#define IS_OB_IWDG_SOURCE(SOURCE) (((SOURCE) == OB_IWDG_SW) || ((SOURCE) == OB_IWDG_HW))


#define OB_STOP_NoRST                  ((uint8_t)0x40)
#define OB_STOP_RST                    ((uint8_t)0x00)
#define IS_OB_STOP_SOURCE(SOURCE) (((SOURCE) == OB_STOP_NoRST) || ((SOURCE) == OB_STOP_RST))


#define OB_STDBY_NoRST                 ((uint8_t)0x80)
#define OB_STDBY_RST                   ((uint8_t)0x00)
#define IS_OB_STDBY_SOURCE(SOURCE) (((SOURCE) == OB_STDBY_NoRST) || ((SOURCE) == OB_STDBY_RST))


#define OB_BOR_LEVEL3          ((uint8_t)0x00)
#define OB_BOR_LEVEL2          ((uint8_t)0x04)
#define OB_BOR_LEVEL1          ((uint8_t)0x08)
#define OB_BOR_OFF             ((uint8_t)0x0C)
#define IS_OB_BOR(LEVEL) (((LEVEL) == OB_BOR_LEVEL1) || ((LEVEL) == OB_BOR_LEVEL2) ||\
                          ((LEVEL) == OB_BOR_LEVEL3) || ((LEVEL) == OB_BOR_OFF))


#define OB_Dual_BootEnabled   ((uint8_t)0x10)
#define OB_Dual_BootDisabled  ((uint8_t)0x00)
#define IS_OB_BOOT(BOOT) (((BOOT) == OB_Dual_BootEnabled) || ((BOOT) == OB_Dual_BootDisabled))


#define FLASH_IT_EOP                   ((uint32_t)0x01000000)
#define FLASH_IT_ERR                   ((uint32_t)0x02000000)
#define IS_FLASH_IT(IT) ((((IT) & (uint32_t)0xFCFFFFFF) == 0x00000000) && ((IT) != 0x00000000))


#define FLASH_FLAG_EOP                 ((uint32_t)0x00000001)
#define FLASH_FLAG_OPERR               ((uint32_t)0x00000002)
#define FLASH_FLAG_WRPERR              ((uint32_t)0x00000010)
#define FLASH_FLAG_PGAERR              ((uint32_t)0x00000020)
#define FLASH_FLAG_PGPERR              ((uint32_t)0x00000040)
#define FLASH_FLAG_PGSERR              ((uint32_t)0x00000080)
#define FLASH_FLAG_RDERR               ((uint32_t)0x00000100)
#define FLASH_FLAG_BSY                 ((uint32_t)0x00010000)
#define IS_FLASH_CLEAR_FLAG(FLAG) ((((FLAG) & (uint32_t)0xFFFFFE0C) == 0x00000000) && ((FLAG) != 0x00000000))
#define IS_FLASH_GET_FLAG(FLAG)  (((FLAG) == FLASH_FLAG_EOP)    || ((FLAG) == FLASH_FLAG_OPERR)  || \
                                  ((FLAG) == FLASH_FLAG_WRPERR) || ((FLAG) == FLASH_FLAG_PGAERR) || \
                                  ((FLAG) == FLASH_FLAG_PGPERR) || ((FLAG) == FLASH_FLAG_PGSERR) || \
                                  ((FLAG) == FLASH_FLAG_BSY)    || ((FLAG) == FLASH_FLAG_RDERR))


#define FLASH_PSIZE_BYTE           ((uint32_t)0x00000000)
#define FLASH_PSIZE_HALF_WORD      ((uint32_t)0x00000100)
#define FLASH_PSIZE_WORD           ((uint32_t)0x00000200)
#define FLASH_PSIZE_DOUBLE_WORD    ((uint32_t)0x00000300)
#define CR_PSIZE_MASK              ((uint32_t)0xFFFFFCFF)


#define RDP_KEY                  ((uint16_t)0x00A5)
#define FLASH_KEY1               ((uint32_t)0x45670123)
#define FLASH_KEY2               ((uint32_t)0xCDEF89AB)
#define FLASH_OPT_KEY1           ((uint32_t)0x08192A3B)
#define FLASH_OPT_KEY2           ((uint32_t)0x4C5D6E7F)


#define ACR_BYTE0_ADDRESS           ((uint32_t)0x40023C00)

#define OPTCR_BYTE0_ADDRESS         ((uint32_t)0x40023C14)

#define OPTCR_BYTE1_ADDRESS         ((uint32_t)0x40023C15)

#define OPTCR_BYTE2_ADDRESS         ((uint32_t)0x40023C16)

#define OPTCR_BYTE3_ADDRESS         ((uint32_t)0x40023C17)


#define OPTCR1_BYTE2_ADDRESS         ((uint32_t)0x40023C1A)


void FLASH_SetLatency(uint32_t FLASH_Latency);
void FLASH_PrefetchBufferCmd(FunctionalState NewState);
void FLASH_InstructionCacheCmd(FunctionalState NewState);
void FLASH_DataCacheCmd(FunctionalState NewState);
void FLASH_InstructionCacheReset(void);
void FLASH_DataCacheReset(void);


void         FLASH_Unlock(void);
void         FLASH_Lock(void);
FLASH_Status FLASH_EraseSector(uint32_t FLASH_Sector, uint8_t VoltageRange);
FLASH_Status FLASH_EraseAllSectors(uint8_t VoltageRange);
FLASH_Status FLASH_EraseAllBank1Sectors(uint8_t VoltageRange);
FLASH_Status FLASH_EraseAllBank2Sectors(uint8_t VoltageRange);
FLASH_Status FLASH_ProgramDoubleWord(uint32_t Address, uint64_t Data);
FLASH_Status FLASH_ProgramWord(uint32_t Address, uint32_t Data);
FLASH_Status FLASH_ProgramHalfWord(uint32_t Address, uint16_t Data);
FLASH_Status FLASH_ProgramByte(uint32_t Address, uint8_t Data);


void         FLASH_OB_Unlock(void);
void         FLASH_OB_Lock(void);
void         FLASH_OB_WRPConfig(uint32_t OB_WRP, FunctionalState NewState);
void         FLASH_OB_WRP1Config(uint32_t OB_WRP, FunctionalState NewState);
void         FLASH_OB_PCROPSelectionConfig(uint8_t OB_PcROP);
void         FLASH_OB_PCROPConfig(uint32_t OB_PCROP, FunctionalState NewState);
void         FLASH_OB_PCROP1Config(uint32_t OB_PCROP, FunctionalState NewState);
void         FLASH_OB_RDPConfig(uint8_t OB_RDP);
void         FLASH_OB_UserConfig(uint8_t OB_IWDG, uint8_t OB_STOP, uint8_t OB_STDBY);
void         FLASH_OB_BORConfig(uint8_t OB_BOR);
void         FLASH_OB_BootConfig(uint8_t OB_BOOT);
FLASH_Status FLASH_OB_Launch(void);
uint8_t      FLASH_OB_GetUser(void);
uint16_t     FLASH_OB_GetWRP(void);
uint16_t     FLASH_OB_GetWRP1(void);
uint16_t     FLASH_OB_GetPCROP(void);
uint16_t     FLASH_OB_GetPCROP1(void);
FlagStatus   FLASH_OB_GetRDP(void);
uint8_t      FLASH_OB_GetBOR(void);


void         FLASH_ITConfig(uint32_t FLASH_IT, FunctionalState NewState);
FlagStatus   FLASH_GetFlagStatus(uint32_t FLASH_FLAG);
void         FLASH_ClearFlag(uint32_t FLASH_FLAG);
FLASH_Status FLASH_GetStatus(void);
FLASH_Status FLASH_WaitForLastOperation(void);

#ifdef __cplusplus
}
#endif

#endif
