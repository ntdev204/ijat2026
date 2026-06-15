/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        sys.c
 *
 * Description:  sys.c module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#include "sys.h"

__asm void WFI_SET(void)
{
	WFI;
}

__asm void INTX_DISABLE(void)
{
	CPSID   I
	BX      LR
}

__asm void INTX_ENABLE(void)
{
	CPSIE   I
	BX      LR
}

__asm void MSR_MSP(u32 addr)
{
	MSR MSP, r0
	BX r14
}
