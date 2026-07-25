#include "FreeRTOS.h"
#include "queue.h"
#include "list.h"

typedef struct QueueDefinition
{
    int8_t *pcHead;
    int8_t *pcTail;
    int8_t *pcWriteTo;
    int8_t *pcReadFrom;
    List_t xTasksWaitingToSend;
    List_t xTasksWaitingToReceive;
    volatile UBaseType_t uxMessagesWaiting;
    UBaseType_t uxLength;
    UBaseType_t uxItemSize;
    volatile BaseType_t xRxLock;
    volatile BaseType_t xTxLock;
} xQUEUE;

QueueHandle_t xQueueCreate( UBaseType_t uxQueueLength, UBaseType_t uxItemSize )
{
    QueueHandle_t pxNewQueue;
    size_t xQueueSize;

    xQueueSize = ( size_t ) ( uxQueueLength * uxItemSize );
    pxNewQueue = pvPortMalloc( sizeof( xQUEUE ) );

    if( pxNewQueue != NULL )
    {
        pxNewQueue->pcHead = ( int8_t * ) pvPortMalloc( xQueueSize );

        if( pxNewQueue->pcHead != NULL )
        {
            pxNewQueue->uxLength = uxQueueLength;
            pxNewQueue->uxItemSize = uxItemSize;
            ( void ) xQueueGenericReset( pxNewQueue, pdTRUE );

            vListInitialise( &( pxNewQueue->xTasksWaitingToSend ) );
            vListInitialise( &( pxNewQueue->xTasksWaitingToReceive ) );
        }
        else
        {
            vPortFree( pxNewQueue );
            pxNewQueue = NULL;
        }
    }

    return pxNewQueue;
}

BaseType_t xQueueGenericReset( QueueHandle_t xQueue, BaseType_t xNewQueue )
{
    xQUEUE *pxQueue = ( xQUEUE * ) xQueue;

    pxQueue->pcTail = pxQueue->pcHead;
    pxQueue->pcWriteTo = pxQueue->pcHead;
    pxQueue->pcReadFrom = pxQueue->pcHead;
    pxQueue->uxMessagesWaiting = ( UBaseType_t ) 0U;
    pxQueue->xRxLock = queueUNLOCKED;
    pxQueue->xTxLock = queueUNLOCKED;

    ( void ) xNewQueue;

    return pdPASS;
}

BaseType_t xQueueSend( QueueHandle_t xQueue, const void * pvItemToQueue, TickType_t xTicksToWait )
{
    return xQueueGenericSend( xQueue, pvItemToQueue, xTicksToWait, queueSEND_TO_BACK );
}

BaseType_t xQueueSendToFront( QueueHandle_t xQueue, const void * pvItemToQueue, TickType_t xTicksToWait )
{
    return xQueueGenericSend( xQueue, pvItemToQueue, xTicksToWait, queueSEND_TO_FRONT );
}

BaseType_t xQueueSendToBack( QueueHandle_t xQueue, const void * pvItemToQueue, TickType_t xTicksToWait )
{
    return xQueueGenericSend( xQueue, pvItemToQueue, xTicksToWait, queueSEND_TO_BACK );
}

BaseType_t xQueueGenericSend( QueueHandle_t xQueue, const void * pvItemToQueue, TickType_t xTicksToWait, BaseType_t xCopyPosition )
{
    xQUEUE *pxQueue = ( xQUEUE * ) xQueue;
    BaseType_t xReturn = pdFAIL;

    vTaskSuspendAll();
    {
        if( pxQueue->uxMessagesWaiting < pxQueue->uxLength )
        {
            if( xCopyPosition == queueSEND_TO_FRONT )
            {
                pxQueue->pcWriteTo = pxQueue->pcReadFrom;
                pxQueue->pcReadFrom = ( pxQueue->pcReadFrom == pxQueue->pcHead ) ? ( pxQueue->pcHead + ( pxQueue->uxLength * pxQueue->uxItemSize ) ) : ( pxQueue->pcReadFrom - pxQueue->uxItemSize );
            }

            memcpy( ( void * ) pxQueue->pcWriteTo, pvItemToQueue, ( size_t ) pxQueue->uxItemSize );
            pxQueue->pcWriteTo += pxQueue->uxItemSize;

            if( pxQueue->pcWriteTo >= ( pxQueue->pcHead + ( pxQueue->uxLength * pxQueue->uxItemSize ) ) )
            {
                pxQueue->pcWriteTo = pxQueue->pcHead;
            }

            pxQueue->uxMessagesWaiting++;
            xReturn = pdPASS;
        }
    }
    xTaskResumeAll();

    return xReturn;
}

BaseType_t xQueueReceive( QueueHandle_t xQueue, void * pvBuffer, TickType_t xTicksToWait )
{
    xQUEUE *pxQueue = ( xQUEUE * ) xQueue;
    BaseType_t xReturn = pdFAIL;

    vTaskSuspendAll();
    {
        if( pxQueue->uxMessagesWaiting > ( UBaseType_t ) 0 )
        {
            memcpy( pvBuffer, ( void * ) pxQueue->pcReadFrom, ( size_t ) pxQueue->uxItemSize );
            pxQueue->pcReadFrom += pxQueue->uxItemSize;

            if( pxQueue->pcReadFrom >= ( pxQueue->pcHead + ( pxQueue->uxLength * pxQueue->uxItemSize ) ) )
            {
                pxQueue->pcReadFrom = pxQueue->pcHead;
            }

            pxQueue->uxMessagesWaiting--;
            xReturn = pdPASS;
        }
    }
    xTaskResumeAll();

    return xReturn;
}

UBaseType_t uxQueueMessagesWaiting( QueueHandle_t xQueue )
{
    xQUEUE *pxQueue = ( xQUEUE * ) xQueue;
    return pxQueue->uxMessagesWaiting;
}

void vQueueDelete( QueueHandle_t xQueue )
{
    xQUEUE *pxQueue = ( xQUEUE * ) xQueue;

    if( pxQueue != NULL )
    {
        if( pxQueue->pcHead != NULL )
        {
            vPortFree( pxQueue->pcHead );
        }
        vPortFree( pxQueue );
    }
}

#define queueUNLOCKED ( ( BaseType_t ) 0 )
