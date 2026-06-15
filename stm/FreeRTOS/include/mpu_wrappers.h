/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        mpu_wrappers.h
 *
 * Description:  mpu_wrappers.h module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#ifndef MPU_WRAPPERS_H
#define MPU_WRAPPERS_H


#ifdef portUSING_MPU_WRAPPERS


	#ifndef MPU_WRAPPERS_INCLUDED_FROM_API_FILE


		#define xTaskCreate								MPU_xTaskCreate
		#define xTaskCreateStatic						MPU_xTaskCreateStatic
		#define xTaskCreateRestricted					MPU_xTaskCreateRestricted
		#define vTaskAllocateMPURegions					MPU_vTaskAllocateMPURegions
		#define vTaskDelete								MPU_vTaskDelete
		#define vTaskDelay								MPU_vTaskDelay
		#define vTaskDelayUntil							MPU_vTaskDelayUntil
		#define xTaskAbortDelay							MPU_xTaskAbortDelay
		#define uxTaskPriorityGet						MPU_uxTaskPriorityGet
		#define eTaskGetState							MPU_eTaskGetState
		#define vTaskGetInfo							MPU_vTaskGetInfo
		#define vTaskPrioritySet						MPU_vTaskPrioritySet
		#define vTaskSuspend							MPU_vTaskSuspend
		#define vTaskResume								MPU_vTaskResume
		#define vTaskSuspendAll							MPU_vTaskSuspendAll
		#define xTaskResumeAll							MPU_xTaskResumeAll
		#define xTaskGetTickCount						MPU_xTaskGetTickCount
		#define uxTaskGetNumberOfTasks					MPU_uxTaskGetNumberOfTasks
		#define pcTaskGetName							MPU_pcTaskGetName
		#define xTaskGetHandle							MPU_xTaskGetHandle
		#define uxTaskGetStackHighWaterMark				MPU_uxTaskGetStackHighWaterMark
		#define vTaskSetApplicationTaskTag				MPU_vTaskSetApplicationTaskTag
		#define xTaskGetApplicationTaskTag				MPU_xTaskGetApplicationTaskTag
		#define vTaskSetThreadLocalStoragePointer		MPU_vTaskSetThreadLocalStoragePointer
		#define pvTaskGetThreadLocalStoragePointer		MPU_pvTaskGetThreadLocalStoragePointer
		#define xTaskCallApplicationTaskHook			MPU_xTaskCallApplicationTaskHook
		#define xTaskGetIdleTaskHandle					MPU_xTaskGetIdleTaskHandle
		#define uxTaskGetSystemState					MPU_uxTaskGetSystemState
		#define vTaskList								MPU_vTaskList
		#define vTaskGetRunTimeStats					MPU_vTaskGetRunTimeStats
		#define xTaskGenericNotify						MPU_xTaskGenericNotify
		#define xTaskNotifyWait							MPU_xTaskNotifyWait
		#define ulTaskNotifyTake						MPU_ulTaskNotifyTake
		#define xTaskNotifyStateClear					MPU_xTaskNotifyStateClear

		#define xTaskGetCurrentTaskHandle				MPU_xTaskGetCurrentTaskHandle
		#define vTaskSetTimeOutState					MPU_vTaskSetTimeOutState
		#define xTaskCheckForTimeOut					MPU_xTaskCheckForTimeOut
		#define xTaskGetSchedulerState					MPU_xTaskGetSchedulerState


		#define xQueueGenericSend						MPU_xQueueGenericSend
		#define xQueueGenericReceive					MPU_xQueueGenericReceive
		#define uxQueueMessagesWaiting					MPU_uxQueueMessagesWaiting
		#define uxQueueSpacesAvailable					MPU_uxQueueSpacesAvailable
		#define vQueueDelete							MPU_vQueueDelete
		#define xQueueCreateMutex						MPU_xQueueCreateMutex
		#define xQueueCreateMutexStatic					MPU_xQueueCreateMutexStatic
		#define xQueueCreateCountingSemaphore			MPU_xQueueCreateCountingSemaphore
		#define xQueueCreateCountingSemaphoreStatic		MPU_xQueueCreateCountingSemaphoreStatic
		#define xQueueGetMutexHolder					MPU_xQueueGetMutexHolder
		#define xQueueTakeMutexRecursive				MPU_xQueueTakeMutexRecursive
		#define xQueueGiveMutexRecursive				MPU_xQueueGiveMutexRecursive
		#define xQueueGenericCreate						MPU_xQueueGenericCreate
		#define xQueueGenericCreateStatic				MPU_xQueueGenericCreateStatic
		#define xQueueCreateSet							MPU_xQueueCreateSet
		#define xQueueAddToSet							MPU_xQueueAddToSet
		#define xQueueRemoveFromSet						MPU_xQueueRemoveFromSet
		#define xQueueSelectFromSet						MPU_xQueueSelectFromSet
		#define xQueueGenericReset						MPU_xQueueGenericReset

		#if( configQUEUE_REGISTRY_SIZE > 0 )
			#define vQueueAddToRegistry						MPU_vQueueAddToRegistry
			#define vQueueUnregisterQueue					MPU_vQueueUnregisterQueue
			#define pcQueueGetName							MPU_pcQueueGetName
		#endif


		#define xTimerCreate							MPU_xTimerCreate
		#define xTimerCreateStatic						MPU_xTimerCreateStatic
		#define pvTimerGetTimerID						MPU_pvTimerGetTimerID
		#define vTimerSetTimerID						MPU_vTimerSetTimerID
		#define xTimerIsTimerActive						MPU_xTimerIsTimerActive
		#define xTimerGetTimerDaemonTaskHandle			MPU_xTimerGetTimerDaemonTaskHandle
		#define xTimerPendFunctionCall					MPU_xTimerPendFunctionCall
		#define pcTimerGetName							MPU_pcTimerGetName
		#define xTimerGetPeriod							MPU_xTimerGetPeriod
		#define xTimerGetExpiryTime						MPU_xTimerGetExpiryTime
		#define xTimerGenericCommand					MPU_xTimerGenericCommand


		#define xEventGroupCreate						MPU_xEventGroupCreate
		#define xEventGroupCreateStatic					MPU_xEventGroupCreateStatic
		#define xEventGroupWaitBits						MPU_xEventGroupWaitBits
		#define xEventGroupClearBits					MPU_xEventGroupClearBits
		#define xEventGroupSetBits						MPU_xEventGroupSetBits
		#define xEventGroupSync							MPU_xEventGroupSync
		#define vEventGroupDelete						MPU_vEventGroupDelete


		#define PRIVILEGED_FUNCTION

	#else


		#define PRIVILEGED_FUNCTION __attribute__((section("privileged_functions")))
		#define PRIVILEGED_DATA __attribute__((section("privileged_data")))

	#endif

#else

	#define PRIVILEGED_FUNCTION
	#define PRIVILEGED_DATA
	#define portUSING_MPU_WRAPPERS 0

#endif


#endif
