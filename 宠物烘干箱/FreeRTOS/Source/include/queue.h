#ifndef QUEUE_H
#define QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"

typedef void * QueueHandle_t;

#define queueSEND_TO_BACK    ( ( BaseType_t ) 0 )
#define queueSEND_TO_FRONT   ( ( BaseType_t ) 1 )

QueueHandle_t xQueueCreate( UBaseType_t uxQueueLength, UBaseType_t uxItemSize );

BaseType_t xQueueSend( QueueHandle_t xQueue, const void * pvItemToQueue, TickType_t xTicksToWait );

BaseType_t xQueueSendToFront( QueueHandle_t xQueue, const void * pvItemToQueue, TickType_t xTicksToWait );

BaseType_t xQueueSendToBack( QueueHandle_t xQueue, const void * pvItemToQueue, TickType_t xTicksToWait );

BaseType_t xQueueReceive( QueueHandle_t xQueue, void * pvBuffer, TickType_t xTicksToWait );

BaseType_t xQueuePeek( QueueHandle_t xQueue, void * pvBuffer, TickType_t xTicksToWait );

UBaseType_t uxQueueMessagesWaiting( QueueHandle_t xQueue );

BaseType_t xQueueIsQueueEmptyFromISR( QueueHandle_t xQueue );

BaseType_t xQueueIsQueueFullFromISR( QueueHandle_t xQueue );

void vQueueDelete( QueueHandle_t xQueue );

#define xSemaphoreCreateBinary()     xQueueCreate( 1, 0 )
#define xSemaphoreCreateMutex()      xQueueCreateMutex( queueQUEUE_TYPE_MUTEX )
#define xSemaphoreTake( xSemaphore, xBlockTime ) xQueueSemaphoreTake( ( xSemaphore ), ( xBlockTime ) )
#define xSemaphoreGive( xSemaphore ) xQueueGenericSend( ( xSemaphore ), NULL, ( TickType_t ) 0U, queueSEND_TO_BACK )

#ifdef __cplusplus
}
#endif

#endif /* QUEUE_H */
