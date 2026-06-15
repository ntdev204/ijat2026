/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        event_groups.h
 *
 * Description:  event_groups.h module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#ifndef EVENT_GROUPS_H
#define EVENT_GROUPS_H

#ifndef INC_FREERTOS_H
	#error "include FreeRTOS.h" must appear in source files before "include event_groups.h"
#endif


#include "timers.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef void * EventGroupHandle_t;


typedef TickType_t EventBits_t;


#if( configSUPPORT_DYNAMIC_ALLOCATION == 1 )
	EventGroupHandle_t xEventGroupCreate( void ) PRIVILEGED_FUNCTION;
#endif


#if( configSUPPORT_STATIC_ALLOCATION == 1 )
	EventGroupHandle_t xEventGroupCreateStatic( StaticEventGroup_t *pxEventGroupBuffer ) PRIVILEGED_FUNCTION;
#endif


EventBits_t xEventGroupWaitBits( EventGroupHandle_t xEventGroup, const EventBits_t uxBitsToWaitFor, const BaseType_t xClearOnExit, const BaseType_t xWaitForAllBits, TickType_t xTicksToWait ) PRIVILEGED_FUNCTION;


EventBits_t xEventGroupClearBits( EventGroupHandle_t xEventGroup, const EventBits_t uxBitsToClear ) PRIVILEGED_FUNCTION;


#if( configUSE_TRACE_FACILITY == 1 )
	BaseType_t xEventGroupClearBitsFromISR( EventGroupHandle_t xEventGroup, const EventBits_t uxBitsToSet ) PRIVILEGED_FUNCTION;
#else
	#define xEventGroupClearBitsFromISR( xEventGroup, uxBitsToClear ) xTimerPendFunctionCallFromISR( vEventGroupClearBitsCallback, ( void * ) xEventGroup, ( uint32_t ) uxBitsToClear, NULL )
#endif


EventBits_t xEventGroupSetBits( EventGroupHandle_t xEventGroup, const EventBits_t uxBitsToSet ) PRIVILEGED_FUNCTION;


#if( configUSE_TRACE_FACILITY == 1 )
	BaseType_t xEventGroupSetBitsFromISR( EventGroupHandle_t xEventGroup, const EventBits_t uxBitsToSet, BaseType_t *pxHigherPriorityTaskWoken ) PRIVILEGED_FUNCTION;
#else
	#define xEventGroupSetBitsFromISR( xEventGroup, uxBitsToSet, pxHigherPriorityTaskWoken ) xTimerPendFunctionCallFromISR( vEventGroupSetBitsCallback, ( void * ) xEventGroup, ( uint32_t ) uxBitsToSet, pxHigherPriorityTaskWoken )
#endif


EventBits_t xEventGroupSync( EventGroupHandle_t xEventGroup, const EventBits_t uxBitsToSet, const EventBits_t uxBitsToWaitFor, TickType_t xTicksToWait ) PRIVILEGED_FUNCTION;


#define xEventGroupGetBits( xEventGroup ) xEventGroupClearBits( xEventGroup, 0 )


EventBits_t xEventGroupGetBitsFromISR( EventGroupHandle_t xEventGroup ) PRIVILEGED_FUNCTION;


void vEventGroupDelete( EventGroupHandle_t xEventGroup ) PRIVILEGED_FUNCTION;


void vEventGroupSetBitsCallback( void *pvEventGroup, const uint32_t ulBitsToSet ) PRIVILEGED_FUNCTION;
void vEventGroupClearBitsCallback( void *pvEventGroup, const uint32_t ulBitsToClear ) PRIVILEGED_FUNCTION;


#if (configUSE_TRACE_FACILITY == 1)
	UBaseType_t uxEventGroupGetNumber( void* xEventGroup ) PRIVILEGED_FUNCTION;
#endif

#ifdef __cplusplus
}
#endif

#endif
