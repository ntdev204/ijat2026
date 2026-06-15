/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        port.c
 *
 * Description:  port.c module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#include <stdlib.h>


#include "FreeRTOS.h"
#include "task.h"


#define portINITIAL_SPSR				( ( StackType_t ) 0x1f )
#define portTHUMB_MODE_BIT				( ( StackType_t ) 0x20 )
#define portINSTRUCTION_SIZE			( ( StackType_t ) 4 )
#define portNO_CRITICAL_SECTION_NESTING	( ( StackType_t ) 0 )


#define portENABLE_TIMER			( ( uint8_t ) 0x01 )
#define portPRESCALE_VALUE			0x00
#define portINTERRUPT_ON_MATCH		( ( uint32_t ) 0x01 )
#define portRESET_COUNT_ON_MATCH	( ( uint32_t ) 0x02 )


#define portTIMER_VIC_CHANNEL		( ( uint32_t ) 0x0004 )
#define portTIMER_VIC_CHANNEL_BIT	( ( uint32_t ) 0x0010 )
#define portTIMER_VIC_ENABLE		( ( uint32_t ) 0x0020 )


#define portTIMER_MATCH_ISR_BIT		( ( uint8_t ) 0x01 )
#define portCLEAR_VIC_INTERRUPT		( ( uint32_t ) 0 )


#define portNO_CRITICAL_NESTING		( ( uint32_t ) 0 )
volatile uint32_t ulCriticalNesting = 9999UL;


static void prvSetupTimerInterrupt( void );


extern __asm void vPortStartFirstTask( void );


StackType_t *pxPortInitialiseStack( StackType_t *pxTopOfStack, TaskFunction_t pxCode, void *pvParameters )
{
StackType_t *pxOriginalTOS;


	pxOriginalTOS = pxTopOfStack;


	pxTopOfStack--;


	*pxTopOfStack = ( StackType_t ) pxCode + portINSTRUCTION_SIZE;
	pxTopOfStack--;

	*pxTopOfStack = ( StackType_t ) 0xaaaaaaaa;
	pxTopOfStack--;
	*pxTopOfStack = ( StackType_t ) pxOriginalTOS;
	pxTopOfStack--;
	*pxTopOfStack = ( StackType_t ) 0x12121212;
	pxTopOfStack--;
	*pxTopOfStack = ( StackType_t ) 0x11111111;
	pxTopOfStack--;
	*pxTopOfStack = ( StackType_t ) 0x10101010;
	pxTopOfStack--;
	*pxTopOfStack = ( StackType_t ) 0x09090909;
	pxTopOfStack--;
	*pxTopOfStack = ( StackType_t ) 0x08080808;
	pxTopOfStack--;
	*pxTopOfStack = ( StackType_t ) 0x07070707;
	pxTopOfStack--;
	*pxTopOfStack = ( StackType_t ) 0x06060606;
	pxTopOfStack--;
	*pxTopOfStack = ( StackType_t ) 0x05050505;
	pxTopOfStack--;
	*pxTopOfStack = ( StackType_t ) 0x04040404;
	pxTopOfStack--;
	*pxTopOfStack = ( StackType_t ) 0x03030303;
	pxTopOfStack--;
	*pxTopOfStack = ( StackType_t ) 0x02020202;
	pxTopOfStack--;
	*pxTopOfStack = ( StackType_t ) 0x01010101;
	pxTopOfStack--;
	*pxTopOfStack = ( StackType_t ) pvParameters;
	pxTopOfStack--;


	*pxTopOfStack = ( StackType_t ) portINITIAL_SPSR;

	if( ( ( uint32_t ) pxCode & 0x01UL ) != 0x00UL )
	{

		*pxTopOfStack |= portTHUMB_MODE_BIT;
	}

	pxTopOfStack--;


	*pxTopOfStack = portNO_CRITICAL_SECTION_NESTING;

	return pxTopOfStack;
}


BaseType_t xPortStartScheduler( void )
{

	prvSetupTimerInterrupt();


	vPortStartFirstTask();


	return 0;
}


void vPortEndScheduler( void )
{

}


#if configUSE_PREEMPTION == 0


	void vNonPreemptiveTick( void ) __irq;
	void vNonPreemptiveTick( void ) __irq
	{

		xTaskIncrementTick();

		T0IR = portTIMER_MATCH_ISR_BIT;
		VICVectAddr = portCLEAR_VIC_INTERRUPT;
	}

 #else


	  void vPreemptiveTick( void );

#endif


static void prvSetupTimerInterrupt( void )
{
uint32_t ulCompareMatch;


	T0PR = portPRESCALE_VALUE;


	ulCompareMatch = configCPU_CLOCK_HZ / configTICK_RATE_HZ;


	#if portPRESCALE_VALUE != 0
	{
		ulCompareMatch /= ( portPRESCALE_VALUE + 1 );
	}
	#endif

	T0MR0 = ulCompareMatch;


	T0MCR = portRESET_COUNT_ON_MATCH | portINTERRUPT_ON_MATCH;


	VICIntSelect &= ~( portTIMER_VIC_CHANNEL_BIT );
	VICIntEnable |= portTIMER_VIC_CHANNEL_BIT;


	#if configUSE_PREEMPTION == 1
	{
		VICVectAddr0 = ( uint32_t ) vPreemptiveTick;
	}
	#else
	{
		VICVectAddr0 = ( uint32_t ) vNonPreemptiveTick;
	}
	#endif

	VICVectCntl0 = portTIMER_VIC_CHANNEL | portTIMER_VIC_ENABLE;


	T0TCR = portENABLE_TIMER;
}


void vPortEnterCritical( void )
{

	__disable_irq();


	ulCriticalNesting++;
}


void vPortExitCritical( void )
{
	if( ulCriticalNesting > portNO_CRITICAL_NESTING )
	{

		ulCriticalNesting--;


		if( ulCriticalNesting == portNO_CRITICAL_NESTING )
		{

			__enable_irq();
		}
	}
}
