/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        key.h
 *
 * Description:  key.h module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#ifndef __KEY_H
#define __KEY_H
#include "sys.h"
#include "system.h"

void KEY_Init(void);
u8 click(void);
void Delay_ms(void);
u8 click_N_Double (u8 time);
u8 click_N_Double_MPU6050 (u8 time);
u8 Long_Press(void);

#define KEY_PORT	GPIOC
#define KEY_PIN		GPIO_Pin_5
#define KEY			PCin(5)

u8 KEY_Scan(u16 Frequency,u16 filter_times);
enum {
	key_stateless,
	single_click,
	double_click,
	long_click
};
#endif
