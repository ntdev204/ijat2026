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

#include "FreeRTOS.h"
#include "task.h"


#define portNVIC_SYSTICK_CTRL		( ( volatile uint32_t *) 0xe000e010 )
#define portNVIC_SYSTICK_LOAD		( ( volatile uint32_t *) 0xe000e014 )
#define portNVIC_INT_CTRL			( ( volatile uint32_t *) 0xe000ed04 )
#define portNVIC_SYSPRI2			( ( volatile uint32_t *) 0xe000ed20 )
#define portNVIC_SYSTICK_CLK		0x00000004
#define portNVIC_SYSTICK_INT		0x00000002
#define portNVIC_SYSTICK_ENABLE		0x00000001
#define portNVIC_PENDSVSET			0x10000000
#define portMIN_INTERRUPT_PRIORITY	( 255UL )
#define portNVIC_PENDSV_PRI			( portMIN_INTERRUPT_PRIORITY << 16UL )
#define portNVIC_SYSTICK_PRI		( portMIN_INTERRUPT_PRIORITY << 24UL )


#define portINITIAL_XPSR			( 0x01000000 )


#define portSY_FULL_READ_WRITE		( 15 )


static UBaseType_t uxCriticalNesting = 0xaaaaaaaa;


static void prvSetupTimerInterrupt( void );


void xPortPendSVHandler( void );
void xPortSysTickHandler( void );
void vPortSVCHandler( void );


static void prvPortStartFirstTask( void );


static void prvTaskExitError( void );


StackType_t *pxPortInitialiseStack( StackType_t *pxTopOfStack, TaskFunction_t pxCode, void *pvParameters )
{

	pxTopOfStack--;
	*pxTopOfStack = portINITIAL_XPSR;
	pxTopOfStack--;
	*pxTopOfStack = ( StackType_t ) pxCode;
	pxTopOfStack--;
	*pxTopOfStack = ( StackType_t ) prvTaskExitError;
	pxTopOfStack -= 5;
	*pxTopOfStack = ( StackType_t ) pvParameters;
	pxTopOfStack -= 8;

	return pxTopOfStack;
}


static void prvTaskExitError( void )
{

	configASSERT( uxCriticalNesting == ~0UL );
	portDISABLE_INTERRUPTS();
	for( ;; );
}


void vPortSVCHandler( void )
{

}


__asm void prvPortStartFirstTask( void )
{
	extern pxCurrentTCB;

	PRESERVE8


	ldr	r3, =pxCurrentTCB
	ldr r1, [r3]
	ldr r0, [r1]
	adds r0, #32
	msr psp, r0
	movs r0, #2
	msr CONTROL, r0
	isb
	pop {r0-r5}
	mov lr, r5
	pop {r3}
	pop {r2}
	cpsie i
	bx r3

	ALIGN
}


BaseType_t xPortStartScheduler( void )
{

	*(portNVIC_SYSPRI2) |= portNVIC_PENDSV_PRI;
	*(portNVIC_SYSPRI2) |= portNVIC_SYSTICK_PRI;


	prvSetupTimerInterrupt();


	uxCriticalNesting = 0;


	prvPortStartFirstTask();


	return 0;
}


void vPortEndScheduler( void )
{

	configASSERT( uxCriticalNesting == 1000UL );
}


void vPortYield( void )
{

	*( portNVIC_INT_CTRL ) = portNVIC_PENDSVSET;


	__dsb( portSY_FULL_READ_WRITE );
	__isb( portSY_FULL_READ_WRITE );
}


void vPortEnterCritical( void )
{
    portDISABLE_INTERRUPTS();
    uxCriticalNesting++;
	__dsb( portSY_FULL_READ_WRITE );
	__isb( portSY_FULL_READ_WRITE );
}


void vPortExitCritical( void )
{
	configASSERT( uxCriticalNesting );
    uxCriticalNesting--;
    if( uxCriticalNesting == 0 )
    {
        portENABLE_INTERRUPTS();
    }
}


__asm uint32_t ulSetInterruptMaskFromISR( void )
{
	mrs r0, PRIMASK
	cpsid i
	bx lr
}


__asm void vClearInterruptMaskFromISR( uint32_t ulMask )
{
	msr PRIMASK, r0
	bx lr
}


__asm void xPortPendSVHandler( void )
{
	extern vTaskSwitchContext
	extern pxCurrentTCB

	PRESERVE8

	mrs r0, psp

	ldr	r3, =pxCurrentTCB
	ldr	r2, [r3]

	subs r0, #32
	str r0, [r2]
	stmia r0!, {r4-r7}
	mov r4, r8
	mov r5, r9
	mov r6, r10
	mov r7, r11
	stmia r0!, {r4-r7}

	push {r3, r14}
	cpsid i
	bl vTaskSwitchContext
	cpsie i
	pop {r2, r3}

	ldr r1, [r2]
	ldr r0, [r1]
	adds r0, #16
	ldmia r0!, {r4-r7}
	mov r8, r4
	mov r9, r5
	mov r10, r6
	mov r11, r7

	msr psp, r0

	subs r0, #32
	ldmia r0!, {r4-r7}

	bx r3
	ALIGN
}


void xPortSysTickHandler( void )
{
uint32_t ulPreviousMask;

	ulPreviousMask = portSET_INTERRUPT_MASK_FROM_ISR();
	{

		if( xTaskIncrementTick() != pdFALSE )
		{

			*(portNVIC_INT_CTRL) = portNVIC_PENDSVSET;
		}
	}
	portCLEAR_INTERRUPT_MASK_FROM_ISR( ulPreviousMask );
}


void prvSetupTimerInterrupt( void )
{

	*(portNVIC_SYSTICK_LOAD) = ( configCPU_CLOCK_HZ / configTICK_RATE_HZ ) - 1UL;
	*(portNVIC_SYSTICK_CTRL) = portNVIC_SYSTICK_CLK | portNVIC_SYSTICK_INT | portNVIC_SYSTICK_ENABLE;
}
