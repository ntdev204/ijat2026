/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        croutine.h
 *
 * Description:  croutine.h module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#ifndef CO_ROUTINE_H
#define CO_ROUTINE_H

#ifndef INC_FREERTOS_H
	#error "include FreeRTOS.h must appear in source files before include croutine.h"
#endif

#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef void * CoRoutineHandle_t;


typedef void (*crCOROUTINE_CODE)( CoRoutineHandle_t, UBaseType_t );

typedef struct corCoRoutineControlBlock
{
	crCOROUTINE_CODE 	pxCoRoutineFunction;
	ListItem_t			xGenericListItem;
	ListItem_t			xEventListItem;
	UBaseType_t 		uxPriority;
	UBaseType_t 		uxIndex;
	uint16_t 			uxState;
} CRCB_t;


BaseType_t xCoRoutineCreate( crCOROUTINE_CODE pxCoRoutineCode, UBaseType_t uxPriority, UBaseType_t uxIndex );


void vCoRoutineSchedule( void );


#define crSTART( pxCRCB ) switch( ( ( CRCB_t * )( pxCRCB ) )->uxState ) { case 0:


#define crEND() }


#define crSET_STATE0( xHandle ) ( ( CRCB_t * )( xHandle ) )->uxState = (__LINE__ * 2); return; case (__LINE__ * 2):
#define crSET_STATE1( xHandle ) ( ( CRCB_t * )( xHandle ) )->uxState = ((__LINE__ * 2)+1); return; case ((__LINE__ * 2)+1):


#define crDELAY( xHandle, xTicksToDelay )												\
	if( ( xTicksToDelay ) > 0 )															\
	{																					\
		vCoRoutineAddToDelayedList( ( xTicksToDelay ), NULL );							\
	}																					\
	crSET_STATE0( ( xHandle ) );


#define crQUEUE_SEND( xHandle, pxQueue, pvItemToQueue, xTicksToWait, pxResult )			\
{																						\
	*( pxResult ) = xQueueCRSend( ( pxQueue) , ( pvItemToQueue) , ( xTicksToWait ) );	\
	if( *( pxResult ) == errQUEUE_BLOCKED )												\
	{																					\
		crSET_STATE0( ( xHandle ) );													\
		*pxResult = xQueueCRSend( ( pxQueue ), ( pvItemToQueue ), 0 );					\
	}																					\
	if( *pxResult == errQUEUE_YIELD )													\
	{																					\
		crSET_STATE1( ( xHandle ) );													\
		*pxResult = pdPASS;																\
	}																					\
}


#define crQUEUE_RECEIVE( xHandle, pxQueue, pvBuffer, xTicksToWait, pxResult )			\
{																						\
	*( pxResult ) = xQueueCRReceive( ( pxQueue) , ( pvBuffer ), ( xTicksToWait ) );		\
	if( *( pxResult ) == errQUEUE_BLOCKED ) 											\
	{																					\
		crSET_STATE0( ( xHandle ) );													\
		*( pxResult ) = xQueueCRReceive( ( pxQueue) , ( pvBuffer ), 0 );				\
	}																					\
	if( *( pxResult ) == errQUEUE_YIELD )												\
	{																					\
		crSET_STATE1( ( xHandle ) );													\
		*( pxResult ) = pdPASS;															\
	}																					\
}


#define crQUEUE_SEND_FROM_ISR( pxQueue, pvItemToQueue, xCoRoutinePreviouslyWoken ) xQueueCRSendFromISR( ( pxQueue ), ( pvItemToQueue ), ( xCoRoutinePreviouslyWoken ) )


#define crQUEUE_RECEIVE_FROM_ISR( pxQueue, pvBuffer, pxCoRoutineWoken ) xQueueCRReceiveFromISR( ( pxQueue ), ( pvBuffer ), ( pxCoRoutineWoken ) )


void vCoRoutineAddToDelayedList( TickType_t xTicksToDelay, List_t *pxEventList );


BaseType_t xCoRoutineRemoveFromEventList( const List_t *pxEventList );

#ifdef __cplusplus
}
#endif

#endif
