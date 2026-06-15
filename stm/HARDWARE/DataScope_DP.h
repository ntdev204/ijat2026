/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        DataScope_DP.h
 *
 * Description:  DataScope_DP.h module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#ifndef __DATA_PRTOCOL_H
#define __DATA_PRTOCOL_H
#include "system.h"

extern unsigned char DataScope_OutPut_Buffer[42];

void DataScope_Get_Channel_Data(float Data,unsigned char Channel);

unsigned char DataScope_Data_Generate(unsigned char Channel_Number);

#endif
