/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        timer.h
 *
 * Description:  timer.h module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#ifndef __TIMER_H
#define __TIMER_H
#include "system.h"
void TIM1_Cap_Init(u16 arr, u16 psc);
extern int L_Remoter_Ch1,L_Remoter_Ch2,L_Remoter_Ch3,L_Remoter_Ch4;
extern int Remoter_Ch1,Remoter_Ch2,Remoter_Ch3,Remoter_Ch4;

#endif
