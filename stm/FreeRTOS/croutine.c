/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        croutine.c
 *
 * Description:  croutine.c module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#include "FreeRTOS.h"
#include "task.h"
#include "croutine.h"


#if( configUSE_CO_ROUTINES != 0 )


#ifdef portREMOVE_STATIC_QUALIFIER
	#define static
#endif


static List_t pxReadyCoRoutineLists[ configMAX_CO_ROUTINE_PRIORITIES ];
static List_t xDelayedCoRoutineList1;
static List_t xDelayedCoRoutineList2;
static List_t * pxDelayedCoRoutineList;
static List_t * pxOverflowDelayedCoRoutineList;
static List_t xPendingReadyCoRoutineList;


CRCB_t * pxCurrentCoRoutine = NULL;
static UBaseType_t uxTopCoRoutineReadyPriority = 0;
static TickType_t xCoRoutineTickCount = 0, xLastTickCount = 0, xPassedTicks = 0;


#define corINITIAL_STATE	( 0 )


#define prvAddCoRoutineToReadyQueue( pxCRCB )																		\
{																													\
	if( pxCRCB->uxPriority > uxTopCoRoutineReadyPriority )															\
	{																												\
		uxTopCoRoutineReadyPriority = pxCRCB->uxPriority;															\
	}																												\
	vListInsertEnd( ( List_t * ) &( pxReadyCoRoutineLists[ pxCRCB->uxPriority ] ), &( pxCRCB->xGenericListItem ) );	\
}


static void prvInitialiseCoRoutineLists( void );


static void prvCheckPendingReadyList( void );


static void prvCheckDelayedList( void );


BaseType_t xCoRoutineCreate( crCOROUTINE_CODE pxCoRoutineCode, UBaseType_t uxPriority, UBaseType_t uxIndex )
{
BaseType_t xReturn;
CRCB_t *pxCoRoutine;


	pxCoRoutine = ( CRCB_t * ) pvPortMalloc( sizeof( CRCB_t ) );
	if( pxCoRoutine )
	{

		if( pxCurrentCoRoutine == NULL )
		{
			pxCurrentCoRoutine = pxCoRoutine;
			prvInitialiseCoRoutineLists();
		}


		if( uxPriority >= configMAX_CO_ROUTINE_PRIORITIES )
		{
			uxPriority = configMAX_CO_ROUTINE_PRIORITIES - 1;
		}


		pxCoRoutine->uxState = corINITIAL_STATE;
		pxCoRoutine->uxPriority = uxPriority;
		pxCoRoutine->uxIndex = uxIndex;
		pxCoRoutine->pxCoRoutineFunction = pxCoRoutineCode;


		vListInitialiseItem( &( pxCoRoutine->xGenericListItem ) );
		vListInitialiseItem( &( pxCoRoutine->xEventListItem ) );


		listSET_LIST_ITEM_OWNER( &( pxCoRoutine->xGenericListItem ), pxCoRoutine );
		listSET_LIST_ITEM_OWNER( &( pxCoRoutine->xEventListItem ), pxCoRoutine );


		listSET_LIST_ITEM_VALUE( &( pxCoRoutine->xEventListItem ), ( ( TickType_t ) configMAX_CO_ROUTINE_PRIORITIES - ( TickType_t ) uxPriority ) );


		prvAddCoRoutineToReadyQueue( pxCoRoutine );

		xReturn = pdPASS;
	}
	else
	{
		xReturn = errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
	}

	return xReturn;
}


void vCoRoutineAddToDelayedList( TickType_t xTicksToDelay, List_t *pxEventList )
{
TickType_t xTimeToWake;


	xTimeToWake = xCoRoutineTickCount + xTicksToDelay;


	( void ) uxListRemove( ( ListItem_t * ) &( pxCurrentCoRoutine->xGenericListItem ) );


	listSET_LIST_ITEM_VALUE( &( pxCurrentCoRoutine->xGenericListItem ), xTimeToWake );

	if( xTimeToWake < xCoRoutineTickCount )
	{

		vListInsert( ( List_t * ) pxOverflowDelayedCoRoutineList, ( ListItem_t * ) &( pxCurrentCoRoutine->xGenericListItem ) );
	}
	else
	{

		vListInsert( ( List_t * ) pxDelayedCoRoutineList, ( ListItem_t * ) &( pxCurrentCoRoutine->xGenericListItem ) );
	}

	if( pxEventList )
	{

		vListInsert( pxEventList, &( pxCurrentCoRoutine->xEventListItem ) );
	}
}


static void prvCheckPendingReadyList( void )
{

	while( listLIST_IS_EMPTY( &xPendingReadyCoRoutineList ) == pdFALSE )
	{
		CRCB_t *pxUnblockedCRCB;


		portDISABLE_INTERRUPTS();
		{
			pxUnblockedCRCB = ( CRCB_t * ) listGET_OWNER_OF_HEAD_ENTRY( (&xPendingReadyCoRoutineList) );
			( void ) uxListRemove( &( pxUnblockedCRCB->xEventListItem ) );
		}
		portENABLE_INTERRUPTS();

		( void ) uxListRemove( &( pxUnblockedCRCB->xGenericListItem ) );
		prvAddCoRoutineToReadyQueue( pxUnblockedCRCB );
	}
}


static void prvCheckDelayedList( void )
{
CRCB_t *pxCRCB;

	xPassedTicks = xTaskGetTickCount() - xLastTickCount;
	while( xPassedTicks )
	{
		xCoRoutineTickCount++;
		xPassedTicks--;


		if( xCoRoutineTickCount == 0 )
		{
			List_t * pxTemp;


			pxTemp = pxDelayedCoRoutineList;
			pxDelayedCoRoutineList = pxOverflowDelayedCoRoutineList;
			pxOverflowDelayedCoRoutineList = pxTemp;
		}


		while( listLIST_IS_EMPTY( pxDelayedCoRoutineList ) == pdFALSE )
		{
			pxCRCB = ( CRCB_t * ) listGET_OWNER_OF_HEAD_ENTRY( pxDelayedCoRoutineList );

			if( xCoRoutineTickCount < listGET_LIST_ITEM_VALUE( &( pxCRCB->xGenericListItem ) ) )
			{

				break;
			}

			portDISABLE_INTERRUPTS();
			{

				( void ) uxListRemove( &( pxCRCB->xGenericListItem ) );


				if( pxCRCB->xEventListItem.pvContainer )
				{
					( void ) uxListRemove( &( pxCRCB->xEventListItem ) );
				}
			}
			portENABLE_INTERRUPTS();

			prvAddCoRoutineToReadyQueue( pxCRCB );
		}
	}

	xLastTickCount = xCoRoutineTickCount;
}


void vCoRoutineSchedule( void )
{

	prvCheckPendingReadyList();


	prvCheckDelayedList();


	while( listLIST_IS_EMPTY( &( pxReadyCoRoutineLists[ uxTopCoRoutineReadyPriority ] ) ) )
	{
		if( uxTopCoRoutineReadyPriority == 0 )
		{

			return;
		}
		--uxTopCoRoutineReadyPriority;
	}


	listGET_OWNER_OF_NEXT_ENTRY( pxCurrentCoRoutine, &( pxReadyCoRoutineLists[ uxTopCoRoutineReadyPriority ] ) );


	( pxCurrentCoRoutine->pxCoRoutineFunction )( pxCurrentCoRoutine, pxCurrentCoRoutine->uxIndex );

	return;
}


static void prvInitialiseCoRoutineLists( void )
{
UBaseType_t uxPriority;

	for( uxPriority = 0; uxPriority < configMAX_CO_ROUTINE_PRIORITIES; uxPriority++ )
	{
		vListInitialise( ( List_t * ) &( pxReadyCoRoutineLists[ uxPriority ] ) );
	}

	vListInitialise( ( List_t * ) &xDelayedCoRoutineList1 );
	vListInitialise( ( List_t * ) &xDelayedCoRoutineList2 );
	vListInitialise( ( List_t * ) &xPendingReadyCoRoutineList );


	pxDelayedCoRoutineList = &xDelayedCoRoutineList1;
	pxOverflowDelayedCoRoutineList = &xDelayedCoRoutineList2;
}


BaseType_t xCoRoutineRemoveFromEventList( const List_t *pxEventList )
{
CRCB_t *pxUnblockedCRCB;
BaseType_t xReturn;


	pxUnblockedCRCB = ( CRCB_t * ) listGET_OWNER_OF_HEAD_ENTRY( pxEventList );
	( void ) uxListRemove( &( pxUnblockedCRCB->xEventListItem ) );
	vListInsertEnd( ( List_t * ) &( xPendingReadyCoRoutineList ), &( pxUnblockedCRCB->xEventListItem ) );

	if( pxUnblockedCRCB->uxPriority >= pxCurrentCoRoutine->uxPriority )
	{
		xReturn = pdTRUE;
	}
	else
	{
		xReturn = pdFALSE;
	}

	return xReturn;
}

#endif
