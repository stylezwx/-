#ifndef TASK_H
#define TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"

typedef void (*TaskFunction_t)( void * );

typedef struct tskTaskControlBlock
{
    volatile StackType_t *pxTopOfStack;
    ListItem_t xStateListItem;
    ListItem_t xEventListItem;
    UBaseType_t uxPriority;
    StackType_t *pxStack;
    char pcTaskName[ configMAX_TASK_NAME_LEN ];
    #if( ( configUSE_MUTEXES == 1 ) || ( configUSE_RECURSIVE_MUTEXES == 1 ) )
        UBaseType_t uxBasePriority;
        UBaseType_t uxMutexesHeld;
    #endif
} tskTCB;

typedef tskTCB * TaskHandle_t;

#define tskIDLE_PRIORITY          ( ( UBaseType_t ) 0 )
#define pdTRUE                    ( ( BaseType_t ) 1 )
#define pdFALSE                   ( ( BaseType_t ) 0 )
#define pdPASS                    ( pdTRUE )
#define pdFAIL                    ( pdFALSE )

#define portMAX_DELAY             ( TickType_t ) 0xffffffffUL

#define vTaskDelay( xTicksToDelay ) vTaskDelay( xTicksToDelay )
#define vTaskDelayUntil( pxPreviousWakeTime, xTimeIncrement ) vTaskDelayUntil( pxPreviousWakeTime, xTimeIncrement )

TaskHandle_t xTaskCreate( TaskFunction_t pxTaskCode, const char * const pcName, const configSTACK_DEPTH_TYPE usStackDepth, void * const pvParameters, UBaseType_t uxPriority, TaskHandle_t * const pxCreatedTask );

void vTaskDelete( TaskHandle_t xTaskToDelete );

void vTaskDelay( TickType_t xTicksToDelay );

void vTaskDelayUntil( TickType_t *pxPreviousWakeTime, TickType_t xTimeIncrement );

void vTaskSuspend( TaskHandle_t xTaskToSuspend );

void vTaskResume( TaskHandle_t xTaskToResume );

UBaseType_t uxTaskPriorityGet( TaskHandle_t xTask );

void vTaskPrioritySet( TaskHandle_t xTask, UBaseType_t uxNewPriority );

void vTaskStartScheduler( void );

void vTaskEndScheduler( void );

void vTaskYIELD( void );

#ifdef __cplusplus
}
#endif

#endif /* TASK_H */
