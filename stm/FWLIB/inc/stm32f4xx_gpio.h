/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        stm32f4xx_gpio.h
 *
 * Description:  stm32f4xx_gpio.h module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#ifndef __STM32F4xx_GPIO_H
#define __STM32F4xx_GPIO_H

#ifdef __cplusplus
 extern "C" {
#endif


#include "stm32f4xx.h"


#define IS_GPIO_ALL_PERIPH(PERIPH) (((PERIPH) == GPIOA) || \
                                    ((PERIPH) == GPIOB) || \
                                    ((PERIPH) == GPIOC) || \
                                    ((PERIPH) == GPIOD) || \
                                    ((PERIPH) == GPIOE) || \
                                    ((PERIPH) == GPIOF) || \
                                    ((PERIPH) == GPIOG) || \
                                    ((PERIPH) == GPIOH) || \
                                    ((PERIPH) == GPIOI) || \
                                    ((PERIPH) == GPIOJ) || \
                                    ((PERIPH) == GPIOK))


typedef enum
{
  GPIO_Mode_IN   = 0x00,
  GPIO_Mode_OUT  = 0x01,
  GPIO_Mode_AF   = 0x02,
  GPIO_Mode_AN   = 0x03
}GPIOMode_TypeDef;
#define IS_GPIO_MODE(MODE) (((MODE) == GPIO_Mode_IN)  || ((MODE) == GPIO_Mode_OUT) || \
                            ((MODE) == GPIO_Mode_AF)|| ((MODE) == GPIO_Mode_AN))


typedef enum
{
  GPIO_OType_PP = 0x00,
  GPIO_OType_OD = 0x01
}GPIOOType_TypeDef;
#define IS_GPIO_OTYPE(OTYPE) (((OTYPE) == GPIO_OType_PP) || ((OTYPE) == GPIO_OType_OD))


typedef enum
{
  GPIO_Low_Speed     = 0x00,
  GPIO_Medium_Speed  = 0x01,
  GPIO_Fast_Speed    = 0x02,
  GPIO_High_Speed    = 0x03
}GPIOSpeed_TypeDef;


#define  GPIO_Speed_2MHz    GPIO_Low_Speed
#define  GPIO_Speed_25MHz   GPIO_Medium_Speed
#define  GPIO_Speed_50MHz   GPIO_Fast_Speed
#define  GPIO_Speed_100MHz  GPIO_High_Speed

#define IS_GPIO_SPEED(SPEED) (((SPEED) == GPIO_Low_Speed) || ((SPEED) == GPIO_Medium_Speed) || \
                              ((SPEED) == GPIO_Fast_Speed)||  ((SPEED) == GPIO_High_Speed))


typedef enum
{
  GPIO_PuPd_NOPULL = 0x00,
  GPIO_PuPd_UP     = 0x01,
  GPIO_PuPd_DOWN   = 0x02
}GPIOPuPd_TypeDef;
#define IS_GPIO_PUPD(PUPD) (((PUPD) == GPIO_PuPd_NOPULL) || ((PUPD) == GPIO_PuPd_UP) || \
                            ((PUPD) == GPIO_PuPd_DOWN))


typedef enum
{
  Bit_RESET = 0,
  Bit_SET
}BitAction;
#define IS_GPIO_BIT_ACTION(ACTION) (((ACTION) == Bit_RESET) || ((ACTION) == Bit_SET))


typedef struct
{
  uint32_t GPIO_Pin;

  GPIOMode_TypeDef GPIO_Mode;

  GPIOSpeed_TypeDef GPIO_Speed;

  GPIOOType_TypeDef GPIO_OType;

  GPIOPuPd_TypeDef GPIO_PuPd;
}GPIO_InitTypeDef;


#define GPIO_Pin_0                 ((uint16_t)0x0001)
#define GPIO_Pin_1                 ((uint16_t)0x0002)
#define GPIO_Pin_2                 ((uint16_t)0x0004)
#define GPIO_Pin_3                 ((uint16_t)0x0008)
#define GPIO_Pin_4                 ((uint16_t)0x0010)
#define GPIO_Pin_5                 ((uint16_t)0x0020)
#define GPIO_Pin_6                 ((uint16_t)0x0040)
#define GPIO_Pin_7                 ((uint16_t)0x0080)
#define GPIO_Pin_8                 ((uint16_t)0x0100)
#define GPIO_Pin_9                 ((uint16_t)0x0200)
#define GPIO_Pin_10                ((uint16_t)0x0400)
#define GPIO_Pin_11                ((uint16_t)0x0800)
#define GPIO_Pin_12                ((uint16_t)0x1000)
#define GPIO_Pin_13                ((uint16_t)0x2000)
#define GPIO_Pin_14                ((uint16_t)0x4000)
#define GPIO_Pin_15                ((uint16_t)0x8000)
#define GPIO_Pin_All               ((uint16_t)0xFFFF)

#define GPIO_PIN_MASK              ((uint32_t)0x0000FFFF)
#define IS_GPIO_PIN(PIN)           (((PIN) & GPIO_PIN_MASK ) != (uint32_t)0x00)
#define IS_GET_GPIO_PIN(PIN) (((PIN) == GPIO_Pin_0) || \
                              ((PIN) == GPIO_Pin_1) || \
                              ((PIN) == GPIO_Pin_2) || \
                              ((PIN) == GPIO_Pin_3) || \
                              ((PIN) == GPIO_Pin_4) || \
                              ((PIN) == GPIO_Pin_5) || \
                              ((PIN) == GPIO_Pin_6) || \
                              ((PIN) == GPIO_Pin_7) || \
                              ((PIN) == GPIO_Pin_8) || \
                              ((PIN) == GPIO_Pin_9) || \
                              ((PIN) == GPIO_Pin_10) || \
                              ((PIN) == GPIO_Pin_11) || \
                              ((PIN) == GPIO_Pin_12) || \
                              ((PIN) == GPIO_Pin_13) || \
                              ((PIN) == GPIO_Pin_14) || \
                              ((PIN) == GPIO_Pin_15))


#define GPIO_PinSource0            ((uint8_t)0x00)
#define GPIO_PinSource1            ((uint8_t)0x01)
#define GPIO_PinSource2            ((uint8_t)0x02)
#define GPIO_PinSource3            ((uint8_t)0x03)
#define GPIO_PinSource4            ((uint8_t)0x04)
#define GPIO_PinSource5            ((uint8_t)0x05)
#define GPIO_PinSource6            ((uint8_t)0x06)
#define GPIO_PinSource7            ((uint8_t)0x07)
#define GPIO_PinSource8            ((uint8_t)0x08)
#define GPIO_PinSource9            ((uint8_t)0x09)
#define GPIO_PinSource10           ((uint8_t)0x0A)
#define GPIO_PinSource11           ((uint8_t)0x0B)
#define GPIO_PinSource12           ((uint8_t)0x0C)
#define GPIO_PinSource13           ((uint8_t)0x0D)
#define GPIO_PinSource14           ((uint8_t)0x0E)
#define GPIO_PinSource15           ((uint8_t)0x0F)

#define IS_GPIO_PIN_SOURCE(PINSOURCE) (((PINSOURCE) == GPIO_PinSource0) || \
                                       ((PINSOURCE) == GPIO_PinSource1) || \
                                       ((PINSOURCE) == GPIO_PinSource2) || \
                                       ((PINSOURCE) == GPIO_PinSource3) || \
                                       ((PINSOURCE) == GPIO_PinSource4) || \
                                       ((PINSOURCE) == GPIO_PinSource5) || \
                                       ((PINSOURCE) == GPIO_PinSource6) || \
                                       ((PINSOURCE) == GPIO_PinSource7) || \
                                       ((PINSOURCE) == GPIO_PinSource8) || \
                                       ((PINSOURCE) == GPIO_PinSource9) || \
                                       ((PINSOURCE) == GPIO_PinSource10) || \
                                       ((PINSOURCE) == GPIO_PinSource11) || \
                                       ((PINSOURCE) == GPIO_PinSource12) || \
                                       ((PINSOURCE) == GPIO_PinSource13) || \
                                       ((PINSOURCE) == GPIO_PinSource14) || \
                                       ((PINSOURCE) == GPIO_PinSource15))


#define GPIO_AF_RTC_50Hz      ((uint8_t)0x00)
#define GPIO_AF_MCO           ((uint8_t)0x00)
#define GPIO_AF_TAMPER        ((uint8_t)0x00)
#define GPIO_AF_SWJ           ((uint8_t)0x00)
#define GPIO_AF_TRACE         ((uint8_t)0x00)


#define GPIO_AF_TIM1          ((uint8_t)0x01)
#define GPIO_AF_TIM2          ((uint8_t)0x01)


#define GPIO_AF_TIM3          ((uint8_t)0x02)
#define GPIO_AF_TIM4          ((uint8_t)0x02)
#define GPIO_AF_TIM5          ((uint8_t)0x02)


#define GPIO_AF_TIM8          ((uint8_t)0x03)
#define GPIO_AF_TIM9          ((uint8_t)0x03)
#define GPIO_AF_TIM10         ((uint8_t)0x03)
#define GPIO_AF_TIM11         ((uint8_t)0x03)


#define GPIO_AF_I2C1          ((uint8_t)0x04)
#define GPIO_AF_I2C2          ((uint8_t)0x04)
#define GPIO_AF_I2C3          ((uint8_t)0x04)


#define GPIO_AF_SPI1          ((uint8_t)0x05)
#define GPIO_AF_SPI2          ((uint8_t)0x05)
#define GPIO_AF5_SPI3         ((uint8_t)0x05)
#define GPIO_AF_SPI4          ((uint8_t)0x05)
#define GPIO_AF_SPI5          ((uint8_t)0x05)
#define GPIO_AF_SPI6          ((uint8_t)0x05)


#define GPIO_AF_SPI3          ((uint8_t)0x06)
#define GPIO_AF6_SPI2         ((uint8_t)0x06)
#define GPIO_AF6_SPI4         ((uint8_t)0x06)
#define GPIO_AF6_SPI5         ((uint8_t)0x06)
#define GPIO_AF_SAI1          ((uint8_t)0x06)


#define GPIO_AF_USART1         ((uint8_t)0x07)
#define GPIO_AF_USART2         ((uint8_t)0x07)
#define GPIO_AF_USART3         ((uint8_t)0x07)
#define GPIO_AF7_SPI3          ((uint8_t)0x07)


#define GPIO_AF_I2S3ext   GPIO_AF7_SPI3


#define GPIO_AF_UART4         ((uint8_t)0x08)
#define GPIO_AF_UART5         ((uint8_t)0x08)
#define GPIO_AF_USART6        ((uint8_t)0x08)
#define GPIO_AF_UART7         ((uint8_t)0x08)
#define GPIO_AF_UART8         ((uint8_t)0x08)


#define GPIO_AF_CAN1          ((uint8_t)0x09)
#define GPIO_AF_CAN2          ((uint8_t)0x09)
#define GPIO_AF_TIM12         ((uint8_t)0x09)
#define GPIO_AF_TIM13         ((uint8_t)0x09)
#define GPIO_AF_TIM14         ((uint8_t)0x09)

#define GPIO_AF9_I2C2         ((uint8_t)0x09)
#define GPIO_AF9_I2C3         ((uint8_t)0x09)


#define GPIO_AF_OTG_FS         ((uint8_t)0xA)
#define GPIO_AF_OTG_HS         ((uint8_t)0xA)


#define GPIO_AF_ETH             ((uint8_t)0x0B)


#if defined (STM32F40_41xxx)
#define GPIO_AF_FSMC             ((uint8_t)0xC)
#endif

#if defined (STM32F427_437xx) || defined (STM32F429_439xx)
#define GPIO_AF_FMC              ((uint8_t)0xC)
#endif

#define GPIO_AF_OTG_HS_FS        ((uint8_t)0xC)
#define GPIO_AF_SDIO             ((uint8_t)0xC)


#define GPIO_AF_DCMI          ((uint8_t)0x0D)


#define GPIO_AF_LTDC          ((uint8_t)0x0E)


#define GPIO_AF_EVENTOUT      ((uint8_t)0x0F)

#if defined (STM32F40_41xxx)
#define IS_GPIO_AF(AF)   (((AF) == GPIO_AF_RTC_50Hz)  || ((AF) == GPIO_AF_TIM14)     || \
                          ((AF) == GPIO_AF_MCO)       || ((AF) == GPIO_AF_TAMPER)    || \
                          ((AF) == GPIO_AF_SWJ)       || ((AF) == GPIO_AF_TRACE)     || \
                          ((AF) == GPIO_AF_TIM1)      || ((AF) == GPIO_AF_TIM2)      || \
                          ((AF) == GPIO_AF_TIM3)      || ((AF) == GPIO_AF_TIM4)      || \
                          ((AF) == GPIO_AF_TIM5)      || ((AF) == GPIO_AF_TIM8)      || \
                          ((AF) == GPIO_AF_I2C1)      || ((AF) == GPIO_AF_I2C2)      || \
                          ((AF) == GPIO_AF_I2C3)      || ((AF) == GPIO_AF_SPI1)      || \
                          ((AF) == GPIO_AF_SPI2)      || ((AF) == GPIO_AF_TIM13)     || \
                          ((AF) == GPIO_AF_SPI3)      || ((AF) == GPIO_AF_TIM14)     || \
                          ((AF) == GPIO_AF_USART1)    || ((AF) == GPIO_AF_USART2)    || \
                          ((AF) == GPIO_AF_USART3)    || ((AF) == GPIO_AF_UART4)     || \
                          ((AF) == GPIO_AF_UART5)     || ((AF) == GPIO_AF_USART6)    || \
                          ((AF) == GPIO_AF_CAN1)      || ((AF) == GPIO_AF_CAN2)      || \
                          ((AF) == GPIO_AF_OTG_FS)    || ((AF) == GPIO_AF_OTG_HS)    || \
                          ((AF) == GPIO_AF_ETH)       || ((AF) == GPIO_AF_OTG_HS_FS) || \
                          ((AF) == GPIO_AF_SDIO)      || ((AF) == GPIO_AF_DCMI)      || \
                          ((AF) == GPIO_AF_EVENTOUT)  || ((AF) == GPIO_AF_FSMC))
#endif

#if defined (STM32F401xx)
#define IS_GPIO_AF(AF)   (((AF) == GPIO_AF_RTC_50Hz)  || ((AF) == GPIO_AF_TIM14)     || \
                          ((AF) == GPIO_AF_MCO)       || ((AF) == GPIO_AF_TAMPER)    || \
                          ((AF) == GPIO_AF_SWJ)       || ((AF) == GPIO_AF_TRACE)     || \
                          ((AF) == GPIO_AF_TIM1)      || ((AF) == GPIO_AF_TIM2)      || \
                          ((AF) == GPIO_AF_TIM3)      || ((AF) == GPIO_AF_TIM4)      || \
                          ((AF) == GPIO_AF_TIM5)      || ((AF) == GPIO_AF_TIM8)      || \
                          ((AF) == GPIO_AF_I2C1)      || ((AF) == GPIO_AF_I2C2)      || \
                          ((AF) == GPIO_AF_I2C3)      || ((AF) == GPIO_AF_SPI1)      || \
                          ((AF) == GPIO_AF_SPI2)      || ((AF) == GPIO_AF_TIM13)     || \
                          ((AF) == GPIO_AF_SPI3)      || ((AF) == GPIO_AF_TIM14)     || \
                          ((AF) == GPIO_AF_USART1)    || ((AF) == GPIO_AF_USART2)    || \
                          ((AF) == GPIO_AF_SDIO)      || ((AF) == GPIO_AF_USART6)    || \
                          ((AF) == GPIO_AF_OTG_FS)    || ((AF) == GPIO_AF_OTG_HS)    || \
                          ((AF) == GPIO_AF_EVENTOUT)  || ((AF) == GPIO_AF_SPI4))
#endif

#if defined (STM32F411xE)
#define IS_GPIO_AF(AF)   (((AF) < 16) && ((AF) != 11) && ((AF) != 13) && ((AF) != 14))
#endif

#if defined (STM32F427_437xx) || defined (STM32F429_439xx)
#define IS_GPIO_AF(AF)   (((AF) == GPIO_AF_RTC_50Hz)  || ((AF) == GPIO_AF_TIM14)     || \
                          ((AF) == GPIO_AF_MCO)       || ((AF) == GPIO_AF_TAMPER)    || \
                          ((AF) == GPIO_AF_SWJ)       || ((AF) == GPIO_AF_TRACE)     || \
                          ((AF) == GPIO_AF_TIM1)      || ((AF) == GPIO_AF_TIM2)      || \
                          ((AF) == GPIO_AF_TIM3)      || ((AF) == GPIO_AF_TIM4)      || \
                          ((AF) == GPIO_AF_TIM5)      || ((AF) == GPIO_AF_TIM8)      || \
                          ((AF) == GPIO_AF_I2C1)      || ((AF) == GPIO_AF_I2C2)      || \
                          ((AF) == GPIO_AF_I2C3)      || ((AF) == GPIO_AF_SPI1)      || \
                          ((AF) == GPIO_AF_SPI2)      || ((AF) == GPIO_AF_TIM13)     || \
                          ((AF) == GPIO_AF_SPI3)      || ((AF) == GPIO_AF_TIM14)     || \
                          ((AF) == GPIO_AF_USART1)    || ((AF) == GPIO_AF_USART2)    || \
                          ((AF) == GPIO_AF_USART3)    || ((AF) == GPIO_AF_UART4)     || \
                          ((AF) == GPIO_AF_UART5)     || ((AF) == GPIO_AF_USART6)    || \
                          ((AF) == GPIO_AF_CAN1)      || ((AF) == GPIO_AF_CAN2)      || \
                          ((AF) == GPIO_AF_OTG_FS)    || ((AF) == GPIO_AF_OTG_HS)    || \
                          ((AF) == GPIO_AF_ETH)       || ((AF) == GPIO_AF_OTG_HS_FS) || \
                          ((AF) == GPIO_AF_SDIO)      || ((AF) == GPIO_AF_DCMI)      || \
                          ((AF) == GPIO_AF_EVENTOUT)  || ((AF) == GPIO_AF_SPI4)      || \
                          ((AF) == GPIO_AF_SPI5)      || ((AF) == GPIO_AF_SPI6)      || \
                          ((AF) == GPIO_AF_UART7)     || ((AF) == GPIO_AF_UART8)     || \
                          ((AF) == GPIO_AF_FMC)       ||  ((AF) == GPIO_AF_SAI1)     || \
                          ((AF) == GPIO_AF_LTDC))
#endif


#define GPIO_Mode_AIN           GPIO_Mode_AN

#define GPIO_AF_OTG1_FS         GPIO_AF_OTG_FS
#define GPIO_AF_OTG2_HS         GPIO_AF_OTG_HS
#define GPIO_AF_OTG2_FS         GPIO_AF_OTG_HS_FS


void GPIO_DeInit(GPIO_TypeDef* GPIOx);


void GPIO_Init(GPIO_TypeDef* GPIOx, GPIO_InitTypeDef* GPIO_InitStruct);
void GPIO_StructInit(GPIO_InitTypeDef* GPIO_InitStruct);
void GPIO_PinLockConfig(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);


uint8_t GPIO_ReadInputDataBit(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
uint16_t GPIO_ReadInputData(GPIO_TypeDef* GPIOx);
uint8_t GPIO_ReadOutputDataBit(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
uint16_t GPIO_ReadOutputData(GPIO_TypeDef* GPIOx);
void GPIO_SetBits(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
void GPIO_ResetBits(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
void GPIO_WriteBit(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, BitAction BitVal);
void GPIO_Write(GPIO_TypeDef* GPIOx, uint16_t PortVal);
void GPIO_ToggleBits(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);


void GPIO_PinAFConfig(GPIO_TypeDef* GPIOx, uint16_t GPIO_PinSource, uint8_t GPIO_AF);

#ifdef __cplusplus
}
#endif

#endif
