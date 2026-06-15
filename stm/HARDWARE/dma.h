/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        dma.h
 *
 * Description:  dma.h module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#ifndef __DMA_H
#define __DMA_H

#include "stm32f10x_dma.h"
#include "sys.h"
#include "system.h"

void MYDMA_Init2(DMA_Channel_TypeDef*DMA_CHx,u32 cpar,u32 cmar,u16 cndtr);
void MYDMA_Enable2(DMA_Channel_TypeDef*DMA_CHx);

void DMA_printf(const char *format,...);
u32 USART_SendBuffer(const char* buffer, u32 length);

#endif
