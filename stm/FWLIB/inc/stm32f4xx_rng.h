/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        stm32f4xx_rng.h
 *
 * Description:  stm32f4xx_rng.h module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#ifndef __STM32F4xx_RNG_H
#define __STM32F4xx_RNG_H

#ifdef __cplusplus
 extern "C" {
#endif


#include "stm32f4xx.h"


#define RNG_FLAG_DRDY               ((uint8_t)0x0001)
#define RNG_FLAG_CECS               ((uint8_t)0x0002)
#define RNG_FLAG_SECS               ((uint8_t)0x0004)

#define IS_RNG_GET_FLAG(RNG_FLAG) (((RNG_FLAG) == RNG_FLAG_DRDY) || \
                                   ((RNG_FLAG) == RNG_FLAG_CECS) || \
                                   ((RNG_FLAG) == RNG_FLAG_SECS))
#define IS_RNG_CLEAR_FLAG(RNG_FLAG) (((RNG_FLAG) == RNG_FLAG_CECS) || \
                                    ((RNG_FLAG) == RNG_FLAG_SECS))


#define RNG_IT_CEI                  ((uint8_t)0x20)
#define RNG_IT_SEI                  ((uint8_t)0x40)

#define IS_RNG_IT(IT) ((((IT) & (uint8_t)0x9F) == 0x00) && ((IT) != 0x00))
#define IS_RNG_GET_IT(RNG_IT) (((RNG_IT) == RNG_IT_CEI) || ((RNG_IT) == RNG_IT_SEI))


void RNG_DeInit(void);


void RNG_Cmd(FunctionalState NewState);


uint32_t RNG_GetRandomNumber(void);


void RNG_ITConfig(FunctionalState NewState);
FlagStatus RNG_GetFlagStatus(uint8_t RNG_FLAG);
void RNG_ClearFlag(uint8_t RNG_FLAG);
ITStatus RNG_GetITStatus(uint8_t RNG_IT);
void RNG_ClearITPendingBit(uint8_t RNG_IT);

#ifdef __cplusplus
}
#endif

#endif
