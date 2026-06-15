/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        task.h
 *
 * Description:  task.h module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#ifndef INC_TASK_H
#define INC_TASK_H

#ifndef INC_FREERTOS_H
	#error "include FreeRTOS.h must appear in source files before include task.h"
#endif

#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif


#define tskKERNEL_VERSION_NUMBER "V9.0.0"
#define tskKERNEL_VERSION_MAJOR 9
#define tskKERNEL_VERSION_MINOR 0
#define tskKERNEL_VERSION_BUILD 0


typedef void * TaskHandle_t;


typedef BaseType_t (*TaskHookFunction_t)( void * );


typedef enum
{
	eRunning = 0,
	eReady,
	eBlocked,
	eSuspended,
	eDeleted,
	eInvalid
} eTaskState;


typedef enum
{
	eNoAction = 0,
	eSetBits,
	eIncrement,
	eSetValueWithOverwrite,
	eSetValueWithoutOverwrite
} eNotifyAction;


typedef struct xTIME_OUT
{
	BaseType_t xOverflowCount;
	TickType_t xTimeOnEntering;
} TimeOut_t;


typedef struct xMEMORY_REGION
{
	void *pvBaseAddress;
	uint32_t ulLengthInBytes;
	uint32_t ulParameters;
} MemoryRegion_t;


typedef struct xTASK_PARAMETERS
{
	TaskFunction_t pvTaskCode;
	const char * const pcName;
	uint16_t usStackDepth;
	void *pvParameters;
	UBaseType_t uxPriority;
	StackType_t *puxStackBuffer;
	MemoryRegion_t xRegions[ portNUM_CONFIGURABLE_REGIONS ];
} TaskParameters_t;


typedef struct xTASK_STATUS
{
	TaskHandle_t xHandle;
	const char *pcTaskName;
	UBaseType_t xTaskNumber;
	eTaskState eCurrentState;
	UBaseType_t uxCurrentPriority;
	UBaseType_t uxBasePriority;
	uint32_t ulRunTimeCounter;
	StackType_t *pxStackBase;
	uint16_t usStackHighWaterMark;
} TaskStatus_t;


typedef enum
{
	eAbortSleep = 0,
	eStandardSleep,
	eNoTasksWaitingTimeout
} eSleepModeStatus;


#define tskIDLE_PRIORITY			( ( UBaseType_t ) 0U )


#define taskYIELD()					portYIELD()


#define taskENTER_CRITICAL()		portENTER_CRITICAL()
#define taskENTER_CRITICAL_FROM_ISR() portSET_INTERRUPT_MASK_FROM_ISR()


#define taskEXIT_CRITICAL()			portEXIT_CRITICAL()
#define taskEXIT_CRITICAL_FROM_ISR( x ) portCLEAR_INTERRUPT_MASK_FROM_ISR( x )

#define taskDISABLE_INTERRUPTS()	portDISABLE_INTERRUPTS()


#define taskENABLE_INTERRUPTS()		portENABLE_INTERRUPTS()


#define taskSCHEDULER_SUSPENDED		( ( BaseType_t ) 0 )
#define taskSCHEDULER_NOT_STARTED	( ( BaseType_t ) 1 )
#define taskSCHEDULER_RUNNING		( ( BaseType_t ) 2 )


#if( configSUPPORT_DYNAMIC_ALLOCATION == 1 )
	BaseType_t xTaskCreate(	TaskFunction_t pxTaskCode,
							const char * const pcName,
							const uint16_t usStackDepth,
							void * const pvParameters,
							UBaseType_t uxPriority,
							TaskHandle_t * const pxCreatedTask ) PRIVILEGED_FUNCTION;
#endif


#if( configSUPPORT_STATIC_ALLOCATION == 1 )
	TaskHandle_t xTaskCreateStatic(	TaskFunction_t pxTaskCode,
									const char * const pcName,
									const uint32_t ulStackDepth,
									void * const pvParameters,
									UBaseType_t uxPriority,
									StackType_t * const puxStackBuffer,
									StaticTask_t * const pxTaskBuffer ) PRIVILEGED_FUNCTION;
#endif


#if( portUSING_MPU_WRAPPERS == 1 )
	BaseType_t xTaskCreateRestricted( const TaskParameters_t * const pxTaskDefinition, TaskHandle_t *pxCreatedTask ) PRIVILEGED_FUNCTION;
#endif


void vTaskAllocateMPURegions( TaskHandle_t xTask, const MemoryRegion_t * const pxRegions ) PRIVILEGED_FUNCTION;


void vTaskDelete( TaskHandle_t xTaskToDelete ) PRIVILEGED_FUNCTION;


void vTaskDelay( const TickType_t xTicksToDelay ) PRIVILEGED_FUNCTION;


void vTaskDelayUntil( TickType_t * const pxPreviousWakeTime, const TickType_t xTimeIncrement ) PRIVILEGED_FUNCTION;


BaseType_t xTaskAbortDelay( TaskHandle_t xTask ) PRIVILEGED_FUNCTION;


UBaseType_t uxTaskPriorityGet( TaskHandle_t xTask ) PRIVILEGED_FUNCTION;


UBaseType_t uxTaskPriorityGetFromISR( TaskHandle_t xTask ) PRIVILEGED_FUNCTION;


eTaskState eTaskGetState( TaskHandle_t xTask ) PRIVILEGED_FUNCTION;


void vTaskGetInfo( TaskHandle_t xTask, TaskStatus_t *pxTaskStatus, BaseType_t xGetFreeStackSpace, eTaskState eState ) PRIVILEGED_FUNCTION;


void vTaskPrioritySet( TaskHandle_t xTask, UBaseType_t uxNewPriority ) PRIVILEGED_FUNCTION;


void vTaskSuspend( TaskHandle_t xTaskToSuspend ) PRIVILEGED_FUNCTION;


void vTaskResume( TaskHandle_t xTaskToResume ) PRIVILEGED_FUNCTION;


BaseType_t xTaskResumeFromISR( TaskHandle_t xTaskToResume ) PRIVILEGED_FUNCTION;


void vTaskStartScheduler( void ) PRIVILEGED_FUNCTION;


void vTaskEndScheduler( void ) PRIVILEGED_FUNCTION;


void vTaskSuspendAll( void ) PRIVILEGED_FUNCTION;


BaseType_t xTaskResumeAll( void ) PRIVILEGED_FUNCTION;


TickType_t xTaskGetTickCount( void ) PRIVILEGED_FUNCTION;


TickType_t xTaskGetTickCountFromISR( void ) PRIVILEGED_FUNCTION;


UBaseType_t uxTaskGetNumberOfTasks( void ) PRIVILEGED_FUNCTION;


char *pcTaskGetName( TaskHandle_t xTaskToQuery ) PRIVILEGED_FUNCTION;


TaskHandle_t xTaskGetHandle( const char *pcNameToQuery ) PRIVILEGED_FUNCTION;


UBaseType_t uxTaskGetStackHighWaterMark( TaskHandle_t xTask ) PRIVILEGED_FUNCTION;


#ifdef configUSE_APPLICATION_TASK_TAG
	#if configUSE_APPLICATION_TASK_TAG == 1

		void vTaskSetApplicationTaskTag( TaskHandle_t xTask, TaskHookFunction_t pxHookFunction ) PRIVILEGED_FUNCTION;


		TaskHookFunction_t xTaskGetApplicationTaskTag( TaskHandle_t xTask ) PRIVILEGED_FUNCTION;
	#endif
#endif

#if( configNUM_THREAD_LOCAL_STORAGE_POINTERS > 0 )


	void vTaskSetThreadLocalStoragePointer( TaskHandle_t xTaskToSet, BaseType_t xIndex, void *pvValue ) PRIVILEGED_FUNCTION;
	void *pvTaskGetThreadLocalStoragePointer( TaskHandle_t xTaskToQuery, BaseType_t xIndex ) PRIVILEGED_FUNCTION;

#endif


BaseType_t xTaskCallApplicationTaskHook( TaskHandle_t xTask, void *pvParameter ) PRIVILEGED_FUNCTION;


TaskHandle_t xTaskGetIdleTaskHandle( void ) PRIVILEGED_FUNCTION;


UBaseType_t uxTaskGetSystemState( TaskStatus_t * const pxTaskStatusArray, const UBaseType_t uxArraySize, uint32_t * const pulTotalRunTime ) PRIVILEGED_FUNCTION;


void vTaskList( char * pcWriteBuffer ) PRIVILEGED_FUNCTION;


void vTaskGetRunTimeStats( char *pcWriteBuffer ) PRIVILEGED_FUNCTION;


BaseType_t xTaskGenericNotify( TaskHandle_t xTaskToNotify, uint32_t ulValue, eNotifyAction eAction, uint32_t *pulPreviousNotificationValue ) PRIVILEGED_FUNCTION;
#define xTaskNotify( xTaskToNotify, ulValue, eAction ) xTaskGenericNotify( ( xTaskToNotify ), ( ulValue ), ( eAction ), NULL )
#define xTaskNotifyAndQuery( xTaskToNotify, ulValue, eAction, pulPreviousNotifyValue ) xTaskGenericNotify( ( xTaskToNotify ), ( ulValue ), ( eAction ), ( pulPreviousNotifyValue ) )


BaseType_t xTaskGenericNotifyFromISR( TaskHandle_t xTaskToNotify, uint32_t ulValue, eNotifyAction eAction, uint32_t *pulPreviousNotificationValue, BaseType_t *pxHigherPriorityTaskWoken ) PRIVILEGED_FUNCTION;
#define xTaskNotifyFromISR( xTaskToNotify, ulValue, eAction, pxHigherPriorityTaskWoken ) xTaskGenericNotifyFromISR( ( xTaskToNotify ), ( ulValue ), ( eAction ), NULL, ( pxHigherPriorityTaskWoken ) )
#define xTaskNotifyAndQueryFromISR( xTaskToNotify, ulValue, eAction, pulPreviousNotificationValue, pxHigherPriorityTaskWoken ) xTaskGenericNotifyFromISR( ( xTaskToNotify ), ( ulValue ), ( eAction ), ( pulPreviousNotificationValue ), ( pxHigherPriorityTaskWoken ) )


BaseType_t xTaskNotifyWait( uint32_t ulBitsToClearOnEntry, uint32_t ulBitsToClearOnExit, uint32_t *pulNotificationValue, TickType_t xTicksToWait ) PRIVILEGED_FUNCTION;


#define xTaskNotifyGive( xTaskToNotify ) xTaskGenericNotify( ( xTaskToNotify ), ( 0 ), eIncrement, NULL )


void vTaskNotifyGiveFromISR( TaskHandle_t xTaskToNotify, BaseType_t *pxHigherPriorityTaskWoken ) PRIVILEGED_FUNCTION;


uint32_t ulTaskNotifyTake( BaseType_t xClearCountOnExit, TickType_t xTicksToWait ) PRIVILEGED_FUNCTION;


BaseType_t xTaskNotifyStateClear( TaskHandle_t xTask );


BaseType_t xTaskIncrementTick( void ) PRIVILEGED_FUNCTION;


void vTaskPlaceOnEventList( List_t * const pxEventList, const TickType_t xTicksToWait ) PRIVILEGED_FUNCTION;
void vTaskPlaceOnUnorderedEventList( List_t * pxEventList, const TickType_t xItemValue, const TickType_t xTicksToWait ) PRIVILEGED_FUNCTION;


void vTaskPlaceOnEventListRestricted( List_t * const pxEventList, TickType_t xTicksToWait, const BaseType_t xWaitIndefinitely ) PRIVILEGED_FUNCTION;


BaseType_t xTaskRemoveFromEventList( const List_t * const pxEventList ) PRIVILEGED_FUNCTION;
BaseType_t xTaskRemoveFromUnorderedEventList( ListItem_t * pxEventListItem, const TickType_t xItemValue ) PRIVILEGED_FUNCTION;


void vTaskSwitchContext( void ) PRIVILEGED_FUNCTION;


TickType_t uxTaskResetEventItemValue( void ) PRIVILEGED_FUNCTION;


TaskHandle_t xTaskGetCurrentTaskHandle( void ) PRIVILEGED_FUNCTION;


void vTaskSetTimeOutState( TimeOut_t * const pxTimeOut ) PRIVILEGED_FUNCTION;


BaseType_t xTaskCheckForTimeOut( TimeOut_t * const pxTimeOut, TickType_t * const pxTicksToWait ) PRIVILEGED_FUNCTION;


void vTaskMissedYield( void ) PRIVILEGED_FUNCTION;


BaseType_t xTaskGetSchedulerState( void ) PRIVILEGED_FUNCTION;


void vTaskPriorityInherit( TaskHandle_t const pxMutexHolder ) PRIVILEGED_FUNCTION;


BaseType_t xTaskPriorityDisinherit( TaskHandle_t const pxMutexHolder ) PRIVILEGED_FUNCTION;


UBaseType_t uxTaskGetTaskNumber( TaskHandle_t xTask ) PRIVILEGED_FUNCTION;


void vTaskSetTaskNumber( TaskHandle_t xTask, const UBaseType_t uxHandle ) PRIVILEGED_FUNCTION;


void vTaskStepTick( const TickType_t xTicksToJump ) PRIVILEGED_FUNCTION;


eSleepModeStatus eTaskConfirmSleepModeStatus( void ) PRIVILEGED_FUNCTION;


void *pvTaskIncrementMutexHeldCount( void ) PRIVILEGED_FUNCTION;

#ifdef __cplusplus
}
#endif
#endif
