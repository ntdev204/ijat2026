/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        filter.h
 *
 * Description:  filter.h module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#ifndef __FILTER_H
#define __FILTER_H
#include "system.h"

extern float angle, angle_dot;
void Kalman_Filter(float Accel,float Gyro);
void Yijielvbo(float angle_m, float gyro_m);
#endif
