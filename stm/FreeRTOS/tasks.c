/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        tasks.c
 *
 * Description:  tasks.c module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#include <stdlib.h>
#include <string.h>


#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE


#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "StackMacros.h"


#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE


#if ( configUSE_STATS_FORMATTING_FUNCTIONS == 1 )

	#include <stdio.h>
#endif

#if( configUSE_PREEMPTION == 0 )

	#define taskYIELD_IF_USING_PREEMPTION()
#else
	#define taskYIELD_IF_USING_PREEMPTION() portYIELD_WITHIN_API()
#endif


#define taskNOT_WAITING_NOTIFICATION	( ( uint8_t ) 0 )
#define taskWAITING_NOTIFICATION		( ( uint8_t ) 1 )
#define taskNOTIFICATION_RECEIVED		( ( uint8_t ) 2 )


#define tskSTACK_FILL_BYTE	( 0xa5U )


#define tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE ( ( ( configSUPPORT_STATIC_ALLOCATION == 1 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) ) || ( portUSING_MPU_WRAPPERS == 1 ) )
#define tskDYNAMICALLY_ALLOCATED_STACK_AND_TCB 		( ( uint8_t ) 0 )
#define tskSTATICALLY_ALLOCATED_STACK_ONLY 			( ( uint8_t ) 1 )
#define tskSTATICALLY_ALLOCATED_STACK_AND_TCB		( ( uint8_t ) 2 )


#define tskBLOCKED_CHAR		( 'B' )
#define tskREADY_CHAR		( 'R' )
#define tskDELETED_CHAR		( 'D' )
#define tskSUSPENDED_CHAR	( 'S' )


#ifdef portREMOVE_STATIC_QUALIFIER
	#define static
#endif

#if ( configUSE_PORT_OPTIMISED_TASK_SELECTION == 0 )


	#define taskRECORD_READY_PRIORITY( uxPriority )														\
	{																									\
		if( ( uxPriority ) > uxTopReadyPriority )														\
		{																								\
			uxTopReadyPriority = ( uxPriority );														\
		}																								\
	}


	#define taskSELECT_HIGHEST_PRIORITY_TASK()															\
	{																									\
	UBaseType_t uxTopPriority = uxTopReadyPriority;														\
																										\
										\
		while( listLIST_IS_EMPTY( &( pxReadyTasksLists[ uxTopPriority ] ) ) )							\
		{																								\
			configASSERT( uxTopPriority );																\
			--uxTopPriority;																			\
		}																								\
																										\
											\
		listGET_OWNER_OF_NEXT_ENTRY( pxCurrentTCB, &( pxReadyTasksLists[ uxTopPriority ] ) );			\
		uxTopReadyPriority = uxTopPriority;																\
	}


	#define taskRESET_READY_PRIORITY( uxPriority )
	#define portRESET_READY_PRIORITY( uxPriority, uxTopReadyPriority )

#else


	#define taskRECORD_READY_PRIORITY( uxPriority )	portRECORD_READY_PRIORITY( uxPriority, uxTopReadyPriority )


	#define taskSELECT_HIGHEST_PRIORITY_TASK()														\
	{																								\
	UBaseType_t uxTopPriority;																		\
																									\
										\
		portGET_HIGHEST_PRIORITY( uxTopPriority, uxTopReadyPriority );								\
		configASSERT( listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[ uxTopPriority ] ) ) > 0 );		\
		listGET_OWNER_OF_NEXT_ENTRY( pxCurrentTCB, &( pxReadyTasksLists[ uxTopPriority ] ) );		\
	}


	#define taskRESET_READY_PRIORITY( uxPriority )														\
	{																									\
		if( listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[ ( uxPriority ) ] ) ) == ( UBaseType_t ) 0 )	\
		{																								\
			portRESET_READY_PRIORITY( ( uxPriority ), ( uxTopReadyPriority ) );							\
		}																								\
	}

#endif


#define taskSWITCH_DELAYED_LISTS()																	\
{																									\
	List_t *pxTemp;																					\
																									\
							\
	configASSERT( ( listLIST_IS_EMPTY( pxDelayedTaskList ) ) );										\
																									\
	pxTemp = pxDelayedTaskList;																		\
	pxDelayedTaskList = pxOverflowDelayedTaskList;													\
	pxOverflowDelayedTaskList = pxTemp;																\
	xNumOfOverflows++;																				\
	prvResetNextTaskUnblockTime();																	\
}


#define prvAddTaskToReadyList( pxTCB )																\
	traceMOVED_TASK_TO_READY_STATE( pxTCB );														\
	taskRECORD_READY_PRIORITY( ( pxTCB )->uxPriority );												\
	vListInsertEnd( &( pxReadyTasksLists[ ( pxTCB )->uxPriority ] ), &( ( pxTCB )->xStateListItem ) ); \
	tracePOST_MOVED_TASK_TO_READY_STATE( pxTCB )


#define prvGetTCBFromHandle( pxHandle ) ( ( ( pxHandle ) == NULL ) ? ( TCB_t * ) pxCurrentTCB : ( TCB_t * ) ( pxHandle ) )


#if( configUSE_16_BIT_TICKS == 1 )
	#define taskEVENT_LIST_ITEM_VALUE_IN_USE	0x8000U
#else
	#define taskEVENT_LIST_ITEM_VALUE_IN_USE	0x80000000UL
#endif


typedef struct tskTaskControlBlock
{
	volatile StackType_t	*pxTopOfStack;

	#if ( portUSING_MPU_WRAPPERS == 1 )
		xMPU_SETTINGS	xMPUSettings;
	#endif

	ListItem_t			xStateListItem;
	ListItem_t			xEventListItem;
	UBaseType_t			uxPriority;
	StackType_t			*pxStack;
	char				pcTaskName[ configMAX_TASK_NAME_LEN ];

	#if ( portSTACK_GROWTH > 0 )
		StackType_t		*pxEndOfStack;
	#endif

	#if ( portCRITICAL_NESTING_IN_TCB == 1 )
		UBaseType_t		uxCriticalNesting;
	#endif

	#if ( configUSE_TRACE_FACILITY == 1 )
		UBaseType_t		uxTCBNumber;
		UBaseType_t		uxTaskNumber;
	#endif

	#if ( configUSE_MUTEXES == 1 )
		UBaseType_t		uxBasePriority;
		UBaseType_t		uxMutexesHeld;
	#endif

	#if ( configUSE_APPLICATION_TASK_TAG == 1 )
		TaskHookFunction_t pxTaskTag;
	#endif

	#if( configNUM_THREAD_LOCAL_STORAGE_POINTERS > 0 )
		void *pvThreadLocalStoragePointers[ configNUM_THREAD_LOCAL_STORAGE_POINTERS ];
	#endif

	#if( configGENERATE_RUN_TIME_STATS == 1 )
		uint32_t		ulRunTimeCounter;
	#endif

	#if ( configUSE_NEWLIB_REENTRANT == 1 )

		struct	_reent xNewLib_reent;
	#endif

	#if( configUSE_TASK_NOTIFICATIONS == 1 )
		volatile uint32_t ulNotifiedValue;
		volatile uint8_t ucNotifyState;
	#endif


	#if( tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE != 0 )
		uint8_t	ucStaticallyAllocated;
	#endif

	#if( INCLUDE_xTaskAbortDelay == 1 )
		uint8_t ucDelayAborted;
	#endif

} tskTCB;


typedef tskTCB TCB_t;


PRIVILEGED_DATA TCB_t * volatile pxCurrentTCB = NULL;


PRIVILEGED_DATA static List_t pxReadyTasksLists[ configMAX_PRIORITIES ];
PRIVILEGED_DATA static List_t xDelayedTaskList1;
PRIVILEGED_DATA static List_t xDelayedTaskList2;
PRIVILEGED_DATA static List_t * volatile pxDelayedTaskList;
PRIVILEGED_DATA static List_t * volatile pxOverflowDelayedTaskList;
PRIVILEGED_DATA static List_t xPendingReadyList;

#if( INCLUDE_vTaskDelete == 1 )

	PRIVILEGED_DATA static List_t xTasksWaitingTermination;
	PRIVILEGED_DATA static volatile UBaseType_t uxDeletedTasksWaitingCleanUp = ( UBaseType_t ) 0U;

#endif

#if ( INCLUDE_vTaskSuspend == 1 )

	PRIVILEGED_DATA static List_t xSuspendedTaskList;

#endif


PRIVILEGED_DATA static volatile UBaseType_t uxCurrentNumberOfTasks 	= ( UBaseType_t ) 0U;
PRIVILEGED_DATA static volatile TickType_t xTickCount 				= ( TickType_t ) 0U;
PRIVILEGED_DATA static volatile UBaseType_t uxTopReadyPriority 		= tskIDLE_PRIORITY;
PRIVILEGED_DATA static volatile BaseType_t xSchedulerRunning 		= pdFALSE;
PRIVILEGED_DATA static volatile UBaseType_t uxPendedTicks 			= ( UBaseType_t ) 0U;
PRIVILEGED_DATA static volatile BaseType_t xYieldPending 			= pdFALSE;
PRIVILEGED_DATA static volatile BaseType_t xNumOfOverflows 			= ( BaseType_t ) 0;
PRIVILEGED_DATA static UBaseType_t uxTaskNumber 					= ( UBaseType_t ) 0U;
PRIVILEGED_DATA static volatile TickType_t xNextTaskUnblockTime		= ( TickType_t ) 0U;
PRIVILEGED_DATA static TaskHandle_t xIdleTaskHandle					= NULL;


PRIVILEGED_DATA static volatile UBaseType_t uxSchedulerSuspended	= ( UBaseType_t ) pdFALSE;

#if ( configGENERATE_RUN_TIME_STATS == 1 )

	PRIVILEGED_DATA static uint32_t ulTaskSwitchedInTime = 0UL;
	PRIVILEGED_DATA static uint32_t ulTotalRunTime = 0UL;

#endif


#if(  configCHECK_FOR_STACK_OVERFLOW > 0 )
	extern void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName );
#endif

#if( configUSE_TICK_HOOK > 0 )
	extern void vApplicationTickHook( void );
#endif

#if( configSUPPORT_STATIC_ALLOCATION == 1 )
	extern void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );
#endif


#if ( INCLUDE_vTaskSuspend == 1 )
	static BaseType_t prvTaskIsTaskSuspended( const TaskHandle_t xTask ) PRIVILEGED_FUNCTION;
#endif


static void prvInitialiseTaskLists( void ) PRIVILEGED_FUNCTION;


static portTASK_FUNCTION_PROTO( prvIdleTask, pvParameters );


#if ( INCLUDE_vTaskDelete == 1 )

	static void prvDeleteTCB( TCB_t *pxTCB ) PRIVILEGED_FUNCTION;

#endif


static void prvCheckTasksWaitingTermination( void ) PRIVILEGED_FUNCTION;


static void prvAddCurrentTaskToDelayedList( TickType_t xTicksToWait, const BaseType_t xCanBlockIndefinitely ) PRIVILEGED_FUNCTION;


#if ( configUSE_TRACE_FACILITY == 1 )

	static UBaseType_t prvListTasksWithinSingleList( TaskStatus_t *pxTaskStatusArray, List_t *pxList, eTaskState eState ) PRIVILEGED_FUNCTION;

#endif


#if ( INCLUDE_xTaskGetHandle == 1 )

	static TCB_t *prvSearchForNameWithinSingleList( List_t *pxList, const char pcNameToQuery[] ) PRIVILEGED_FUNCTION;

#endif


#if ( ( configUSE_TRACE_FACILITY == 1 ) || ( INCLUDE_uxTaskGetStackHighWaterMark == 1 ) )

	static uint16_t prvTaskCheckFreeStackSpace( const uint8_t * pucStackByte ) PRIVILEGED_FUNCTION;

#endif


#if ( configUSE_TICKLESS_IDLE != 0 )

	static TickType_t prvGetExpectedIdleTime( void ) PRIVILEGED_FUNCTION;

#endif


static void prvResetNextTaskUnblockTime( void );

#if ( ( configUSE_TRACE_FACILITY == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) )


	static char *prvWriteNameToBuffer( char *pcBuffer, const char *pcTaskName ) PRIVILEGED_FUNCTION;

#endif


static void prvInitialiseNewTask( 	TaskFunction_t pxTaskCode,
									const char * const pcName,
									const uint32_t ulStackDepth,
									void * const pvParameters,
									UBaseType_t uxPriority,
									TaskHandle_t * const pxCreatedTask,
									TCB_t *pxNewTCB,
									const MemoryRegion_t * const xRegions ) PRIVILEGED_FUNCTION;


static void prvAddNewTaskToReadyList( TCB_t *pxNewTCB ) PRIVILEGED_FUNCTION;


#if( configSUPPORT_STATIC_ALLOCATION == 1 )

	TaskHandle_t xTaskCreateStatic(	TaskFunction_t pxTaskCode,
									const char * const pcName,
									const uint32_t ulStackDepth,
									void * const pvParameters,
									UBaseType_t uxPriority,
									StackType_t * const puxStackBuffer,
									StaticTask_t * const pxTaskBuffer )
	{
	TCB_t *pxNewTCB;
	TaskHandle_t xReturn;

		configASSERT( puxStackBuffer != NULL );
		configASSERT( pxTaskBuffer != NULL );

		if( ( pxTaskBuffer != NULL ) && ( puxStackBuffer != NULL ) )
		{

			pxNewTCB = ( TCB_t * ) pxTaskBuffer;
			pxNewTCB->pxStack = ( StackType_t * ) puxStackBuffer;

			#if( tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE != 0 )
			{

				pxNewTCB->ucStaticallyAllocated = tskSTATICALLY_ALLOCATED_STACK_AND_TCB;
			}
			#endif

			prvInitialiseNewTask( pxTaskCode, pcName, ulStackDepth, pvParameters, uxPriority, &xReturn, pxNewTCB, NULL );
			prvAddNewTaskToReadyList( pxNewTCB );
		}
		else
		{
			xReturn = NULL;
		}

		return xReturn;
	}

#endif


#if( portUSING_MPU_WRAPPERS == 1 )

	BaseType_t xTaskCreateRestricted( const TaskParameters_t * const pxTaskDefinition, TaskHandle_t *pxCreatedTask )
	{
	TCB_t *pxNewTCB;
	BaseType_t xReturn = errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;

		configASSERT( pxTaskDefinition->puxStackBuffer );

		if( pxTaskDefinition->puxStackBuffer != NULL )
		{

			pxNewTCB = ( TCB_t * ) pvPortMalloc( sizeof( TCB_t ) );

			if( pxNewTCB != NULL )
			{

				pxNewTCB->pxStack = pxTaskDefinition->puxStackBuffer;


				pxNewTCB->ucStaticallyAllocated = tskSTATICALLY_ALLOCATED_STACK_ONLY;

				prvInitialiseNewTask(	pxTaskDefinition->pvTaskCode,
										pxTaskDefinition->pcName,
										( uint32_t ) pxTaskDefinition->usStackDepth,
										pxTaskDefinition->pvParameters,
										pxTaskDefinition->uxPriority,
										pxCreatedTask, pxNewTCB,
										pxTaskDefinition->xRegions );

				prvAddNewTaskToReadyList( pxNewTCB );
				xReturn = pdPASS;
			}
		}

		return xReturn;
	}

#endif


#if( configSUPPORT_DYNAMIC_ALLOCATION == 1 )

	BaseType_t xTaskCreate(	TaskFunction_t pxTaskCode,
							const char * const pcName,
							const uint16_t usStackDepth,
							void * const pvParameters,
							UBaseType_t uxPriority,
							TaskHandle_t * const pxCreatedTask )
	{
	TCB_t *pxNewTCB;
	BaseType_t xReturn;


		#if( portSTACK_GROWTH > 0 )
		{

			pxNewTCB = ( TCB_t * ) pvPortMalloc( sizeof( TCB_t ) );

			if( pxNewTCB != NULL )
			{

				pxNewTCB->pxStack = ( StackType_t * ) pvPortMalloc( ( ( ( size_t ) usStackDepth ) * sizeof( StackType_t ) ) );

				if( pxNewTCB->pxStack == NULL )
				{

					vPortFree( pxNewTCB );
					pxNewTCB = NULL;
				}
			}
		}
		#else
		{
		StackType_t *pxStack;


			pxStack = ( StackType_t * ) pvPortMalloc( ( ( ( size_t ) usStackDepth ) * sizeof( StackType_t ) ) );

			if( pxStack != NULL )
			{

				pxNewTCB = ( TCB_t * ) pvPortMalloc( sizeof( TCB_t ) );

				if( pxNewTCB != NULL )
				{

					pxNewTCB->pxStack = pxStack;
				}
				else
				{

					vPortFree( pxStack );
				}
			}
			else
			{
				pxNewTCB = NULL;
			}
		}
		#endif

		if( pxNewTCB != NULL )
		{
			#if( tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE != 0 )
			{

				pxNewTCB->ucStaticallyAllocated = tskDYNAMICALLY_ALLOCATED_STACK_AND_TCB;
			}
			#endif

			prvInitialiseNewTask( pxTaskCode, pcName, ( uint32_t ) usStackDepth, pvParameters, uxPriority, pxCreatedTask, pxNewTCB, NULL );
			prvAddNewTaskToReadyList( pxNewTCB );
			xReturn = pdPASS;
		}
		else
		{
			xReturn = errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
		}

		return xReturn;
	}

#endif


static void prvInitialiseNewTask( 	TaskFunction_t pxTaskCode,
									const char * const pcName,
									const uint32_t ulStackDepth,
									void * const pvParameters,
									UBaseType_t uxPriority,
									TaskHandle_t * const pxCreatedTask,
									TCB_t *pxNewTCB,
									const MemoryRegion_t * const xRegions )
{
StackType_t *pxTopOfStack;
UBaseType_t x;

	#if( portUSING_MPU_WRAPPERS == 1 )

		BaseType_t xRunPrivileged;
		if( ( uxPriority & portPRIVILEGE_BIT ) != 0U )
		{
			xRunPrivileged = pdTRUE;
		}
		else
		{
			xRunPrivileged = pdFALSE;
		}
		uxPriority &= ~portPRIVILEGE_BIT;
	#endif


	#if( ( configCHECK_FOR_STACK_OVERFLOW > 1 ) || ( configUSE_TRACE_FACILITY == 1 ) || ( INCLUDE_uxTaskGetStackHighWaterMark == 1 ) )
	{

		( void ) memset( pxNewTCB->pxStack, ( int ) tskSTACK_FILL_BYTE, ( size_t ) ulStackDepth * sizeof( StackType_t ) );
	}
	#endif


	#if( portSTACK_GROWTH < 0 )
	{
		pxTopOfStack = pxNewTCB->pxStack + ( ulStackDepth - ( uint32_t ) 1 );
		pxTopOfStack = ( StackType_t * ) ( ( ( portPOINTER_SIZE_TYPE ) pxTopOfStack ) & ( ~( ( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK ) ) );


		configASSERT( ( ( ( portPOINTER_SIZE_TYPE ) pxTopOfStack & ( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK ) == 0UL ) );
	}
	#else
	{
		pxTopOfStack = pxNewTCB->pxStack;


		configASSERT( ( ( ( portPOINTER_SIZE_TYPE ) pxNewTCB->pxStack & ( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK ) == 0UL ) );


		pxNewTCB->pxEndOfStack = pxNewTCB->pxStack + ( ulStackDepth - ( uint32_t ) 1 );
	}
	#endif


	for( x = ( UBaseType_t ) 0; x < ( UBaseType_t ) configMAX_TASK_NAME_LEN; x++ )
	{
		pxNewTCB->pcTaskName[ x ] = pcName[ x ];


		if( pcName[ x ] == 0x00 )
		{
			break;
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}


	pxNewTCB->pcTaskName[ configMAX_TASK_NAME_LEN - 1 ] = '\0';


	if( uxPriority >= ( UBaseType_t ) configMAX_PRIORITIES )
	{
		uxPriority = ( UBaseType_t ) configMAX_PRIORITIES - ( UBaseType_t ) 1U;
	}
	else
	{
		mtCOVERAGE_TEST_MARKER();
	}

	pxNewTCB->uxPriority = uxPriority;
	#if ( configUSE_MUTEXES == 1 )
	{
		pxNewTCB->uxBasePriority = uxPriority;
		pxNewTCB->uxMutexesHeld = 0;
	}
	#endif

	vListInitialiseItem( &( pxNewTCB->xStateListItem ) );
	vListInitialiseItem( &( pxNewTCB->xEventListItem ) );


	listSET_LIST_ITEM_OWNER( &( pxNewTCB->xStateListItem ), pxNewTCB );


	listSET_LIST_ITEM_VALUE( &( pxNewTCB->xEventListItem ), ( TickType_t ) configMAX_PRIORITIES - ( TickType_t ) uxPriority );
	listSET_LIST_ITEM_OWNER( &( pxNewTCB->xEventListItem ), pxNewTCB );

	#if ( portCRITICAL_NESTING_IN_TCB == 1 )
	{
		pxNewTCB->uxCriticalNesting = ( UBaseType_t ) 0U;
	}
	#endif

	#if ( configUSE_APPLICATION_TASK_TAG == 1 )
	{
		pxNewTCB->pxTaskTag = NULL;
	}
	#endif

	#if ( configGENERATE_RUN_TIME_STATS == 1 )
	{
		pxNewTCB->ulRunTimeCounter = 0UL;
	}
	#endif

	#if ( portUSING_MPU_WRAPPERS == 1 )
	{
		vPortStoreTaskMPUSettings( &( pxNewTCB->xMPUSettings ), xRegions, pxNewTCB->pxStack, ulStackDepth );
	}
	#else
	{

		( void ) xRegions;
	}
	#endif

	#if( configNUM_THREAD_LOCAL_STORAGE_POINTERS != 0 )
	{
		for( x = 0; x < ( UBaseType_t ) configNUM_THREAD_LOCAL_STORAGE_POINTERS; x++ )
		{
			pxNewTCB->pvThreadLocalStoragePointers[ x ] = NULL;
		}
	}
	#endif

	#if ( configUSE_TASK_NOTIFICATIONS == 1 )
	{
		pxNewTCB->ulNotifiedValue = 0;
		pxNewTCB->ucNotifyState = taskNOT_WAITING_NOTIFICATION;
	}
	#endif

	#if ( configUSE_NEWLIB_REENTRANT == 1 )
	{

		_REENT_INIT_PTR( ( &( pxNewTCB->xNewLib_reent ) ) );
	}
	#endif

	#if( INCLUDE_xTaskAbortDelay == 1 )
	{
		pxNewTCB->ucDelayAborted = pdFALSE;
	}
	#endif


	#if( portUSING_MPU_WRAPPERS == 1 )
	{
		pxNewTCB->pxTopOfStack = pxPortInitialiseStack( pxTopOfStack, pxTaskCode, pvParameters, xRunPrivileged );
	}
	#else
	{
		pxNewTCB->pxTopOfStack = pxPortInitialiseStack( pxTopOfStack, pxTaskCode, pvParameters );
	}
	#endif

	if( ( void * ) pxCreatedTask != NULL )
	{

		*pxCreatedTask = ( TaskHandle_t ) pxNewTCB;
	}
	else
	{
		mtCOVERAGE_TEST_MARKER();
	}
}


static void prvAddNewTaskToReadyList( TCB_t *pxNewTCB )
{

	taskENTER_CRITICAL();
	{
		uxCurrentNumberOfTasks++;
		if( pxCurrentTCB == NULL )
		{

			pxCurrentTCB = pxNewTCB;

			if( uxCurrentNumberOfTasks == ( UBaseType_t ) 1 )
			{

				prvInitialiseTaskLists();
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		else
		{

			if( xSchedulerRunning == pdFALSE )
			{
				if( pxCurrentTCB->uxPriority <= pxNewTCB->uxPriority )
				{
					pxCurrentTCB = pxNewTCB;
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}

		uxTaskNumber++;

		#if ( configUSE_TRACE_FACILITY == 1 )
		{

			pxNewTCB->uxTCBNumber = uxTaskNumber;
		}
		#endif
		traceTASK_CREATE( pxNewTCB );

		prvAddTaskToReadyList( pxNewTCB );

		portSETUP_TCB( pxNewTCB );
	}
	taskEXIT_CRITICAL();

	if( xSchedulerRunning != pdFALSE )
	{

		if( pxCurrentTCB->uxPriority < pxNewTCB->uxPriority )
		{
			taskYIELD_IF_USING_PREEMPTION();
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}
	else
	{
		mtCOVERAGE_TEST_MARKER();
	}
}


#if ( INCLUDE_vTaskDelete == 1 )

	void vTaskDelete( TaskHandle_t xTaskToDelete )
	{
	TCB_t *pxTCB;

		taskENTER_CRITICAL();
		{

			pxTCB = prvGetTCBFromHandle( xTaskToDelete );


			if( uxListRemove( &( pxTCB->xStateListItem ) ) == ( UBaseType_t ) 0 )
			{
				taskRESET_READY_PRIORITY( pxTCB->uxPriority );
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}


			if( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) != NULL )
			{
				( void ) uxListRemove( &( pxTCB->xEventListItem ) );
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}


			uxTaskNumber++;

			if( pxTCB == pxCurrentTCB )
			{

				vListInsertEnd( &xTasksWaitingTermination, &( pxTCB->xStateListItem ) );


				++uxDeletedTasksWaitingCleanUp;


				portPRE_TASK_DELETE_HOOK( pxTCB, &xYieldPending );
			}
			else
			{
				--uxCurrentNumberOfTasks;
				prvDeleteTCB( pxTCB );


				prvResetNextTaskUnblockTime();
			}

			traceTASK_DELETE( pxTCB );
		}
		taskEXIT_CRITICAL();


		if( xSchedulerRunning != pdFALSE )
		{
			if( pxTCB == pxCurrentTCB )
			{
				configASSERT( uxSchedulerSuspended == 0 );
				portYIELD_WITHIN_API();
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
	}

#endif


#if ( INCLUDE_vTaskDelayUntil == 1 )

	void vTaskDelayUntil( TickType_t * const pxPreviousWakeTime, const TickType_t xTimeIncrement )
	{
	TickType_t xTimeToWake;
	BaseType_t xAlreadyYielded, xShouldDelay = pdFALSE;

		configASSERT( pxPreviousWakeTime );
		configASSERT( ( xTimeIncrement > 0U ) );
		configASSERT( uxSchedulerSuspended == 0 );

		vTaskSuspendAll();
		{

			const TickType_t xConstTickCount = xTickCount;


			xTimeToWake = *pxPreviousWakeTime + xTimeIncrement;

			if( xConstTickCount < *pxPreviousWakeTime )
			{

				if( ( xTimeToWake < *pxPreviousWakeTime ) && ( xTimeToWake > xConstTickCount ) )
				{
					xShouldDelay = pdTRUE;
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{

				if( ( xTimeToWake < *pxPreviousWakeTime ) || ( xTimeToWake > xConstTickCount ) )
				{
					xShouldDelay = pdTRUE;
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}


			*pxPreviousWakeTime = xTimeToWake;

			if( xShouldDelay != pdFALSE )
			{
				traceTASK_DELAY_UNTIL( xTimeToWake );


				prvAddCurrentTaskToDelayedList( xTimeToWake - xConstTickCount, pdFALSE );
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		xAlreadyYielded = xTaskResumeAll();


		if( xAlreadyYielded == pdFALSE )
		{
			portYIELD_WITHIN_API();
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}

#endif


#if ( INCLUDE_vTaskDelay == 1 )

	void vTaskDelay( const TickType_t xTicksToDelay )
	{
	BaseType_t xAlreadyYielded = pdFALSE;


		if( xTicksToDelay > ( TickType_t ) 0U )
		{
			configASSERT( uxSchedulerSuspended == 0 );
			vTaskSuspendAll();
			{
				traceTASK_DELAY();


				prvAddCurrentTaskToDelayedList( xTicksToDelay, pdFALSE );
			}
			xAlreadyYielded = xTaskResumeAll();
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}


		if( xAlreadyYielded == pdFALSE )
		{
			portYIELD_WITHIN_API();
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}

#endif


#if( ( INCLUDE_eTaskGetState == 1 ) || ( configUSE_TRACE_FACILITY == 1 ) )

	eTaskState eTaskGetState( TaskHandle_t xTask )
	{
	eTaskState eReturn;
	List_t *pxStateList;
	const TCB_t * const pxTCB = ( TCB_t * ) xTask;

		configASSERT( pxTCB );

		if( pxTCB == pxCurrentTCB )
		{

			eReturn = eRunning;
		}
		else
		{
			taskENTER_CRITICAL();
			{
				pxStateList = ( List_t * ) listLIST_ITEM_CONTAINER( &( pxTCB->xStateListItem ) );
			}
			taskEXIT_CRITICAL();

			if( ( pxStateList == pxDelayedTaskList ) || ( pxStateList == pxOverflowDelayedTaskList ) )
			{

				eReturn = eBlocked;
			}

			#if ( INCLUDE_vTaskSuspend == 1 )
				else if( pxStateList == &xSuspendedTaskList )
				{

					if( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) == NULL )
					{
						eReturn = eSuspended;
					}
					else
					{
						eReturn = eBlocked;
					}
				}
			#endif

			#if ( INCLUDE_vTaskDelete == 1 )
				else if( ( pxStateList == &xTasksWaitingTermination ) || ( pxStateList == NULL ) )
				{

					eReturn = eDeleted;
				}
			#endif

			else
			{

				eReturn = eReady;
			}
		}

		return eReturn;
	}

#endif


#if ( INCLUDE_uxTaskPriorityGet == 1 )

	UBaseType_t uxTaskPriorityGet( TaskHandle_t xTask )
	{
	TCB_t *pxTCB;
	UBaseType_t uxReturn;

		taskENTER_CRITICAL();
		{

			pxTCB = prvGetTCBFromHandle( xTask );
			uxReturn = pxTCB->uxPriority;
		}
		taskEXIT_CRITICAL();

		return uxReturn;
	}

#endif


#if ( INCLUDE_uxTaskPriorityGet == 1 )

	UBaseType_t uxTaskPriorityGetFromISR( TaskHandle_t xTask )
	{
	TCB_t *pxTCB;
	UBaseType_t uxReturn, uxSavedInterruptState;


		portASSERT_IF_INTERRUPT_PRIORITY_INVALID();

		uxSavedInterruptState = portSET_INTERRUPT_MASK_FROM_ISR();
		{

			pxTCB = prvGetTCBFromHandle( xTask );
			uxReturn = pxTCB->uxPriority;
		}
		portCLEAR_INTERRUPT_MASK_FROM_ISR( uxSavedInterruptState );

		return uxReturn;
	}

#endif


#if ( INCLUDE_vTaskPrioritySet == 1 )

	void vTaskPrioritySet( TaskHandle_t xTask, UBaseType_t uxNewPriority )
	{
	TCB_t *pxTCB;
	UBaseType_t uxCurrentBasePriority, uxPriorityUsedOnEntry;
	BaseType_t xYieldRequired = pdFALSE;

		configASSERT( ( uxNewPriority < configMAX_PRIORITIES ) );


		if( uxNewPriority >= ( UBaseType_t ) configMAX_PRIORITIES )
		{
			uxNewPriority = ( UBaseType_t ) configMAX_PRIORITIES - ( UBaseType_t ) 1U;
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}

		taskENTER_CRITICAL();
		{

			pxTCB = prvGetTCBFromHandle( xTask );

			traceTASK_PRIORITY_SET( pxTCB, uxNewPriority );

			#if ( configUSE_MUTEXES == 1 )
			{
				uxCurrentBasePriority = pxTCB->uxBasePriority;
			}
			#else
			{
				uxCurrentBasePriority = pxTCB->uxPriority;
			}
			#endif

			if( uxCurrentBasePriority != uxNewPriority )
			{

				if( uxNewPriority > uxCurrentBasePriority )
				{
					if( pxTCB != pxCurrentTCB )
					{

						if( uxNewPriority >= pxCurrentTCB->uxPriority )
						{
							xYieldRequired = pdTRUE;
						}
						else
						{
							mtCOVERAGE_TEST_MARKER();
						}
					}
					else
					{

					}
				}
				else if( pxTCB == pxCurrentTCB )
				{

					xYieldRequired = pdTRUE;
				}
				else
				{

				}


				uxPriorityUsedOnEntry = pxTCB->uxPriority;

				#if ( configUSE_MUTEXES == 1 )
				{

					if( pxTCB->uxBasePriority == pxTCB->uxPriority )
					{
						pxTCB->uxPriority = uxNewPriority;
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}


					pxTCB->uxBasePriority = uxNewPriority;
				}
				#else
				{
					pxTCB->uxPriority = uxNewPriority;
				}
				#endif


				if( ( listGET_LIST_ITEM_VALUE( &( pxTCB->xEventListItem ) ) & taskEVENT_LIST_ITEM_VALUE_IN_USE ) == 0UL )
				{
					listSET_LIST_ITEM_VALUE( &( pxTCB->xEventListItem ), ( ( TickType_t ) configMAX_PRIORITIES - ( TickType_t ) uxNewPriority ) );
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}


				if( listIS_CONTAINED_WITHIN( &( pxReadyTasksLists[ uxPriorityUsedOnEntry ] ), &( pxTCB->xStateListItem ) ) != pdFALSE )
				{

					if( uxListRemove( &( pxTCB->xStateListItem ) ) == ( UBaseType_t ) 0 )
					{

						portRESET_READY_PRIORITY( uxPriorityUsedOnEntry, uxTopReadyPriority );
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}
					prvAddTaskToReadyList( pxTCB );
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}

				if( xYieldRequired != pdFALSE )
				{
					taskYIELD_IF_USING_PREEMPTION();
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}


				( void ) uxPriorityUsedOnEntry;
			}
		}
		taskEXIT_CRITICAL();
	}

#endif


#if ( INCLUDE_vTaskSuspend == 1 )

	void vTaskSuspend( TaskHandle_t xTaskToSuspend )
	{
	TCB_t *pxTCB;

		taskENTER_CRITICAL();
		{

			pxTCB = prvGetTCBFromHandle( xTaskToSuspend );

			traceTASK_SUSPEND( pxTCB );


			if( uxListRemove( &( pxTCB->xStateListItem ) ) == ( UBaseType_t ) 0 )
			{
				taskRESET_READY_PRIORITY( pxTCB->uxPriority );
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}


			if( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) != NULL )
			{
				( void ) uxListRemove( &( pxTCB->xEventListItem ) );
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}

			vListInsertEnd( &xSuspendedTaskList, &( pxTCB->xStateListItem ) );
		}
		taskEXIT_CRITICAL();

		if( xSchedulerRunning != pdFALSE )
		{

			taskENTER_CRITICAL();
			{
				prvResetNextTaskUnblockTime();
			}
			taskEXIT_CRITICAL();
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}

		if( pxTCB == pxCurrentTCB )
		{
			if( xSchedulerRunning != pdFALSE )
			{

				configASSERT( uxSchedulerSuspended == 0 );
				portYIELD_WITHIN_API();
			}
			else
			{

				if( listCURRENT_LIST_LENGTH( &xSuspendedTaskList ) == uxCurrentNumberOfTasks )
				{

					pxCurrentTCB = NULL;
				}
				else
				{
					vTaskSwitchContext();
				}
			}
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}

#endif


#if ( INCLUDE_vTaskSuspend == 1 )

	static BaseType_t prvTaskIsTaskSuspended( const TaskHandle_t xTask )
	{
	BaseType_t xReturn = pdFALSE;
	const TCB_t * const pxTCB = ( TCB_t * ) xTask;


		configASSERT( xTask );


		if( listIS_CONTAINED_WITHIN( &xSuspendedTaskList, &( pxTCB->xStateListItem ) ) != pdFALSE )
		{

			if( listIS_CONTAINED_WITHIN( &xPendingReadyList, &( pxTCB->xEventListItem ) ) == pdFALSE )
			{

				if( listIS_CONTAINED_WITHIN( NULL, &( pxTCB->xEventListItem ) ) != pdFALSE )
				{
					xReturn = pdTRUE;
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}

		return xReturn;
	}

#endif


#if ( INCLUDE_vTaskSuspend == 1 )

	void vTaskResume( TaskHandle_t xTaskToResume )
	{
	TCB_t * const pxTCB = ( TCB_t * ) xTaskToResume;


		configASSERT( xTaskToResume );


		if( ( pxTCB != NULL ) && ( pxTCB != pxCurrentTCB ) )
		{
			taskENTER_CRITICAL();
			{
				if( prvTaskIsTaskSuspended( pxTCB ) != pdFALSE )
				{
					traceTASK_RESUME( pxTCB );


					( void ) uxListRemove(  &( pxTCB->xStateListItem ) );
					prvAddTaskToReadyList( pxTCB );


					if( pxTCB->uxPriority >= pxCurrentTCB->uxPriority )
					{

						taskYIELD_IF_USING_PREEMPTION();
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			taskEXIT_CRITICAL();
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}

#endif


#if ( ( INCLUDE_xTaskResumeFromISR == 1 ) && ( INCLUDE_vTaskSuspend == 1 ) )

	BaseType_t xTaskResumeFromISR( TaskHandle_t xTaskToResume )
	{
	BaseType_t xYieldRequired = pdFALSE;
	TCB_t * const pxTCB = ( TCB_t * ) xTaskToResume;
	UBaseType_t uxSavedInterruptStatus;

		configASSERT( xTaskToResume );


		portASSERT_IF_INTERRUPT_PRIORITY_INVALID();

		uxSavedInterruptStatus = portSET_INTERRUPT_MASK_FROM_ISR();
		{
			if( prvTaskIsTaskSuspended( pxTCB ) != pdFALSE )
			{
				traceTASK_RESUME_FROM_ISR( pxTCB );


				if( uxSchedulerSuspended == ( UBaseType_t ) pdFALSE )
				{

					if( pxTCB->uxPriority >= pxCurrentTCB->uxPriority )
					{
						xYieldRequired = pdTRUE;
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}

					( void ) uxListRemove( &( pxTCB->xStateListItem ) );
					prvAddTaskToReadyList( pxTCB );
				}
				else
				{

					vListInsertEnd( &( xPendingReadyList ), &( pxTCB->xEventListItem ) );
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		portCLEAR_INTERRUPT_MASK_FROM_ISR( uxSavedInterruptStatus );

		return xYieldRequired;
	}

#endif


void vTaskStartScheduler( void )
{
BaseType_t xReturn;


	#if( configSUPPORT_STATIC_ALLOCATION == 1 )
	{
		StaticTask_t *pxIdleTaskTCBBuffer = NULL;
		StackType_t *pxIdleTaskStackBuffer = NULL;
		uint32_t ulIdleTaskStackSize;


		vApplicationGetIdleTaskMemory( &pxIdleTaskTCBBuffer, &pxIdleTaskStackBuffer, &ulIdleTaskStackSize );
		xIdleTaskHandle = xTaskCreateStatic(	prvIdleTask,
												"IDLE",
												ulIdleTaskStackSize,
												( void * ) NULL,
												( tskIDLE_PRIORITY | portPRIVILEGE_BIT ),
												pxIdleTaskStackBuffer,
												pxIdleTaskTCBBuffer );

		if( xIdleTaskHandle != NULL )
		{
			xReturn = pdPASS;
		}
		else
		{
			xReturn = pdFAIL;
		}
	}
	#else
	{

		xReturn = xTaskCreate(	prvIdleTask,
								"IDLE", configMINIMAL_STACK_SIZE,
								( void * ) NULL,
								( tskIDLE_PRIORITY | portPRIVILEGE_BIT ),
								&xIdleTaskHandle );
	}
	#endif

	#if ( configUSE_TIMERS == 1 )
	{
		if( xReturn == pdPASS )
		{
			xReturn = xTimerCreateTimerTask();
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}
	#endif

	if( xReturn == pdPASS )
	{

		portDISABLE_INTERRUPTS();

		#if ( configUSE_NEWLIB_REENTRANT == 1 )
		{

			_impure_ptr = &( pxCurrentTCB->xNewLib_reent );
		}
		#endif

		xNextTaskUnblockTime = portMAX_DELAY;
		xSchedulerRunning = pdTRUE;
		xTickCount = ( TickType_t ) 0U;


		portCONFIGURE_TIMER_FOR_RUN_TIME_STATS();


		if( xPortStartScheduler() != pdFALSE )
		{

		}
		else
		{

		}
	}
	else
	{

		configASSERT( xReturn != errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY );
	}


	( void ) xIdleTaskHandle;
}


void vTaskEndScheduler( void )
{

	portDISABLE_INTERRUPTS();
	xSchedulerRunning = pdFALSE;
	vPortEndScheduler();
}


void vTaskSuspendAll( void )
{

	++uxSchedulerSuspended;
}


#if ( configUSE_TICKLESS_IDLE != 0 )

	static TickType_t prvGetExpectedIdleTime( void )
	{
	TickType_t xReturn;
	UBaseType_t uxHigherPriorityReadyTasks = pdFALSE;


		#if( configUSE_PORT_OPTIMISED_TASK_SELECTION == 0 )
		{
			if( uxTopReadyPriority > tskIDLE_PRIORITY )
			{
				uxHigherPriorityReadyTasks = pdTRUE;
			}
		}
		#else
		{
			const UBaseType_t uxLeastSignificantBit = ( UBaseType_t ) 0x01;


			if( uxTopReadyPriority > uxLeastSignificantBit )
			{
				uxHigherPriorityReadyTasks = pdTRUE;
			}
		}
		#endif

		if( pxCurrentTCB->uxPriority > tskIDLE_PRIORITY )
		{
			xReturn = 0;
		}
		else if( listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[ tskIDLE_PRIORITY ] ) ) > 1 )
		{

			xReturn = 0;
		}
		else if( uxHigherPriorityReadyTasks != pdFALSE )
		{

			xReturn = 0;
		}
		else
		{
			xReturn = xNextTaskUnblockTime - xTickCount;
		}

		return xReturn;
	}

#endif


BaseType_t xTaskResumeAll( void )
{
TCB_t *pxTCB = NULL;
BaseType_t xAlreadyYielded = pdFALSE;


	configASSERT( uxSchedulerSuspended );


	taskENTER_CRITICAL();
	{
		--uxSchedulerSuspended;

		if( uxSchedulerSuspended == ( UBaseType_t ) pdFALSE )
		{
			if( uxCurrentNumberOfTasks > ( UBaseType_t ) 0U )
			{

				while( listLIST_IS_EMPTY( &xPendingReadyList ) == pdFALSE )
				{
					pxTCB = ( TCB_t * ) listGET_OWNER_OF_HEAD_ENTRY( ( &xPendingReadyList ) );
					( void ) uxListRemove( &( pxTCB->xEventListItem ) );
					( void ) uxListRemove( &( pxTCB->xStateListItem ) );
					prvAddTaskToReadyList( pxTCB );


					if( pxTCB->uxPriority >= pxCurrentTCB->uxPriority )
					{
						xYieldPending = pdTRUE;
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}
				}

				if( pxTCB != NULL )
				{

					prvResetNextTaskUnblockTime();
				}


				{
					UBaseType_t uxPendedCounts = uxPendedTicks;

					if( uxPendedCounts > ( UBaseType_t ) 0U )
					{
						do
						{
							if( xTaskIncrementTick() != pdFALSE )
							{
								xYieldPending = pdTRUE;
							}
							else
							{
								mtCOVERAGE_TEST_MARKER();
							}
							--uxPendedCounts;
						} while( uxPendedCounts > ( UBaseType_t ) 0U );

						uxPendedTicks = 0;
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}
				}

				if( xYieldPending != pdFALSE )
				{
					#if( configUSE_PREEMPTION != 0 )
					{
						xAlreadyYielded = pdTRUE;
					}
					#endif
					taskYIELD_IF_USING_PREEMPTION();
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}
	taskEXIT_CRITICAL();

	return xAlreadyYielded;
}


TickType_t xTaskGetTickCount( void )
{
TickType_t xTicks;


	portTICK_TYPE_ENTER_CRITICAL();
	{
		xTicks = xTickCount;
	}
	portTICK_TYPE_EXIT_CRITICAL();

	return xTicks;
}


TickType_t xTaskGetTickCountFromISR( void )
{
TickType_t xReturn;
UBaseType_t uxSavedInterruptStatus;


	portASSERT_IF_INTERRUPT_PRIORITY_INVALID();

	uxSavedInterruptStatus = portTICK_TYPE_SET_INTERRUPT_MASK_FROM_ISR();
	{
		xReturn = xTickCount;
	}
	portTICK_TYPE_CLEAR_INTERRUPT_MASK_FROM_ISR( uxSavedInterruptStatus );

	return xReturn;
}


UBaseType_t uxTaskGetNumberOfTasks( void )
{

	return uxCurrentNumberOfTasks;
}


char *pcTaskGetName( TaskHandle_t xTaskToQuery )
{
TCB_t *pxTCB;


	pxTCB = prvGetTCBFromHandle( xTaskToQuery );
	configASSERT( pxTCB );
	return &( pxTCB->pcTaskName[ 0 ] );
}


#if ( INCLUDE_xTaskGetHandle == 1 )

	static TCB_t *prvSearchForNameWithinSingleList( List_t *pxList, const char pcNameToQuery[] )
	{
	TCB_t *pxNextTCB, *pxFirstTCB, *pxReturn = NULL;
	UBaseType_t x;
	char cNextChar;


		if( listCURRENT_LIST_LENGTH( pxList ) > ( UBaseType_t ) 0 )
		{
			listGET_OWNER_OF_NEXT_ENTRY( pxFirstTCB, pxList );

			do
			{
				listGET_OWNER_OF_NEXT_ENTRY( pxNextTCB, pxList );


				for( x = ( UBaseType_t ) 0; x < ( UBaseType_t ) configMAX_TASK_NAME_LEN; x++ )
				{
					cNextChar = pxNextTCB->pcTaskName[ x ];

					if( cNextChar != pcNameToQuery[ x ] )
					{

						break;
					}
					else if( cNextChar == 0x00 )
					{

						pxReturn = pxNextTCB;
						break;
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}
				}

				if( pxReturn != NULL )
				{

					break;
				}

			} while( pxNextTCB != pxFirstTCB );
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}

		return pxReturn;
	}

#endif


#if ( INCLUDE_xTaskGetHandle == 1 )

	TaskHandle_t xTaskGetHandle( const char *pcNameToQuery )
	{
	UBaseType_t uxQueue = configMAX_PRIORITIES;
	TCB_t* pxTCB;


		configASSERT( strlen( pcNameToQuery ) < configMAX_TASK_NAME_LEN );

		vTaskSuspendAll();
		{

			do
			{
				uxQueue--;
				pxTCB = prvSearchForNameWithinSingleList( ( List_t * ) &( pxReadyTasksLists[ uxQueue ] ), pcNameToQuery );

				if( pxTCB != NULL )
				{

					break;
				}

			} while( uxQueue > ( UBaseType_t ) tskIDLE_PRIORITY );


			if( pxTCB == NULL )
			{
				pxTCB = prvSearchForNameWithinSingleList( ( List_t * ) pxDelayedTaskList, pcNameToQuery );
			}

			if( pxTCB == NULL )
			{
				pxTCB = prvSearchForNameWithinSingleList( ( List_t * ) pxOverflowDelayedTaskList, pcNameToQuery );
			}

			#if ( INCLUDE_vTaskSuspend == 1 )
			{
				if( pxTCB == NULL )
				{

					pxTCB = prvSearchForNameWithinSingleList( &xSuspendedTaskList, pcNameToQuery );
				}
			}
			#endif

			#if( INCLUDE_vTaskDelete == 1 )
			{
				if( pxTCB == NULL )
				{

					pxTCB = prvSearchForNameWithinSingleList( &xTasksWaitingTermination, pcNameToQuery );
				}
			}
			#endif
		}
		( void ) xTaskResumeAll();

		return ( TaskHandle_t ) pxTCB;
	}

#endif


#if ( configUSE_TRACE_FACILITY == 1 )

	UBaseType_t uxTaskGetSystemState( TaskStatus_t * const pxTaskStatusArray, const UBaseType_t uxArraySize, uint32_t * const pulTotalRunTime )
	{
	UBaseType_t uxTask = 0, uxQueue = configMAX_PRIORITIES;

		vTaskSuspendAll();
		{

			if( uxArraySize >= uxCurrentNumberOfTasks )
			{

				do
				{
					uxQueue--;
					uxTask += prvListTasksWithinSingleList( &( pxTaskStatusArray[ uxTask ] ), &( pxReadyTasksLists[ uxQueue ] ), eReady );

				} while( uxQueue > ( UBaseType_t ) tskIDLE_PRIORITY );


				uxTask += prvListTasksWithinSingleList( &( pxTaskStatusArray[ uxTask ] ), ( List_t * ) pxDelayedTaskList, eBlocked );
				uxTask += prvListTasksWithinSingleList( &( pxTaskStatusArray[ uxTask ] ), ( List_t * ) pxOverflowDelayedTaskList, eBlocked );

				#if( INCLUDE_vTaskDelete == 1 )
				{

					uxTask += prvListTasksWithinSingleList( &( pxTaskStatusArray[ uxTask ] ), &xTasksWaitingTermination, eDeleted );
				}
				#endif

				#if ( INCLUDE_vTaskSuspend == 1 )
				{

					uxTask += prvListTasksWithinSingleList( &( pxTaskStatusArray[ uxTask ] ), &xSuspendedTaskList, eSuspended );
				}
				#endif

				#if ( configGENERATE_RUN_TIME_STATS == 1)
				{
					if( pulTotalRunTime != NULL )
					{
						#ifdef portALT_GET_RUN_TIME_COUNTER_VALUE
							portALT_GET_RUN_TIME_COUNTER_VALUE( ( *pulTotalRunTime ) );
						#else
							*pulTotalRunTime = portGET_RUN_TIME_COUNTER_VALUE();
						#endif
					}
				}
				#else
				{
					if( pulTotalRunTime != NULL )
					{
						*pulTotalRunTime = 0;
					}
				}
				#endif
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		( void ) xTaskResumeAll();

		return uxTask;
	}

#endif


#if ( INCLUDE_xTaskGetIdleTaskHandle == 1 )

	TaskHandle_t xTaskGetIdleTaskHandle( void )
	{

		configASSERT( ( xIdleTaskHandle != NULL ) );
		return xIdleTaskHandle;
	}

#endif


#if ( configUSE_TICKLESS_IDLE != 0 )

	void vTaskStepTick( const TickType_t xTicksToJump )
	{

		configASSERT( ( xTickCount + xTicksToJump ) <= xNextTaskUnblockTime );
		xTickCount += xTicksToJump;
		traceINCREASE_TICK_COUNT( xTicksToJump );
	}

#endif


#if ( INCLUDE_xTaskAbortDelay == 1 )

	BaseType_t xTaskAbortDelay( TaskHandle_t xTask )
	{
	TCB_t *pxTCB = ( TCB_t * ) xTask;
	BaseType_t xReturn = pdFALSE;

		configASSERT( pxTCB );

		vTaskSuspendAll();
		{

			if( eTaskGetState( xTask ) == eBlocked )
			{

				( void ) uxListRemove( &( pxTCB->xStateListItem ) );


				taskENTER_CRITICAL();
				{
					if( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) != NULL )
					{
						( void ) uxListRemove( &( pxTCB->xEventListItem ) );
						pxTCB->ucDelayAborted = pdTRUE;
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}
				}
				taskEXIT_CRITICAL();


				prvAddTaskToReadyList( pxTCB );


				#if (  configUSE_PREEMPTION == 1 )
				{

					if( pxTCB->uxPriority > pxCurrentTCB->uxPriority )
					{

						xYieldPending = pdTRUE;
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}
				}
				#endif
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		xTaskResumeAll();

		return xReturn;
	}

#endif


BaseType_t xTaskIncrementTick( void )
{
TCB_t * pxTCB;
TickType_t xItemValue;
BaseType_t xSwitchRequired = pdFALSE;


	traceTASK_INCREMENT_TICK( xTickCount );
	if( uxSchedulerSuspended == ( UBaseType_t ) pdFALSE )
	{

		const TickType_t xConstTickCount = xTickCount + 1;


		xTickCount = xConstTickCount;

		if( xConstTickCount == ( TickType_t ) 0U )
		{
			taskSWITCH_DELAYED_LISTS();
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}


		if( xConstTickCount >= xNextTaskUnblockTime )
		{
			for( ;; )
			{
				if( listLIST_IS_EMPTY( pxDelayedTaskList ) != pdFALSE )
				{

					xNextTaskUnblockTime = portMAX_DELAY;
					break;
				}
				else
				{

					pxTCB = ( TCB_t * ) listGET_OWNER_OF_HEAD_ENTRY( pxDelayedTaskList );
					xItemValue = listGET_LIST_ITEM_VALUE( &( pxTCB->xStateListItem ) );

					if( xConstTickCount < xItemValue )
					{

						xNextTaskUnblockTime = xItemValue;
						break;
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}


					( void ) uxListRemove( &( pxTCB->xStateListItem ) );


					if( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) != NULL )
					{
						( void ) uxListRemove( &( pxTCB->xEventListItem ) );
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}


					prvAddTaskToReadyList( pxTCB );


					#if (  configUSE_PREEMPTION == 1 )
					{

						if( pxTCB->uxPriority >= pxCurrentTCB->uxPriority )
						{
							xSwitchRequired = pdTRUE;
						}
						else
						{
							mtCOVERAGE_TEST_MARKER();
						}
					}
					#endif
				}
			}
		}


		#if ( ( configUSE_PREEMPTION == 1 ) && ( configUSE_TIME_SLICING == 1 ) )
		{
			if( listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[ pxCurrentTCB->uxPriority ] ) ) > ( UBaseType_t ) 1 )
			{
				xSwitchRequired = pdTRUE;
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		#endif

		#if ( configUSE_TICK_HOOK == 1 )
		{

			if( uxPendedTicks == ( UBaseType_t ) 0U )
			{
				vApplicationTickHook();
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		#endif
	}
	else
	{
		++uxPendedTicks;


		#if ( configUSE_TICK_HOOK == 1 )
		{
			vApplicationTickHook();
		}
		#endif
	}

	#if ( configUSE_PREEMPTION == 1 )
	{
		if( xYieldPending != pdFALSE )
		{
			xSwitchRequired = pdTRUE;
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}
	#endif

	return xSwitchRequired;
}


#if ( configUSE_APPLICATION_TASK_TAG == 1 )

	void vTaskSetApplicationTaskTag( TaskHandle_t xTask, TaskHookFunction_t pxHookFunction )
	{
	TCB_t *xTCB;


		if( xTask == NULL )
		{
			xTCB = ( TCB_t * ) pxCurrentTCB;
		}
		else
		{
			xTCB = ( TCB_t * ) xTask;
		}


		taskENTER_CRITICAL();
			xTCB->pxTaskTag = pxHookFunction;
		taskEXIT_CRITICAL();
	}

#endif


#if ( configUSE_APPLICATION_TASK_TAG == 1 )

	TaskHookFunction_t xTaskGetApplicationTaskTag( TaskHandle_t xTask )
	{
	TCB_t *xTCB;
	TaskHookFunction_t xReturn;


		if( xTask == NULL )
		{
			xTCB = ( TCB_t * ) pxCurrentTCB;
		}
		else
		{
			xTCB = ( TCB_t * ) xTask;
		}


		taskENTER_CRITICAL();
		{
			xReturn = xTCB->pxTaskTag;
		}
		taskEXIT_CRITICAL();

		return xReturn;
	}

#endif


#if ( configUSE_APPLICATION_TASK_TAG == 1 )

	BaseType_t xTaskCallApplicationTaskHook( TaskHandle_t xTask, void *pvParameter )
	{
	TCB_t *xTCB;
	BaseType_t xReturn;


		if( xTask == NULL )
		{
			xTCB = ( TCB_t * ) pxCurrentTCB;
		}
		else
		{
			xTCB = ( TCB_t * ) xTask;
		}

		if( xTCB->pxTaskTag != NULL )
		{
			xReturn = xTCB->pxTaskTag( pvParameter );
		}
		else
		{
			xReturn = pdFAIL;
		}

		return xReturn;
	}

#endif


void vTaskSwitchContext( void )
{
	if( uxSchedulerSuspended != ( UBaseType_t ) pdFALSE )
	{

		xYieldPending = pdTRUE;
	}
	else
	{
		xYieldPending = pdFALSE;
		traceTASK_SWITCHED_OUT();

		#if ( configGENERATE_RUN_TIME_STATS == 1 )
		{
				#ifdef portALT_GET_RUN_TIME_COUNTER_VALUE
					portALT_GET_RUN_TIME_COUNTER_VALUE( ulTotalRunTime );
				#else
					ulTotalRunTime = portGET_RUN_TIME_COUNTER_VALUE();
				#endif


				if( ulTotalRunTime > ulTaskSwitchedInTime )
				{
					pxCurrentTCB->ulRunTimeCounter += ( ulTotalRunTime - ulTaskSwitchedInTime );
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
				ulTaskSwitchedInTime = ulTotalRunTime;
		}
		#endif


		taskCHECK_FOR_STACK_OVERFLOW();


		taskSELECT_HIGHEST_PRIORITY_TASK();
		traceTASK_SWITCHED_IN();

		#if ( configUSE_NEWLIB_REENTRANT == 1 )
		{

			_impure_ptr = &( pxCurrentTCB->xNewLib_reent );
		}
		#endif
	}
}


void vTaskPlaceOnEventList( List_t * const pxEventList, const TickType_t xTicksToWait )
{
	configASSERT( pxEventList );


	vListInsert( pxEventList, &( pxCurrentTCB->xEventListItem ) );

	prvAddCurrentTaskToDelayedList( xTicksToWait, pdTRUE );
}


void vTaskPlaceOnUnorderedEventList( List_t * pxEventList, const TickType_t xItemValue, const TickType_t xTicksToWait )
{
	configASSERT( pxEventList );


	configASSERT( uxSchedulerSuspended != 0 );


	listSET_LIST_ITEM_VALUE( &( pxCurrentTCB->xEventListItem ), xItemValue | taskEVENT_LIST_ITEM_VALUE_IN_USE );


	vListInsertEnd( pxEventList, &( pxCurrentTCB->xEventListItem ) );

	prvAddCurrentTaskToDelayedList( xTicksToWait, pdTRUE );
}


#if( configUSE_TIMERS == 1 )

	void vTaskPlaceOnEventListRestricted( List_t * const pxEventList, TickType_t xTicksToWait, const BaseType_t xWaitIndefinitely )
	{
		configASSERT( pxEventList );


		vListInsertEnd( pxEventList, &( pxCurrentTCB->xEventListItem ) );


		if( xWaitIndefinitely != pdFALSE )
		{
			xTicksToWait = portMAX_DELAY;
		}

		traceTASK_DELAY_UNTIL( ( xTickCount + xTicksToWait ) );
		prvAddCurrentTaskToDelayedList( xTicksToWait, xWaitIndefinitely );
	}

#endif


BaseType_t xTaskRemoveFromEventList( const List_t * const pxEventList )
{
TCB_t *pxUnblockedTCB;
BaseType_t xReturn;


	pxUnblockedTCB = ( TCB_t * ) listGET_OWNER_OF_HEAD_ENTRY( pxEventList );
	configASSERT( pxUnblockedTCB );
	( void ) uxListRemove( &( pxUnblockedTCB->xEventListItem ) );

	if( uxSchedulerSuspended == ( UBaseType_t ) pdFALSE )
	{
		( void ) uxListRemove( &( pxUnblockedTCB->xStateListItem ) );
		prvAddTaskToReadyList( pxUnblockedTCB );
	}
	else
	{

		vListInsertEnd( &( xPendingReadyList ), &( pxUnblockedTCB->xEventListItem ) );
	}

	if( pxUnblockedTCB->uxPriority > pxCurrentTCB->uxPriority )
	{

		xReturn = pdTRUE;


		xYieldPending = pdTRUE;
	}
	else
	{
		xReturn = pdFALSE;
	}

	#if( configUSE_TICKLESS_IDLE != 0 )
	{

		prvResetNextTaskUnblockTime();
	}
	#endif

	return xReturn;
}


BaseType_t xTaskRemoveFromUnorderedEventList( ListItem_t * pxEventListItem, const TickType_t xItemValue )
{
TCB_t *pxUnblockedTCB;
BaseType_t xReturn;


	configASSERT( uxSchedulerSuspended != pdFALSE );


	listSET_LIST_ITEM_VALUE( pxEventListItem, xItemValue | taskEVENT_LIST_ITEM_VALUE_IN_USE );


	pxUnblockedTCB = ( TCB_t * ) listGET_LIST_ITEM_OWNER( pxEventListItem );
	configASSERT( pxUnblockedTCB );
	( void ) uxListRemove( pxEventListItem );


	( void ) uxListRemove( &( pxUnblockedTCB->xStateListItem ) );
	prvAddTaskToReadyList( pxUnblockedTCB );

	if( pxUnblockedTCB->uxPriority > pxCurrentTCB->uxPriority )
	{

		xReturn = pdTRUE;


		xYieldPending = pdTRUE;
	}
	else
	{
		xReturn = pdFALSE;
	}

	return xReturn;
}


void vTaskSetTimeOutState( TimeOut_t * const pxTimeOut )
{
	configASSERT( pxTimeOut );
	pxTimeOut->xOverflowCount = xNumOfOverflows;
	pxTimeOut->xTimeOnEntering = xTickCount;
}


BaseType_t xTaskCheckForTimeOut( TimeOut_t * const pxTimeOut, TickType_t * const pxTicksToWait )
{
BaseType_t xReturn;

	configASSERT( pxTimeOut );
	configASSERT( pxTicksToWait );

	taskENTER_CRITICAL();
	{

		const TickType_t xConstTickCount = xTickCount;

		#if( INCLUDE_xTaskAbortDelay == 1 )
			if( pxCurrentTCB->ucDelayAborted != pdFALSE )
			{

				pxCurrentTCB->ucDelayAborted = pdFALSE;
				xReturn = pdTRUE;
			}
			else
		#endif

		#if ( INCLUDE_vTaskSuspend == 1 )
			if( *pxTicksToWait == portMAX_DELAY )
			{

				xReturn = pdFALSE;
			}
			else
		#endif

		if( ( xNumOfOverflows != pxTimeOut->xOverflowCount ) && ( xConstTickCount >= pxTimeOut->xTimeOnEntering ) )
		{

			xReturn = pdTRUE;
		}
		else if( ( ( TickType_t ) ( xConstTickCount - pxTimeOut->xTimeOnEntering ) ) < *pxTicksToWait )
		{

			*pxTicksToWait -= ( xConstTickCount - pxTimeOut->xTimeOnEntering );
			vTaskSetTimeOutState( pxTimeOut );
			xReturn = pdFALSE;
		}
		else
		{
			xReturn = pdTRUE;
		}
	}
	taskEXIT_CRITICAL();

	return xReturn;
}


void vTaskMissedYield( void )
{
	xYieldPending = pdTRUE;
}


#if ( configUSE_TRACE_FACILITY == 1 )

	UBaseType_t uxTaskGetTaskNumber( TaskHandle_t xTask )
	{
	UBaseType_t uxReturn;
	TCB_t *pxTCB;

		if( xTask != NULL )
		{
			pxTCB = ( TCB_t * ) xTask;
			uxReturn = pxTCB->uxTaskNumber;
		}
		else
		{
			uxReturn = 0U;
		}

		return uxReturn;
	}

#endif


#if ( configUSE_TRACE_FACILITY == 1 )

	void vTaskSetTaskNumber( TaskHandle_t xTask, const UBaseType_t uxHandle )
	{
	TCB_t *pxTCB;

		if( xTask != NULL )
		{
			pxTCB = ( TCB_t * ) xTask;
			pxTCB->uxTaskNumber = uxHandle;
		}
	}

#endif


static portTASK_FUNCTION( prvIdleTask, pvParameters )
{

	( void ) pvParameters;


	for( ;; )
	{

		prvCheckTasksWaitingTermination();

		#if ( configUSE_PREEMPTION == 0 )
		{

			taskYIELD();
		}
		#endif

		#if ( ( configUSE_PREEMPTION == 1 ) && ( configIDLE_SHOULD_YIELD == 1 ) )
		{

			if( listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[ tskIDLE_PRIORITY ] ) ) > ( UBaseType_t ) 1 )
			{
				taskYIELD();
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		#endif

		#if ( configUSE_IDLE_HOOK == 1 )
		{
			extern void vApplicationIdleHook( void );


			vApplicationIdleHook();
		}
		#endif


		#if ( configUSE_TICKLESS_IDLE != 0 )
		{
		TickType_t xExpectedIdleTime;


			xExpectedIdleTime = prvGetExpectedIdleTime();

			if( xExpectedIdleTime >= configEXPECTED_IDLE_TIME_BEFORE_SLEEP )
			{
				vTaskSuspendAll();
				{

					configASSERT( xNextTaskUnblockTime >= xTickCount );
					xExpectedIdleTime = prvGetExpectedIdleTime();

					if( xExpectedIdleTime >= configEXPECTED_IDLE_TIME_BEFORE_SLEEP )
					{
						traceLOW_POWER_IDLE_BEGIN();
						portSUPPRESS_TICKS_AND_SLEEP( xExpectedIdleTime );
						traceLOW_POWER_IDLE_END();
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}
				}
				( void ) xTaskResumeAll();
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		#endif
	}
}


#if( configUSE_TICKLESS_IDLE != 0 )

	eSleepModeStatus eTaskConfirmSleepModeStatus( void )
	{

	const UBaseType_t uxNonApplicationTasks = 1;
	eSleepModeStatus eReturn = eStandardSleep;

		if( listCURRENT_LIST_LENGTH( &xPendingReadyList ) != 0 )
		{

			eReturn = eAbortSleep;
		}
		else if( xYieldPending != pdFALSE )
		{

			eReturn = eAbortSleep;
		}
		else
		{

			if( listCURRENT_LIST_LENGTH( &xSuspendedTaskList ) == ( uxCurrentNumberOfTasks - uxNonApplicationTasks ) )
			{
				eReturn = eNoTasksWaitingTimeout;
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}

		return eReturn;
	}

#endif


#if ( configNUM_THREAD_LOCAL_STORAGE_POINTERS != 0 )

	void vTaskSetThreadLocalStoragePointer( TaskHandle_t xTaskToSet, BaseType_t xIndex, void *pvValue )
	{
	TCB_t *pxTCB;

		if( xIndex < configNUM_THREAD_LOCAL_STORAGE_POINTERS )
		{
			pxTCB = prvGetTCBFromHandle( xTaskToSet );
			pxTCB->pvThreadLocalStoragePointers[ xIndex ] = pvValue;
		}
	}

#endif


#if ( configNUM_THREAD_LOCAL_STORAGE_POINTERS != 0 )

	void *pvTaskGetThreadLocalStoragePointer( TaskHandle_t xTaskToQuery, BaseType_t xIndex )
	{
	void *pvReturn = NULL;
	TCB_t *pxTCB;

		if( xIndex < configNUM_THREAD_LOCAL_STORAGE_POINTERS )
		{
			pxTCB = prvGetTCBFromHandle( xTaskToQuery );
			pvReturn = pxTCB->pvThreadLocalStoragePointers[ xIndex ];
		}
		else
		{
			pvReturn = NULL;
		}

		return pvReturn;
	}

#endif


#if ( portUSING_MPU_WRAPPERS == 1 )

	void vTaskAllocateMPURegions( TaskHandle_t xTaskToModify, const MemoryRegion_t * const xRegions )
	{
	TCB_t *pxTCB;


		pxTCB = prvGetTCBFromHandle( xTaskToModify );

		vPortStoreTaskMPUSettings( &( pxTCB->xMPUSettings ), xRegions, NULL, 0 );
	}

#endif


static void prvInitialiseTaskLists( void )
{
UBaseType_t uxPriority;

	for( uxPriority = ( UBaseType_t ) 0U; uxPriority < ( UBaseType_t ) configMAX_PRIORITIES; uxPriority++ )
	{
		vListInitialise( &( pxReadyTasksLists[ uxPriority ] ) );
	}

	vListInitialise( &xDelayedTaskList1 );
	vListInitialise( &xDelayedTaskList2 );
	vListInitialise( &xPendingReadyList );

	#if ( INCLUDE_vTaskDelete == 1 )
	{
		vListInitialise( &xTasksWaitingTermination );
	}
	#endif

	#if ( INCLUDE_vTaskSuspend == 1 )
	{
		vListInitialise( &xSuspendedTaskList );
	}
	#endif


	pxDelayedTaskList = &xDelayedTaskList1;
	pxOverflowDelayedTaskList = &xDelayedTaskList2;
}


static void prvCheckTasksWaitingTermination( void )
{


	#if ( INCLUDE_vTaskDelete == 1 )
	{
		BaseType_t xListIsEmpty;


		while( uxDeletedTasksWaitingCleanUp > ( UBaseType_t ) 0U )
		{
			vTaskSuspendAll();
			{
				xListIsEmpty = listLIST_IS_EMPTY( &xTasksWaitingTermination );
			}
			( void ) xTaskResumeAll();

			if( xListIsEmpty == pdFALSE )
			{
				TCB_t *pxTCB;

				taskENTER_CRITICAL();
				{
					pxTCB = ( TCB_t * ) listGET_OWNER_OF_HEAD_ENTRY( ( &xTasksWaitingTermination ) );
					( void ) uxListRemove( &( pxTCB->xStateListItem ) );
					--uxCurrentNumberOfTasks;
					--uxDeletedTasksWaitingCleanUp;
				}
				taskEXIT_CRITICAL();

				prvDeleteTCB( pxTCB );
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
	}
	#endif
}


#if( configUSE_TRACE_FACILITY == 1 )

	void vTaskGetInfo( TaskHandle_t xTask, TaskStatus_t *pxTaskStatus, BaseType_t xGetFreeStackSpace, eTaskState eState )
	{
	TCB_t *pxTCB;


		pxTCB = prvGetTCBFromHandle( xTask );

		pxTaskStatus->xHandle = ( TaskHandle_t ) pxTCB;
		pxTaskStatus->pcTaskName = ( const char * ) &( pxTCB->pcTaskName [ 0 ] );
		pxTaskStatus->uxCurrentPriority = pxTCB->uxPriority;
		pxTaskStatus->pxStackBase = pxTCB->pxStack;
		pxTaskStatus->xTaskNumber = pxTCB->uxTCBNumber;

		#if ( INCLUDE_vTaskSuspend == 1 )
		{

			if( pxTaskStatus->eCurrentState == eSuspended )
			{
				vTaskSuspendAll();
				{
					if( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) != NULL )
					{
						pxTaskStatus->eCurrentState = eBlocked;
					}
				}
				xTaskResumeAll();
			}
		}
		#endif

		#if ( configUSE_MUTEXES == 1 )
		{
			pxTaskStatus->uxBasePriority = pxTCB->uxBasePriority;
		}
		#else
		{
			pxTaskStatus->uxBasePriority = 0;
		}
		#endif

		#if ( configGENERATE_RUN_TIME_STATS == 1 )
		{
			pxTaskStatus->ulRunTimeCounter = pxTCB->ulRunTimeCounter;
		}
		#else
		{
			pxTaskStatus->ulRunTimeCounter = 0;
		}
		#endif


		if( eState != eInvalid )
		{
			pxTaskStatus->eCurrentState = eState;
		}
		else
		{
			pxTaskStatus->eCurrentState = eTaskGetState( xTask );
		}


		if( xGetFreeStackSpace != pdFALSE )
		{
			#if ( portSTACK_GROWTH > 0 )
			{
				pxTaskStatus->usStackHighWaterMark = prvTaskCheckFreeStackSpace( ( uint8_t * ) pxTCB->pxEndOfStack );
			}
			#else
			{
				pxTaskStatus->usStackHighWaterMark = prvTaskCheckFreeStackSpace( ( uint8_t * ) pxTCB->pxStack );
			}
			#endif
		}
		else
		{
			pxTaskStatus->usStackHighWaterMark = 0;
		}
	}

#endif


#if ( configUSE_TRACE_FACILITY == 1 )

	static UBaseType_t prvListTasksWithinSingleList( TaskStatus_t *pxTaskStatusArray, List_t *pxList, eTaskState eState )
	{
	volatile TCB_t *pxNextTCB, *pxFirstTCB;
	UBaseType_t uxTask = 0;

		if( listCURRENT_LIST_LENGTH( pxList ) > ( UBaseType_t ) 0 )
		{
			listGET_OWNER_OF_NEXT_ENTRY( pxFirstTCB, pxList );


			do
			{
				listGET_OWNER_OF_NEXT_ENTRY( pxNextTCB, pxList );
				vTaskGetInfo( ( TaskHandle_t ) pxNextTCB, &( pxTaskStatusArray[ uxTask ] ), pdTRUE, eState );
				uxTask++;
			} while( pxNextTCB != pxFirstTCB );
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}

		return uxTask;
	}

#endif


#if ( ( configUSE_TRACE_FACILITY == 1 ) || ( INCLUDE_uxTaskGetStackHighWaterMark == 1 ) )

	static uint16_t prvTaskCheckFreeStackSpace( const uint8_t * pucStackByte )
	{
	uint32_t ulCount = 0U;

		while( *pucStackByte == ( uint8_t ) tskSTACK_FILL_BYTE )
		{
			pucStackByte -= portSTACK_GROWTH;
			ulCount++;
		}

		ulCount /= ( uint32_t ) sizeof( StackType_t );

		return ( uint16_t ) ulCount;
	}

#endif


#if ( INCLUDE_uxTaskGetStackHighWaterMark == 1 )

	UBaseType_t uxTaskGetStackHighWaterMark( TaskHandle_t xTask )
	{
	TCB_t *pxTCB;
	uint8_t *pucEndOfStack;
	UBaseType_t uxReturn;

		pxTCB = prvGetTCBFromHandle( xTask );

		#if portSTACK_GROWTH < 0
		{
			pucEndOfStack = ( uint8_t * ) pxTCB->pxStack;
		}
		#else
		{
			pucEndOfStack = ( uint8_t * ) pxTCB->pxEndOfStack;
		}
		#endif

		uxReturn = ( UBaseType_t ) prvTaskCheckFreeStackSpace( pucEndOfStack );

		return uxReturn;
	}

#endif


#if ( INCLUDE_vTaskDelete == 1 )

	static void prvDeleteTCB( TCB_t *pxTCB )
	{

		portCLEAN_UP_TCB( pxTCB );


		#if ( configUSE_NEWLIB_REENTRANT == 1 )
		{
			_reclaim_reent( &( pxTCB->xNewLib_reent ) );
		}
		#endif

		#if( ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) && ( configSUPPORT_STATIC_ALLOCATION == 0 ) && ( portUSING_MPU_WRAPPERS == 0 ) )
		{

			vPortFree( pxTCB->pxStack );
			vPortFree( pxTCB );
		}
		#elif( tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE == 1 )
		{

			if( pxTCB->ucStaticallyAllocated == tskDYNAMICALLY_ALLOCATED_STACK_AND_TCB )
			{

				vPortFree( pxTCB->pxStack );
				vPortFree( pxTCB );
			}
			else if( pxTCB->ucStaticallyAllocated == tskSTATICALLY_ALLOCATED_STACK_ONLY )
			{

				vPortFree( pxTCB );
			}
			else
			{

				configASSERT( pxTCB->ucStaticallyAllocated == tskSTATICALLY_ALLOCATED_STACK_AND_TCB	)
				mtCOVERAGE_TEST_MARKER();
			}
		}
		#endif
	}

#endif


static void prvResetNextTaskUnblockTime( void )
{
TCB_t *pxTCB;

	if( listLIST_IS_EMPTY( pxDelayedTaskList ) != pdFALSE )
	{

		xNextTaskUnblockTime = portMAX_DELAY;
	}
	else
	{

		( pxTCB ) = ( TCB_t * ) listGET_OWNER_OF_HEAD_ENTRY( pxDelayedTaskList );
		xNextTaskUnblockTime = listGET_LIST_ITEM_VALUE( &( ( pxTCB )->xStateListItem ) );
	}
}


#if ( ( INCLUDE_xTaskGetCurrentTaskHandle == 1 ) || ( configUSE_MUTEXES == 1 ) )

	TaskHandle_t xTaskGetCurrentTaskHandle( void )
	{
	TaskHandle_t xReturn;


		xReturn = pxCurrentTCB;

		return xReturn;
	}

#endif


#if ( ( INCLUDE_xTaskGetSchedulerState == 1 ) || ( configUSE_TIMERS == 1 ) )

	BaseType_t xTaskGetSchedulerState( void )
	{
	BaseType_t xReturn;

		if( xSchedulerRunning == pdFALSE )
		{
			xReturn = taskSCHEDULER_NOT_STARTED;
		}
		else
		{
			if( uxSchedulerSuspended == ( UBaseType_t ) pdFALSE )
			{
				xReturn = taskSCHEDULER_RUNNING;
			}
			else
			{
				xReturn = taskSCHEDULER_SUSPENDED;
			}
		}

		return xReturn;
	}

#endif


#if ( configUSE_MUTEXES == 1 )

	void vTaskPriorityInherit( TaskHandle_t const pxMutexHolder )
	{
	TCB_t * const pxTCB = ( TCB_t * ) pxMutexHolder;


		if( pxMutexHolder != NULL )
		{

			if( pxTCB->uxPriority < pxCurrentTCB->uxPriority )
			{

				if( ( listGET_LIST_ITEM_VALUE( &( pxTCB->xEventListItem ) ) & taskEVENT_LIST_ITEM_VALUE_IN_USE ) == 0UL )
				{
					listSET_LIST_ITEM_VALUE( &( pxTCB->xEventListItem ), ( TickType_t ) configMAX_PRIORITIES - ( TickType_t ) pxCurrentTCB->uxPriority );
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}


				if( listIS_CONTAINED_WITHIN( &( pxReadyTasksLists[ pxTCB->uxPriority ] ), &( pxTCB->xStateListItem ) ) != pdFALSE )
				{
					if( uxListRemove( &( pxTCB->xStateListItem ) ) == ( UBaseType_t ) 0 )
					{
						taskRESET_READY_PRIORITY( pxTCB->uxPriority );
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}


					pxTCB->uxPriority = pxCurrentTCB->uxPriority;
					prvAddTaskToReadyList( pxTCB );
				}
				else
				{

					pxTCB->uxPriority = pxCurrentTCB->uxPriority;
				}

				traceTASK_PRIORITY_INHERIT( pxTCB, pxCurrentTCB->uxPriority );
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}

#endif


#if ( configUSE_MUTEXES == 1 )

	BaseType_t xTaskPriorityDisinherit( TaskHandle_t const pxMutexHolder )
	{
	TCB_t * const pxTCB = ( TCB_t * ) pxMutexHolder;
	BaseType_t xReturn = pdFALSE;

		if( pxMutexHolder != NULL )
		{

			configASSERT( pxTCB == pxCurrentTCB );

			configASSERT( pxTCB->uxMutexesHeld );
			( pxTCB->uxMutexesHeld )--;


			if( pxTCB->uxPriority != pxTCB->uxBasePriority )
			{

				if( pxTCB->uxMutexesHeld == ( UBaseType_t ) 0 )
				{

					if( uxListRemove( &( pxTCB->xStateListItem ) ) == ( UBaseType_t ) 0 )
					{
						taskRESET_READY_PRIORITY( pxTCB->uxPriority );
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}


					traceTASK_PRIORITY_DISINHERIT( pxTCB, pxTCB->uxBasePriority );
					pxTCB->uxPriority = pxTCB->uxBasePriority;


					listSET_LIST_ITEM_VALUE( &( pxTCB->xEventListItem ), ( TickType_t ) configMAX_PRIORITIES - ( TickType_t ) pxTCB->uxPriority );
					prvAddTaskToReadyList( pxTCB );


					xReturn = pdTRUE;
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}

		return xReturn;
	}

#endif


#if ( portCRITICAL_NESTING_IN_TCB == 1 )

	void vTaskEnterCritical( void )
	{
		portDISABLE_INTERRUPTS();

		if( xSchedulerRunning != pdFALSE )
		{
			( pxCurrentTCB->uxCriticalNesting )++;


			if( pxCurrentTCB->uxCriticalNesting == 1 )
			{
				portASSERT_IF_IN_ISR();
			}
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}

#endif


#if ( portCRITICAL_NESTING_IN_TCB == 1 )

	void vTaskExitCritical( void )
	{
		if( xSchedulerRunning != pdFALSE )
		{
			if( pxCurrentTCB->uxCriticalNesting > 0U )
			{
				( pxCurrentTCB->uxCriticalNesting )--;

				if( pxCurrentTCB->uxCriticalNesting == 0U )
				{
					portENABLE_INTERRUPTS();
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}

#endif


#if ( ( configUSE_TRACE_FACILITY == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) )

	static char *prvWriteNameToBuffer( char *pcBuffer, const char *pcTaskName )
	{
	size_t x;


		strcpy( pcBuffer, pcTaskName );


		for( x = strlen( pcBuffer ); x < ( size_t ) ( configMAX_TASK_NAME_LEN - 1 ); x++ )
		{
			pcBuffer[ x ] = ' ';
		}


		pcBuffer[ x ] = 0x00;


		return &( pcBuffer[ x ] );
	}

#endif


#if ( ( configUSE_TRACE_FACILITY == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) )

	void vTaskList( char * pcWriteBuffer )
	{
	TaskStatus_t *pxTaskStatusArray;
	volatile UBaseType_t uxArraySize, x;
	char cStatus;


		*pcWriteBuffer = 0x00;


		uxArraySize = uxCurrentNumberOfTasks;


		pxTaskStatusArray = pvPortMalloc( uxCurrentNumberOfTasks * sizeof( TaskStatus_t ) );

		if( pxTaskStatusArray != NULL )
		{

			uxArraySize = uxTaskGetSystemState( pxTaskStatusArray, uxArraySize, NULL );


			for( x = 0; x < uxArraySize; x++ )
			{
				switch( pxTaskStatusArray[ x ].eCurrentState )
				{
					case eReady:		cStatus = tskREADY_CHAR;
										break;

					case eBlocked:		cStatus = tskBLOCKED_CHAR;
										break;

					case eSuspended:	cStatus = tskSUSPENDED_CHAR;
										break;

					case eDeleted:		cStatus = tskDELETED_CHAR;
										break;

					default:
										cStatus = 0x00;
										break;
				}


				pcWriteBuffer = prvWriteNameToBuffer( pcWriteBuffer, pxTaskStatusArray[ x ].pcTaskName );


				sprintf( pcWriteBuffer, "\t%c\t%u\t%u\t%u\r\n", cStatus, ( unsigned int ) pxTaskStatusArray[ x ].uxCurrentPriority, ( unsigned int ) pxTaskStatusArray[ x ].usStackHighWaterMark, ( unsigned int ) pxTaskStatusArray[ x ].xTaskNumber );
				pcWriteBuffer += strlen( pcWriteBuffer );
			}


			vPortFree( pxTaskStatusArray );
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}

#endif


#if ( ( configGENERATE_RUN_TIME_STATS == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) )

	void vTaskGetRunTimeStats( char *pcWriteBuffer )
	{
	TaskStatus_t *pxTaskStatusArray;
	volatile UBaseType_t uxArraySize, x;
	uint32_t ulTotalTime, ulStatsAsPercentage;

		#if( configUSE_TRACE_FACILITY != 1 )
		{
			#error configUSE_TRACE_FACILITY must also be set to 1 in FreeRTOSConfig.h to use vTaskGetRunTimeStats().
		}
		#endif


		*pcWriteBuffer = 0x00;


		uxArraySize = uxCurrentNumberOfTasks;


		pxTaskStatusArray = pvPortMalloc( uxCurrentNumberOfTasks * sizeof( TaskStatus_t ) );

		if( pxTaskStatusArray != NULL )
		{

			uxArraySize = uxTaskGetSystemState( pxTaskStatusArray, uxArraySize, &ulTotalTime );


			ulTotalTime /= 100UL;


			if( ulTotalTime > 0 )
			{

				for( x = 0; x < uxArraySize; x++ )
				{

					ulStatsAsPercentage = pxTaskStatusArray[ x ].ulRunTimeCounter / ulTotalTime;


					pcWriteBuffer = prvWriteNameToBuffer( pcWriteBuffer, pxTaskStatusArray[ x ].pcTaskName );

					if( ulStatsAsPercentage > 0UL )
					{
						#ifdef portLU_PRINTF_SPECIFIER_REQUIRED
						{
							sprintf( pcWriteBuffer, "\t%lu\t\t%lu%%\r\n", pxTaskStatusArray[ x ].ulRunTimeCounter, ulStatsAsPercentage );
						}
						#else
						{

							sprintf( pcWriteBuffer, "\t%u\t\t%u%%\r\n", ( unsigned int ) pxTaskStatusArray[ x ].ulRunTimeCounter, ( unsigned int ) ulStatsAsPercentage );
						}
						#endif
					}
					else
					{

						#ifdef portLU_PRINTF_SPECIFIER_REQUIRED
						{
							sprintf( pcWriteBuffer, "\t%lu\t\t<1%%\r\n", pxTaskStatusArray[ x ].ulRunTimeCounter );
						}
						#else
						{

							sprintf( pcWriteBuffer, "\t%u\t\t<1%%\r\n", ( unsigned int ) pxTaskStatusArray[ x ].ulRunTimeCounter );
						}
						#endif
					}

					pcWriteBuffer += strlen( pcWriteBuffer );
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}


			vPortFree( pxTaskStatusArray );
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}

#endif


TickType_t uxTaskResetEventItemValue( void )
{
TickType_t uxReturn;

	uxReturn = listGET_LIST_ITEM_VALUE( &( pxCurrentTCB->xEventListItem ) );


	listSET_LIST_ITEM_VALUE( &( pxCurrentTCB->xEventListItem ), ( ( TickType_t ) configMAX_PRIORITIES - ( TickType_t ) pxCurrentTCB->uxPriority ) );

	return uxReturn;
}


#if ( configUSE_MUTEXES == 1 )

	void *pvTaskIncrementMutexHeldCount( void )
	{

		if( pxCurrentTCB != NULL )
		{
			( pxCurrentTCB->uxMutexesHeld )++;
		}

		return pxCurrentTCB;
	}

#endif


#if( configUSE_TASK_NOTIFICATIONS == 1 )

	uint32_t ulTaskNotifyTake( BaseType_t xClearCountOnExit, TickType_t xTicksToWait )
	{
	uint32_t ulReturn;

		taskENTER_CRITICAL();
		{

			if( pxCurrentTCB->ulNotifiedValue == 0UL )
			{

				pxCurrentTCB->ucNotifyState = taskWAITING_NOTIFICATION;

				if( xTicksToWait > ( TickType_t ) 0 )
				{
					prvAddCurrentTaskToDelayedList( xTicksToWait, pdTRUE );
					traceTASK_NOTIFY_TAKE_BLOCK();


					portYIELD_WITHIN_API();
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		taskEXIT_CRITICAL();

		taskENTER_CRITICAL();
		{
			traceTASK_NOTIFY_TAKE();
			ulReturn = pxCurrentTCB->ulNotifiedValue;

			if( ulReturn != 0UL )
			{
				if( xClearCountOnExit != pdFALSE )
				{
					pxCurrentTCB->ulNotifiedValue = 0UL;
				}
				else
				{
					pxCurrentTCB->ulNotifiedValue = ulReturn - 1;
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}

			pxCurrentTCB->ucNotifyState = taskNOT_WAITING_NOTIFICATION;
		}
		taskEXIT_CRITICAL();

		return ulReturn;
	}

#endif


#if( configUSE_TASK_NOTIFICATIONS == 1 )

	BaseType_t xTaskNotifyWait( uint32_t ulBitsToClearOnEntry, uint32_t ulBitsToClearOnExit, uint32_t *pulNotificationValue, TickType_t xTicksToWait )
	{
	BaseType_t xReturn;

		taskENTER_CRITICAL();
		{

			if( pxCurrentTCB->ucNotifyState != taskNOTIFICATION_RECEIVED )
			{

				pxCurrentTCB->ulNotifiedValue &= ~ulBitsToClearOnEntry;


				pxCurrentTCB->ucNotifyState = taskWAITING_NOTIFICATION;

				if( xTicksToWait > ( TickType_t ) 0 )
				{
					prvAddCurrentTaskToDelayedList( xTicksToWait, pdTRUE );
					traceTASK_NOTIFY_WAIT_BLOCK();


					portYIELD_WITHIN_API();
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		taskEXIT_CRITICAL();

		taskENTER_CRITICAL();
		{
			traceTASK_NOTIFY_WAIT();

			if( pulNotificationValue != NULL )
			{

				*pulNotificationValue = pxCurrentTCB->ulNotifiedValue;
			}


			if( pxCurrentTCB->ucNotifyState == taskWAITING_NOTIFICATION )
			{

				xReturn = pdFALSE;
			}
			else
			{

				pxCurrentTCB->ulNotifiedValue &= ~ulBitsToClearOnExit;
				xReturn = pdTRUE;
			}

			pxCurrentTCB->ucNotifyState = taskNOT_WAITING_NOTIFICATION;
		}
		taskEXIT_CRITICAL();

		return xReturn;
	}

#endif


#if( configUSE_TASK_NOTIFICATIONS == 1 )

	BaseType_t xTaskGenericNotify( TaskHandle_t xTaskToNotify, uint32_t ulValue, eNotifyAction eAction, uint32_t *pulPreviousNotificationValue )
	{
	TCB_t * pxTCB;
	BaseType_t xReturn = pdPASS;
	uint8_t ucOriginalNotifyState;

		configASSERT( xTaskToNotify );
		pxTCB = ( TCB_t * ) xTaskToNotify;

		taskENTER_CRITICAL();
		{
			if( pulPreviousNotificationValue != NULL )
			{
				*pulPreviousNotificationValue = pxTCB->ulNotifiedValue;
			}

			ucOriginalNotifyState = pxTCB->ucNotifyState;

			pxTCB->ucNotifyState = taskNOTIFICATION_RECEIVED;

			switch( eAction )
			{
				case eSetBits	:
					pxTCB->ulNotifiedValue |= ulValue;
					break;

				case eIncrement	:
					( pxTCB->ulNotifiedValue )++;
					break;

				case eSetValueWithOverwrite	:
					pxTCB->ulNotifiedValue = ulValue;
					break;

				case eSetValueWithoutOverwrite :
					if( ucOriginalNotifyState != taskNOTIFICATION_RECEIVED )
					{
						pxTCB->ulNotifiedValue = ulValue;
					}
					else
					{

						xReturn = pdFAIL;
					}
					break;

				case eNoAction:

					break;
			}

			traceTASK_NOTIFY();


			if( ucOriginalNotifyState == taskWAITING_NOTIFICATION )
			{
				( void ) uxListRemove( &( pxTCB->xStateListItem ) );
				prvAddTaskToReadyList( pxTCB );


				configASSERT( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) == NULL );

				#if( configUSE_TICKLESS_IDLE != 0 )
				{

					prvResetNextTaskUnblockTime();
				}
				#endif

				if( pxTCB->uxPriority > pxCurrentTCB->uxPriority )
				{

					taskYIELD_IF_USING_PREEMPTION();
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		taskEXIT_CRITICAL();

		return xReturn;
	}

#endif


#if( configUSE_TASK_NOTIFICATIONS == 1 )

	BaseType_t xTaskGenericNotifyFromISR( TaskHandle_t xTaskToNotify, uint32_t ulValue, eNotifyAction eAction, uint32_t *pulPreviousNotificationValue, BaseType_t *pxHigherPriorityTaskWoken )
	{
	TCB_t * pxTCB;
	uint8_t ucOriginalNotifyState;
	BaseType_t xReturn = pdPASS;
	UBaseType_t uxSavedInterruptStatus;

		configASSERT( xTaskToNotify );


		portASSERT_IF_INTERRUPT_PRIORITY_INVALID();

		pxTCB = ( TCB_t * ) xTaskToNotify;

		uxSavedInterruptStatus = portSET_INTERRUPT_MASK_FROM_ISR();
		{
			if( pulPreviousNotificationValue != NULL )
			{
				*pulPreviousNotificationValue = pxTCB->ulNotifiedValue;
			}

			ucOriginalNotifyState = pxTCB->ucNotifyState;
			pxTCB->ucNotifyState = taskNOTIFICATION_RECEIVED;

			switch( eAction )
			{
				case eSetBits	:
					pxTCB->ulNotifiedValue |= ulValue;
					break;

				case eIncrement	:
					( pxTCB->ulNotifiedValue )++;
					break;

				case eSetValueWithOverwrite	:
					pxTCB->ulNotifiedValue = ulValue;
					break;

				case eSetValueWithoutOverwrite :
					if( ucOriginalNotifyState != taskNOTIFICATION_RECEIVED )
					{
						pxTCB->ulNotifiedValue = ulValue;
					}
					else
					{

						xReturn = pdFAIL;
					}
					break;

				case eNoAction :

					break;
			}

			traceTASK_NOTIFY_FROM_ISR();


			if( ucOriginalNotifyState == taskWAITING_NOTIFICATION )
			{

				configASSERT( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) == NULL );

				if( uxSchedulerSuspended == ( UBaseType_t ) pdFALSE )
				{
					( void ) uxListRemove( &( pxTCB->xStateListItem ) );
					prvAddTaskToReadyList( pxTCB );
				}
				else
				{

					vListInsertEnd( &( xPendingReadyList ), &( pxTCB->xEventListItem ) );
				}

				if( pxTCB->uxPriority > pxCurrentTCB->uxPriority )
				{

					if( pxHigherPriorityTaskWoken != NULL )
					{
						*pxHigherPriorityTaskWoken = pdTRUE;
					}
					else
					{

						xYieldPending = pdTRUE;
					}
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
		}
		portCLEAR_INTERRUPT_MASK_FROM_ISR( uxSavedInterruptStatus );

		return xReturn;
	}

#endif


#if( configUSE_TASK_NOTIFICATIONS == 1 )

	void vTaskNotifyGiveFromISR( TaskHandle_t xTaskToNotify, BaseType_t *pxHigherPriorityTaskWoken )
	{
	TCB_t * pxTCB;
	uint8_t ucOriginalNotifyState;
	UBaseType_t uxSavedInterruptStatus;

		configASSERT( xTaskToNotify );


		portASSERT_IF_INTERRUPT_PRIORITY_INVALID();

		pxTCB = ( TCB_t * ) xTaskToNotify;

		uxSavedInterruptStatus = portSET_INTERRUPT_MASK_FROM_ISR();
		{
			ucOriginalNotifyState = pxTCB->ucNotifyState;
			pxTCB->ucNotifyState = taskNOTIFICATION_RECEIVED;


			( pxTCB->ulNotifiedValue )++;

			traceTASK_NOTIFY_GIVE_FROM_ISR();


			if( ucOriginalNotifyState == taskWAITING_NOTIFICATION )
			{

				configASSERT( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) == NULL );

				if( uxSchedulerSuspended == ( UBaseType_t ) pdFALSE )
				{
					( void ) uxListRemove( &( pxTCB->xStateListItem ) );
					prvAddTaskToReadyList( pxTCB );
				}
				else
				{

					vListInsertEnd( &( xPendingReadyList ), &( pxTCB->xEventListItem ) );
				}

				if( pxTCB->uxPriority > pxCurrentTCB->uxPriority )
				{

					if( pxHigherPriorityTaskWoken != NULL )
					{
						*pxHigherPriorityTaskWoken = pdTRUE;
					}
					else
					{

						xYieldPending = pdTRUE;
					}
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
		}
		portCLEAR_INTERRUPT_MASK_FROM_ISR( uxSavedInterruptStatus );
	}

#endif


#if( configUSE_TASK_NOTIFICATIONS == 1 )

	BaseType_t xTaskNotifyStateClear( TaskHandle_t xTask )
	{
	TCB_t *pxTCB;
	BaseType_t xReturn;


		pxTCB = prvGetTCBFromHandle( xTask );

		taskENTER_CRITICAL();
		{
			if( pxTCB->ucNotifyState == taskNOTIFICATION_RECEIVED )
			{
				pxTCB->ucNotifyState = taskNOT_WAITING_NOTIFICATION;
				xReturn = pdPASS;
			}
			else
			{
				xReturn = pdFAIL;
			}
		}
		taskEXIT_CRITICAL();

		return xReturn;
	}

#endif


static void prvAddCurrentTaskToDelayedList( TickType_t xTicksToWait, const BaseType_t xCanBlockIndefinitely )
{
TickType_t xTimeToWake;
const TickType_t xConstTickCount = xTickCount;

	#if( INCLUDE_xTaskAbortDelay == 1 )
	{

		pxCurrentTCB->ucDelayAborted = pdFALSE;
	}
	#endif


	if( uxListRemove( &( pxCurrentTCB->xStateListItem ) ) == ( UBaseType_t ) 0 )
	{

		portRESET_READY_PRIORITY( pxCurrentTCB->uxPriority, uxTopReadyPriority );
	}
	else
	{
		mtCOVERAGE_TEST_MARKER();
	}

	#if ( INCLUDE_vTaskSuspend == 1 )
	{
		if( ( xTicksToWait == portMAX_DELAY ) && ( xCanBlockIndefinitely != pdFALSE ) )
		{

			vListInsertEnd( &xSuspendedTaskList, &( pxCurrentTCB->xStateListItem ) );
		}
		else
		{

			xTimeToWake = xConstTickCount + xTicksToWait;


			listSET_LIST_ITEM_VALUE( &( pxCurrentTCB->xStateListItem ), xTimeToWake );

			if( xTimeToWake < xConstTickCount )
			{

				vListInsert( pxOverflowDelayedTaskList, &( pxCurrentTCB->xStateListItem ) );
			}
			else
			{

				vListInsert( pxDelayedTaskList, &( pxCurrentTCB->xStateListItem ) );


				if( xTimeToWake < xNextTaskUnblockTime )
				{
					xNextTaskUnblockTime = xTimeToWake;
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
		}
	}
	#else
	{

		xTimeToWake = xConstTickCount + xTicksToWait;


		listSET_LIST_ITEM_VALUE( &( pxCurrentTCB->xStateListItem ), xTimeToWake );

		if( xTimeToWake < xConstTickCount )
		{

			vListInsert( pxOverflowDelayedTaskList, &( pxCurrentTCB->xStateListItem ) );
		}
		else
		{

			vListInsert( pxDelayedTaskList, &( pxCurrentTCB->xStateListItem ) );


			if( xTimeToWake < xNextTaskUnblockTime )
			{
				xNextTaskUnblockTime = xTimeToWake;
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}


		( void ) xCanBlockIndefinitely;
	}
	#endif
}


#ifdef FREERTOS_MODULE_TEST
	#include "tasks_test_access_functions.h"
#endif
