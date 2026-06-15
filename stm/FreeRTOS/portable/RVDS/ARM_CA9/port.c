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

#ifndef configINTERRUPT_CONTROLLER_BASE_ADDRESS
	#error configINTERRUPT_CONTROLLER_BASE_ADDRESS must be defined.  See http:
#endif

#ifndef configINTERRUPT_CONTROLLER_CPU_INTERFACE_OFFSET
	#error configINTERRUPT_CONTROLLER_CPU_INTERFACE_OFFSET must be defined.  See http:
#endif

#ifndef configUNIQUE_INTERRUPT_PRIORITIES
	#error configUNIQUE_INTERRUPT_PRIORITIES must be defined.  See http:
#endif

#ifndef configSETUP_TICK_INTERRUPT
	#error configSETUP_TICK_INTERRUPT() must be defined.  See http:
#endif

#ifndef configMAX_API_CALL_INTERRUPT_PRIORITY
	#error configMAX_API_CALL_INTERRUPT_PRIORITY must be defined.  See http:
#endif

#if configMAX_API_CALL_INTERRUPT_PRIORITY == 0
	#error configMAX_API_CALL_INTERRUPT_PRIORITY must not be set to 0
#endif

#if configMAX_API_CALL_INTERRUPT_PRIORITY > configUNIQUE_INTERRUPT_PRIORITIES
	#error configMAX_API_CALL_INTERRUPT_PRIORITY must be less than or equal to configUNIQUE_INTERRUPT_PRIORITIES as the lower the numeric priority value the higher the logical interrupt priority
#endif

#if configUSE_PORT_OPTIMISED_TASK_SELECTION == 1

	#if( configMAX_PRIORITIES > 32 )
		#error configUSE_PORT_OPTIMISED_TASK_SELECTION can only be set to 1 when configMAX_PRIORITIES is less than or equal to 32.  It is very rare that a system requires more than 10 to 15 difference priorities as tasks that share a priority will time slice.
	#endif
#endif


#if configMAX_API_CALL_INTERRUPT_PRIORITY <= ( configUNIQUE_INTERRUPT_PRIORITIES / 2 )
	#error configMAX_API_CALL_INTERRUPT_PRIORITY must be greater than ( configUNIQUE_INTERRUPT_PRIORITIES / 2 )
#endif

#ifndef configCLEAR_TICK_INTERRUPT
	#define configCLEAR_TICK_INTERRUPT()
#endif


#if configUNIQUE_INTERRUPT_PRIORITIES == 16
	#define portPRIORITY_SHIFT 4
	#define portMAX_BINARY_POINT_VALUE	3
#elif configUNIQUE_INTERRUPT_PRIORITIES == 32
	#define portPRIORITY_SHIFT 3
	#define portMAX_BINARY_POINT_VALUE	2
#elif configUNIQUE_INTERRUPT_PRIORITIES == 64
	#define portPRIORITY_SHIFT 2
	#define portMAX_BINARY_POINT_VALUE	1
#elif configUNIQUE_INTERRUPT_PRIORITIES == 128
	#define portPRIORITY_SHIFT 1
	#define portMAX_BINARY_POINT_VALUE	0
#elif configUNIQUE_INTERRUPT_PRIORITIES == 256
	#define portPRIORITY_SHIFT 0
	#define portMAX_BINARY_POINT_VALUE	0
#else
	#error Invalid configUNIQUE_INTERRUPT_PRIORITIES setting.  configUNIQUE_INTERRUPT_PRIORITIES must be set to the number of unique priorities implemented by the target hardware
#endif


#define portNO_CRITICAL_NESTING			( ( uint32_t ) 0 )


#define portUNMASK_VALUE				( 0xFFUL )


#define portNO_FLOATING_POINT_CONTEXT	( ( StackType_t ) 0 )


#define portICCPMR_PRIORITY_MASK_OFFSET  		( 0x04 )
#define portICCIAR_INTERRUPT_ACKNOWLEDGE_OFFSET ( 0x0C )
#define portICCEOIR_END_OF_INTERRUPT_OFFSET 	( 0x10 )
#define portICCBPR_BINARY_POINT_OFFSET			( 0x08 )
#define portICCRPR_RUNNING_PRIORITY_OFFSET		( 0x14 )
#define portINTERRUPT_CONTROLLER_CPU_INTERFACE_ADDRESS 		( configINTERRUPT_CONTROLLER_BASE_ADDRESS + configINTERRUPT_CONTROLLER_CPU_INTERFACE_OFFSET )
#define portICCPMR_PRIORITY_MASK_REGISTER 					( *( ( volatile uint32_t * ) ( portINTERRUPT_CONTROLLER_CPU_INTERFACE_ADDRESS + portICCPMR_PRIORITY_MASK_OFFSET ) ) )
#define portICCIAR_INTERRUPT_ACKNOWLEDGE_REGISTER_ADDRESS 	( portINTERRUPT_CONTROLLER_CPU_INTERFACE_ADDRESS + portICCIAR_INTERRUPT_ACKNOWLEDGE_OFFSET )
#define portICCEOIR_END_OF_INTERRUPT_REGISTER_ADDRESS 		( portINTERRUPT_CONTROLLER_CPU_INTERFACE_ADDRESS + portICCEOIR_END_OF_INTERRUPT_OFFSET )
#define portICCPMR_PRIORITY_MASK_REGISTER_ADDRESS 			( portINTERRUPT_CONTROLLER_CPU_INTERFACE_ADDRESS + portICCPMR_PRIORITY_MASK_OFFSET )
#define portICCBPR_BINARY_POINT_REGISTER 					( *( ( const volatile uint32_t * ) ( portINTERRUPT_CONTROLLER_CPU_INTERFACE_ADDRESS + portICCBPR_BINARY_POINT_OFFSET ) ) )
#define portICCRPR_RUNNING_PRIORITY_REGISTER 				( *( ( const volatile uint32_t * ) ( portINTERRUPT_CONTROLLER_CPU_INTERFACE_ADDRESS + portICCRPR_RUNNING_PRIORITY_OFFSET ) ) )


#define portBINARY_POINT_BITS			( ( uint8_t ) 0x03 )


#define portINITIAL_SPSR				( ( StackType_t ) 0x1f )
#define portTHUMB_MODE_BIT				( ( StackType_t ) 0x20 )
#define portTHUMB_MODE_ADDRESS			( 0x01UL )


#define portAPSR_MODE_BITS_MASK			( 0x1F )


#define portAPSR_USER_MODE				( 0x10 )


#define portCLEAR_INTERRUPT_MASK()											\
{																			\
	__disable_irq();														\
	portICCPMR_PRIORITY_MASK_REGISTER = portUNMASK_VALUE;					\
	__asm(	"DSB		\n"													\
			"ISB		\n" );												\
	__enable_irq();															\
}


extern void vPortRestoreTaskContext( void );


static void prvTaskExitError( void );


volatile uint32_t ulCriticalNesting = 9999UL;


uint32_t ulICCIAR __attribute__( ( at( portICCIAR_INTERRUPT_ACKNOWLEDGE_REGISTER_ADDRESS ) ) );
uint32_t ulICCEOIR __attribute__( ( at( portICCEOIR_END_OF_INTERRUPT_REGISTER_ADDRESS ) ) );
uint32_t ulICCPMR __attribute__( ( at( portICCPMR_PRIORITY_MASK_REGISTER_ADDRESS ) ) );
uint32_t ulAsmAPIPriorityMask __attribute__( ( at( configMAX_API_CALL_INTERRUPT_PRIORITY << portPRIORITY_SHIFT ) ) );


uint32_t ulPortTaskHasFPUContext = pdFALSE;


uint32_t ulPortYieldRequired = pdFALSE;


uint32_t ulPortInterruptNesting = 0UL;


StackType_t *pxPortInitialiseStack( StackType_t *pxTopOfStack, TaskFunction_t pxCode, void *pvParameters )
{

	*pxTopOfStack = NULL;
	pxTopOfStack--;
	*pxTopOfStack = NULL;
	pxTopOfStack--;
	*pxTopOfStack = NULL;
	pxTopOfStack--;
	*pxTopOfStack = ( StackType_t ) portINITIAL_SPSR;

	if( ( ( uint32_t ) pxCode & portTHUMB_MODE_ADDRESS ) != 0x00UL )
	{

		*pxTopOfStack |= portTHUMB_MODE_BIT;
	}

	pxTopOfStack--;


	*pxTopOfStack = ( StackType_t ) pxCode;
	pxTopOfStack--;


	*pxTopOfStack = ( StackType_t ) prvTaskExitError;
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


	*pxTopOfStack = portNO_CRITICAL_NESTING;
	pxTopOfStack--;


	*pxTopOfStack = portNO_FLOATING_POINT_CONTEXT;

	return pxTopOfStack;
}


static void prvTaskExitError( void )
{

	configASSERT( ulPortInterruptNesting == ~0UL );
	portDISABLE_INTERRUPTS();
	for( ;; );
}


BaseType_t xPortStartScheduler( void )
{
uint32_t ulAPSR;


	__asm( "MRS ulAPSR, APSR" );
	ulAPSR &= portAPSR_MODE_BITS_MASK;
	configASSERT( ulAPSR != portAPSR_USER_MODE );

	if( ulAPSR != portAPSR_USER_MODE )
	{

		configASSERT( ( portICCBPR_BINARY_POINT_REGISTER & portBINARY_POINT_BITS ) <= portMAX_BINARY_POINT_VALUE );

		if( ( portICCBPR_BINARY_POINT_REGISTER & portBINARY_POINT_BITS ) <= portMAX_BINARY_POINT_VALUE )
		{

			configSETUP_TICK_INTERRUPT();

			__enable_irq();
			vPortRestoreTaskContext();
		}
	}


	return 0;
}


void vPortEndScheduler( void )
{

	configASSERT( ulCriticalNesting == 1000UL );
}


void vPortEnterCritical( void )
{

	ulPortSetInterruptMask();


	ulCriticalNesting++;


	if( ulCriticalNesting == 1 )
	{
		configASSERT( ulPortInterruptNesting == 0 );
	}
}


void vPortExitCritical( void )
{
	if( ulCriticalNesting > portNO_CRITICAL_NESTING )
	{

		ulCriticalNesting--;


		if( ulCriticalNesting == portNO_CRITICAL_NESTING )
		{

			portCLEAR_INTERRUPT_MASK();
		}
	}
}


void FreeRTOS_Tick_Handler( void )
{

	__disable_irq();
	portICCPMR_PRIORITY_MASK_REGISTER = ( uint32_t ) ( configMAX_API_CALL_INTERRUPT_PRIORITY << portPRIORITY_SHIFT );
	__asm(	"DSB		\n"
			"ISB		\n" );
	__enable_irq();


	if( xTaskIncrementTick() != pdFALSE )
	{
		ulPortYieldRequired = pdTRUE;
	}


	portCLEAR_INTERRUPT_MASK();
	configCLEAR_TICK_INTERRUPT();
}


void vPortTaskUsesFPU( void )
{
uint32_t ulInitialFPSCR = 0;


	ulPortTaskHasFPUContext = pdTRUE;


	__asm( "FMXR 	FPSCR, ulInitialFPSCR" );
}


void vPortClearInterruptMask( uint32_t ulNewMaskValue )
{
	if( ulNewMaskValue == pdFALSE )
	{
		portCLEAR_INTERRUPT_MASK();
	}
}


uint32_t ulPortSetInterruptMask( void )
{
uint32_t ulReturn;

	__disable_irq();
	if( portICCPMR_PRIORITY_MASK_REGISTER == ( uint32_t ) ( configMAX_API_CALL_INTERRUPT_PRIORITY << portPRIORITY_SHIFT ) )
	{

		ulReturn = pdTRUE;
	}
	else
	{
		ulReturn = pdFALSE;
		portICCPMR_PRIORITY_MASK_REGISTER = ( uint32_t ) ( configMAX_API_CALL_INTERRUPT_PRIORITY << portPRIORITY_SHIFT );
		__asm(	"DSB		\n"
				"ISB		\n" );
	}
	__enable_irq();

	return ulReturn;
}


#if( configASSERT_DEFINED == 1 )

	void vPortValidateInterruptPriority( void )
	{

		configASSERT( portICCRPR_RUNNING_PRIORITY_REGISTER >= ( configMAX_API_CALL_INTERRUPT_PRIORITY << portPRIORITY_SHIFT ) );


		configASSERT( portICCBPR_BINARY_POINT_REGISTER <= portMAX_BINARY_POINT_VALUE );
	}

#endif
