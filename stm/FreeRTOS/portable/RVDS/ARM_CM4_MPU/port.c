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

#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE


#include "FreeRTOS.h"
#include "queue.h"
#include "event_groups.h"
#include "mpu_prototypes.h"

#ifndef __TARGET_FPU_VFP
	#error This port can only be used when the project options are configured to enable hardware floating point support.
#endif

#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE


#define portNVIC_SYSTICK_CTRL_REG				( * ( ( volatile uint32_t * ) 0xe000e010 ) )
#define portNVIC_SYSTICK_LOAD_REG				( * ( ( volatile uint32_t * ) 0xe000e014 ) )
#define portNVIC_SYSPRI2_REG					( *	( ( volatile uint32_t * ) 0xe000ed20 ) )
#define portNVIC_SYSPRI1_REG					( * ( ( volatile uint32_t * ) 0xe000ed1c ) )
#define portNVIC_SYS_CTRL_STATE_REG				( * ( ( volatile uint32_t * ) 0xe000ed24 ) )
#define portNVIC_MEM_FAULT_ENABLE				( 1UL << 16UL )


#define portMPU_TYPE_REG						( * ( ( volatile uint32_t * ) 0xe000ed90 ) )
#define portMPU_REGION_BASE_ADDRESS_REG			( * ( ( volatile uint32_t * ) 0xe000ed9C ) )
#define portMPU_REGION_ATTRIBUTE_REG			( * ( ( volatile uint32_t * ) 0xe000edA0 ) )
#define portMPU_CTRL_REG						( * ( ( volatile uint32_t * ) 0xe000ed94 ) )
#define portEXPECTED_MPU_TYPE_VALUE				( 8UL << 8UL )
#define portMPU_ENABLE							( 0x01UL )
#define portMPU_BACKGROUND_ENABLE				( 1UL << 2UL )
#define portPRIVILEGED_EXECUTION_START_ADDRESS	( 0UL )
#define portMPU_REGION_VALID					( 0x10UL )
#define portMPU_REGION_ENABLE					( 0x01UL )
#define portPERIPHERALS_START_ADDRESS			0x40000000UL
#define portPERIPHERALS_END_ADDRESS				0x5FFFFFFFUL


#define portNVIC_SYSTICK_CLK					( 0x00000004UL )
#define portNVIC_SYSTICK_INT					( 0x00000002UL )
#define portNVIC_SYSTICK_ENABLE					( 0x00000001UL )
#define portNVIC_PENDSV_PRI						( ( ( uint32_t ) configKERNEL_INTERRUPT_PRIORITY ) << 16UL )
#define portNVIC_SYSTICK_PRI					( ( ( uint32_t ) configKERNEL_INTERRUPT_PRIORITY ) << 24UL )
#define portNVIC_SVC_PRI						( ( ( uint32_t ) configMAX_SYSCALL_INTERRUPT_PRIORITY - 1UL ) << 24UL )


#define portFPCCR								( ( volatile uint32_t * ) 0xe000ef34UL )
#define portASPEN_AND_LSPEN_BITS				( 0x3UL << 30UL )


#define portINITIAL_XPSR						( 0x01000000UL )
#define portINITIAL_EXEC_RETURN					( 0xfffffffdUL )
#define portINITIAL_CONTROL_IF_UNPRIVILEGED		( 0x03 )
#define portINITIAL_CONTROL_IF_PRIVILEGED		( 0x02 )


#define portFIRST_USER_INTERRUPT_NUMBER		( 16 )
#define portNVIC_IP_REGISTERS_OFFSET_16 	( 0xE000E3F0 )
#define portAIRCR_REG						( * ( ( volatile uint32_t * ) 0xE000ED0C ) )
#define portMAX_8_BIT_VALUE					( ( uint8_t ) 0xff )
#define portTOP_BIT_OF_BYTE					( ( uint8_t ) 0x80 )
#define portMAX_PRIGROUP_BITS				( ( uint8_t ) 7 )
#define portPRIORITY_GROUP_MASK				( 0x07UL << 8UL )
#define portPRIGROUP_SHIFT					( 8UL )


#define portOFFSET_TO_PC						( 6 )


#define portSTART_ADDRESS_MASK				( ( StackType_t ) 0xfffffffeUL )


static UBaseType_t uxCriticalNesting = 0xaaaaaaaa;


static void prvSetupTimerInterrupt( void ) PRIVILEGED_FUNCTION;


static void prvSetupMPU( void ) PRIVILEGED_FUNCTION;


static void prvStartFirstTask( void ) PRIVILEGED_FUNCTION;


static uint32_t prvGetMPURegionSizeSetting( uint32_t ulActualSizeInBytes ) PRIVILEGED_FUNCTION;


BaseType_t xPortRaisePrivilege( void );


void xPortPendSVHandler( void ) PRIVILEGED_FUNCTION;
void xPortSysTickHandler( void ) PRIVILEGED_FUNCTION;
void vPortSVCHandler( void ) PRIVILEGED_FUNCTION;


static void prvRestoreContextOfFirstTask( void ) PRIVILEGED_FUNCTION;


void prvSVCHandler( uint32_t *pulRegisters ) __attribute__((used)) PRIVILEGED_FUNCTION;


static void vPortEnableVFP( void );


static uint32_t prvPortGetIPSR( void );


#if ( configASSERT_DEFINED == 1 )
	 static uint8_t ucMaxSysCallPriority = 0;
	 static uint32_t ulMaxPRIGROUPValue = 0;
	 static const volatile uint8_t * const pcInterruptPriorityRegisters = ( const uint8_t * ) portNVIC_IP_REGISTERS_OFFSET_16;
#endif


StackType_t *pxPortInitialiseStack( StackType_t *pxTopOfStack, TaskFunction_t pxCode, void *pvParameters, BaseType_t xRunPrivileged )
{

	pxTopOfStack--;
	*pxTopOfStack = portINITIAL_XPSR;
	pxTopOfStack--;
	*pxTopOfStack = ( ( StackType_t ) pxCode ) & portSTART_ADDRESS_MASK;
	pxTopOfStack--;
	*pxTopOfStack = 0;
	pxTopOfStack -= 5;
	*pxTopOfStack = ( StackType_t ) pvParameters;


	pxTopOfStack--;
	*pxTopOfStack = portINITIAL_EXEC_RETURN;

	pxTopOfStack -= 9;

	if( xRunPrivileged == pdTRUE )
	{
		*pxTopOfStack = portINITIAL_CONTROL_IF_PRIVILEGED;
	}
	else
	{
		*pxTopOfStack = portINITIAL_CONTROL_IF_UNPRIVILEGED;
	}

	return pxTopOfStack;
}


void prvSVCHandler( uint32_t *pulParam )
{
uint8_t ucSVCNumber;
uint32_t ulReg;


	ucSVCNumber = ( ( uint8_t * ) pulParam[ portOFFSET_TO_PC ] )[ -2 ];
	switch( ucSVCNumber )
	{
		case portSVC_START_SCHEDULER	:	portNVIC_SYSPRI1_REG |= portNVIC_SVC_PRI;
											prvRestoreContextOfFirstTask();
											break;

		case portSVC_YIELD				:	portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;

											__asm volatile( "dsb" );
											__asm volatile( "isb" );

											break;

		case portSVC_RAISE_PRIVILEGE	:	__asm
											{
												mrs ulReg, control
												bic ulReg, #1
												msr control, ulReg
											}
											break;

		default							:
											break;
	}
}


__asm void vPortSVCHandler( void )
{
	extern prvSVCHandler

	PRESERVE8


	#ifndef USE_PROCESS_STACK
		tst lr, #4
		ite eq
		mrseq r0, msp
		mrsne r0, psp
	#else
		mrs r0, psp
	#endif
		b prvSVCHandler
}


__asm void prvRestoreContextOfFirstTask( void )
{
	PRESERVE8

	ldr r0, =0xE000ED08
	ldr r0, [r0]
	ldr r0, [r0]
	msr msp, r0
	ldr	r3, =pxCurrentTCB
	ldr r1, [r3]
	ldr r0, [r1]
	add r1, r1, #4
	ldr r2, =0xe000ed9c
	ldmia r1!, {r4-r11}
	stmia r2!, {r4-r11}
	ldmia r0!, {r3-r11, r14}
	msr control, r3
	msr psp, r0
	mov r0, #0
	msr	basepri, r0
	bx r14
	nop
}


BaseType_t xPortStartScheduler( void )
{

	configASSERT( ( configMAX_SYSCALL_INTERRUPT_PRIORITY ) );

	#if( configASSERT_DEFINED == 1 )
	{
		volatile uint32_t ulOriginalPriority;
		volatile uint8_t * const pucFirstUserPriorityRegister = ( volatile uint8_t * ) ( portNVIC_IP_REGISTERS_OFFSET_16 + portFIRST_USER_INTERRUPT_NUMBER );
		volatile uint8_t ucMaxPriorityValue;


		ulOriginalPriority = *pucFirstUserPriorityRegister;


		*pucFirstUserPriorityRegister = portMAX_8_BIT_VALUE;


		ucMaxPriorityValue = *pucFirstUserPriorityRegister;


		ucMaxSysCallPriority = configMAX_SYSCALL_INTERRUPT_PRIORITY & ucMaxPriorityValue;


		ulMaxPRIGROUPValue = portMAX_PRIGROUP_BITS;
		while( ( ucMaxPriorityValue & portTOP_BIT_OF_BYTE ) == portTOP_BIT_OF_BYTE )
		{
			ulMaxPRIGROUPValue--;
			ucMaxPriorityValue <<= ( uint8_t ) 0x01;
		}


		ulMaxPRIGROUPValue <<= portPRIGROUP_SHIFT;
		ulMaxPRIGROUPValue &= portPRIORITY_GROUP_MASK;


		*pucFirstUserPriorityRegister = ulOriginalPriority;
	}
	#endif


	portNVIC_SYSPRI2_REG |= portNVIC_PENDSV_PRI;
	portNVIC_SYSPRI2_REG |= portNVIC_SYSTICK_PRI;


	prvSetupMPU();


	prvSetupTimerInterrupt();


	uxCriticalNesting = 0;


	vPortEnableVFP();


	*( portFPCCR ) |= portASPEN_AND_LSPEN_BITS;


	prvStartFirstTask();


	return 0;
}


__asm void prvStartFirstTask( void )
{
	PRESERVE8

	ldr r0, =0xE000ED08
	ldr r0, [r0]
	ldr r0, [r0]
	msr msp, r0
	cpsie i
	cpsie f
	dsb
	isb
	svc portSVC_START_SCHEDULER
	nop
	nop
}

void vPortEndScheduler( void )
{

	configASSERT( uxCriticalNesting == 1000UL );
}


void vPortEnterCritical( void )
{
BaseType_t xRunningPrivileged = xPortRaisePrivilege();

	portDISABLE_INTERRUPTS();
	uxCriticalNesting++;

	vPortResetPrivilege( xRunningPrivileged );
}


void vPortExitCritical( void )
{
BaseType_t xRunningPrivileged = xPortRaisePrivilege();

	configASSERT( uxCriticalNesting );
	uxCriticalNesting--;
	if( uxCriticalNesting == 0 )
	{
		portENABLE_INTERRUPTS();
	}
	vPortResetPrivilege( xRunningPrivileged );
}


__asm void xPortPendSVHandler( void )
{
	extern uxCriticalNesting;
	extern pxCurrentTCB;
	extern vTaskSwitchContext;

	PRESERVE8

	mrs r0, psp

	ldr	r3, =pxCurrentTCB
	ldr	r2, [r3]

	tst r14, #0x10
	it eq
	vstmdbeq r0!, {s16-s31}

	mrs r1, control
	stmdb r0!, {r1, r4-r11, r14}
	str r0, [r2]

	stmdb sp!, {r3}
	mov r0, #configMAX_SYSCALL_INTERRUPT_PRIORITY
	msr basepri, r0
	dsb
	isb
	bl vTaskSwitchContext
	mov r0, #0
	msr basepri, r0
	ldmia sp!, {r3}

	ldr r1, [r3]
	ldr r0, [r1]
	add r1, r1, #4
	ldr r2, =0xe000ed9c
	ldmia r1!, {r4-r11}
	stmia r2!, {r4-r11}
	ldmia r0!, {r3-r11, r14}
	msr control, r3

	tst r14, #0x10
	it eq
	vldmiaeq r0!, {s16-s31}

	msr psp, r0
	bx r14
	nop
}


void xPortSysTickHandler( void )
{
uint32_t ulDummy;

	ulDummy = portSET_INTERRUPT_MASK_FROM_ISR();
	{

		if( xTaskIncrementTick() != pdFALSE )
		{

			portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;
		}
	}
	portCLEAR_INTERRUPT_MASK_FROM_ISR( ulDummy );
}


static void prvSetupTimerInterrupt( void )
{

	portNVIC_SYSTICK_LOAD_REG = ( configCPU_CLOCK_HZ / configTICK_RATE_HZ ) - 1UL;
	portNVIC_SYSTICK_CTRL_REG = portNVIC_SYSTICK_CLK | portNVIC_SYSTICK_INT | portNVIC_SYSTICK_ENABLE;
}


__asm void vPortSwitchToUserMode( void )
{
	PRESERVE8

	mrs r0, control
	orr r0, #1
	msr control, r0
	bx r14
}


__asm void vPortEnableVFP( void )
{
	PRESERVE8

	ldr.w r0, =0xE000ED88
	ldr r1, [r0]

	orr r1, r1, #( 0xf << 20 )
	str r1, [r0]
	bx r14
	nop
	nop
}


static void prvSetupMPU( void )
{
extern uint32_t __privileged_functions_end__;
extern uint32_t __FLASH_segment_start__;
extern uint32_t __FLASH_segment_end__;
extern uint32_t __privileged_data_start__;
extern uint32_t __privileged_data_end__;


	if( portMPU_TYPE_REG == portEXPECTED_MPU_TYPE_VALUE )
	{

		portMPU_REGION_BASE_ADDRESS_REG =	( ( uint32_t ) __FLASH_segment_start__ ) |
											( portMPU_REGION_VALID ) |
											( portUNPRIVILEGED_FLASH_REGION );

		portMPU_REGION_ATTRIBUTE_REG =	( portMPU_REGION_READ_ONLY ) |
										( portMPU_REGION_CACHEABLE_BUFFERABLE ) |
										( prvGetMPURegionSizeSetting( ( uint32_t ) __FLASH_segment_end__ - ( uint32_t ) __FLASH_segment_start__ ) ) |
										( portMPU_REGION_ENABLE );


		portMPU_REGION_BASE_ADDRESS_REG =	( ( uint32_t ) __FLASH_segment_start__ ) |
											( portMPU_REGION_VALID ) |
											( portPRIVILEGED_FLASH_REGION );

		portMPU_REGION_ATTRIBUTE_REG =	( portMPU_REGION_PRIVILEGED_READ_ONLY ) |
										( portMPU_REGION_CACHEABLE_BUFFERABLE ) |
										( prvGetMPURegionSizeSetting( ( uint32_t ) __privileged_functions_end__ - ( uint32_t ) __FLASH_segment_start__ ) ) |
										( portMPU_REGION_ENABLE );


		portMPU_REGION_BASE_ADDRESS_REG =	( ( uint32_t ) __privileged_data_start__ ) |
											( portMPU_REGION_VALID ) |
											( portPRIVILEGED_RAM_REGION );

		portMPU_REGION_ATTRIBUTE_REG =	( portMPU_REGION_PRIVILEGED_READ_WRITE ) |
										( portMPU_REGION_CACHEABLE_BUFFERABLE ) |
										prvGetMPURegionSizeSetting( ( uint32_t ) __privileged_data_end__ - ( uint32_t ) __privileged_data_start__ ) |
										( portMPU_REGION_ENABLE );


		portMPU_REGION_BASE_ADDRESS_REG =	( portPERIPHERALS_START_ADDRESS ) |
											( portMPU_REGION_VALID ) |
											( portGENERAL_PERIPHERALS_REGION );

		portMPU_REGION_ATTRIBUTE_REG =	( portMPU_REGION_READ_WRITE | portMPU_REGION_EXECUTE_NEVER ) |
										( prvGetMPURegionSizeSetting( portPERIPHERALS_END_ADDRESS - portPERIPHERALS_START_ADDRESS ) ) |
										( portMPU_REGION_ENABLE );


		portNVIC_SYS_CTRL_STATE_REG |= portNVIC_MEM_FAULT_ENABLE;


		portMPU_CTRL_REG |= ( portMPU_ENABLE | portMPU_BACKGROUND_ENABLE );
	}
}


static uint32_t prvGetMPURegionSizeSetting( uint32_t ulActualSizeInBytes )
{
uint32_t ulRegionSize, ulReturnValue = 4;


	for( ulRegionSize = 32UL; ulReturnValue < 31UL; ( ulRegionSize <<= 1UL ) )
	{
		if( ulActualSizeInBytes <= ulRegionSize )
		{
			break;
		}
		else
		{
			ulReturnValue++;
		}
	}


	return ( ulReturnValue << 1UL );
}


__asm BaseType_t xPortRaisePrivilege( void )
{
	mrs r0, control
	tst r0, #1
	itte ne
	movne r0, #0
	svcne portSVC_RAISE_PRIVILEGE
	moveq r0, #1
	bx lr
}


void vPortStoreTaskMPUSettings( xMPU_SETTINGS *xMPUSettings, const struct xMEMORY_REGION * const xRegions, StackType_t *pxBottomOfStack, uint32_t ulStackDepth )
{
extern uint32_t __SRAM_segment_start__;
extern uint32_t __SRAM_segment_end__;
extern uint32_t __privileged_data_start__;
extern uint32_t __privileged_data_end__;


int32_t lIndex;
uint32_t ul;

	if( xRegions == NULL )
	{

		xMPUSettings->xRegion[ 0 ].ulRegionBaseAddress =
				( ( uint32_t ) __SRAM_segment_start__ ) |
				( portMPU_REGION_VALID ) |
				( portSTACK_REGION );

		xMPUSettings->xRegion[ 0 ].ulRegionAttribute =
				( portMPU_REGION_READ_WRITE ) |
				( portMPU_REGION_CACHEABLE_BUFFERABLE ) |
				( prvGetMPURegionSizeSetting( ( uint32_t ) __SRAM_segment_end__ - ( uint32_t ) __SRAM_segment_start__ ) ) |
				( portMPU_REGION_ENABLE );


		xMPUSettings->xRegion[ 1 ].ulRegionBaseAddress =
				( ( uint32_t ) __privileged_data_start__ ) |
				( portMPU_REGION_VALID ) |
				( portSTACK_REGION + 1 );

		xMPUSettings->xRegion[ 1 ].ulRegionAttribute =
				( portMPU_REGION_PRIVILEGED_READ_WRITE ) |
				( portMPU_REGION_CACHEABLE_BUFFERABLE ) |
				prvGetMPURegionSizeSetting( ( uint32_t ) __privileged_data_end__ - ( uint32_t ) __privileged_data_start__ ) |
				( portMPU_REGION_ENABLE );


		for( ul = 2; ul <= portNUM_CONFIGURABLE_REGIONS; ul++ )
		{
			xMPUSettings->xRegion[ ul ].ulRegionBaseAddress = ( portSTACK_REGION + ul ) | portMPU_REGION_VALID;
			xMPUSettings->xRegion[ ul ].ulRegionAttribute = 0UL;
		}
	}
	else
	{

		if( ulStackDepth > 0 )
		{

			xMPUSettings->xRegion[ 0 ].ulRegionBaseAddress =
					( ( uint32_t ) pxBottomOfStack ) |
					( portMPU_REGION_VALID ) |
					( portSTACK_REGION );

			xMPUSettings->xRegion[ 0 ].ulRegionAttribute =
					( portMPU_REGION_READ_WRITE ) |
					( prvGetMPURegionSizeSetting( ulStackDepth * ( uint32_t ) sizeof( StackType_t ) ) ) |
					( portMPU_REGION_CACHEABLE_BUFFERABLE ) |
					( portMPU_REGION_ENABLE );
		}

		lIndex = 0;

		for( ul = 1; ul <= portNUM_CONFIGURABLE_REGIONS; ul++ )
		{
			if( ( xRegions[ lIndex ] ).ulLengthInBytes > 0UL )
			{

				xMPUSettings->xRegion[ ul ].ulRegionBaseAddress =
						( ( uint32_t ) xRegions[ lIndex ].pvBaseAddress ) |
						( portMPU_REGION_VALID ) |
						( portSTACK_REGION + ul );

				xMPUSettings->xRegion[ ul ].ulRegionAttribute =
						( prvGetMPURegionSizeSetting( xRegions[ lIndex ].ulLengthInBytes ) ) |
						( xRegions[ lIndex ].ulParameters ) |
						( portMPU_REGION_ENABLE );
			}
			else
			{

				xMPUSettings->xRegion[ ul ].ulRegionBaseAddress = ( portSTACK_REGION + ul ) | portMPU_REGION_VALID;
				xMPUSettings->xRegion[ ul ].ulRegionAttribute = 0UL;
			}

			lIndex++;
		}
	}
}


__asm uint32_t prvPortGetIPSR( void )
{
	PRESERVE8

	mrs r0, ipsr
	bx r14
}


#if( configASSERT_DEFINED == 1 )

	void vPortValidateInterruptPriority( void )
	{
	uint32_t ulCurrentInterrupt;
	uint8_t ucCurrentPriority;


		ulCurrentInterrupt = prvPortGetIPSR();


		if( ulCurrentInterrupt >= portFIRST_USER_INTERRUPT_NUMBER )
		{

			ucCurrentPriority = pcInterruptPriorityRegisters[ ulCurrentInterrupt ];


			configASSERT( ucCurrentPriority >= ucMaxSysCallPriority );
		}


		configASSERT( ( portAIRCR_REG & portPRIORITY_GROUP_MASK ) <= ulMaxPRIGROUPValue );
	}

#endif
