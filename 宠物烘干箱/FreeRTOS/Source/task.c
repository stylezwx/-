#include "FreeRTOS.h"
#include "task.h"
#include "list.h"
#include "queue.h"

volatile unsigned long ulTaskNumber = 0;

static List_t pxReadyTasksLists[ configMAX_PRIORITIES ];
static List_t xDelayedTaskList1;
static List_t xDelayedTaskList2;
static List_t *pxDelayedTaskList;
static List_t *pxOverflowDelayedTaskList;
static ListItem_t xIdleTaskListItem;

static TaskHandle_t xCurrentTCB = NULL;
static TaskHandle_t xIdleTaskHandle = NULL;

static void prvAddTaskToReadyList( TaskHandle_t pxNewTCB );
static void prvInitialiseTaskLists( void );
static void prvIdleTask( void *pvParameters );
static void vTaskSwitchContext( void );

void vTaskStartScheduler( void )
{
    prvInitialiseTaskLists();

    xTaskCreate( prvIdleTask, "IDLE", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, &xIdleTaskHandle );

    xCurrentTCB = xIdleTaskHandle;

    if( xPortStartScheduler() != pdFALSE )
    {
    }
}

void vTaskEndScheduler( void )
{
    vPortEndScheduler();
}

void vTaskYIELD( void )
{
    portYIELD();
}

void vTaskDelay( TickType_t xTicksToDelay )
{
    TickType_t xTimeToWake;
    TaskHandle_t pxCurrentTCB = xCurrentTCB;

    vTaskSuspendAll();
    {
        xTimeToWake = xTaskGetTickCount() + xTicksToDelay;

        if( xTicksToDelay > ( TickType_t ) 0 )
        {
            vListRemove( &( pxCurrentTCB->xStateListItem ) );
            vListInsert( pxDelayedTaskList, &( pxCurrentTCB->xStateListItem ) );
        }
    }
    xTaskResumeAll();
}

void vTaskDelayUntil( TickType_t *pxPreviousWakeTime, TickType_t xTimeIncrement )
{
    TickType_t xTimeToWake;
    TaskHandle_t pxCurrentTCB = xCurrentTCB;

    vTaskSuspendAll();
    {
        xTimeToWake = *pxPreviousWakeTime + xTimeIncrement;

        if( xTaskGetTickCount() < *pxPreviousWakeTime )
        {
            while( xTaskGetTickCount() < *pxPreviousWakeTime )
            {
            }
        }

        *pxPreviousWakeTime = xTimeToWake;

        vListRemove( &( pxCurrentTCB->xStateListItem ) );
        vListInsert( pxDelayedTaskList, &( pxCurrentTCB->xStateListItem ) );
    }
    xTaskResumeAll();
}

void vTaskSuspend( TaskHandle_t xTaskToSuspend )
{
    vTaskSuspendAll();
    {
        vListRemove( &( xTaskToSuspend->xStateListItem ) );
    }
    xTaskResumeAll();
}

void vTaskResume( TaskHandle_t xTaskToResume )
{
    vTaskSuspendAll();
    {
        prvAddTaskToReadyList( xTaskToResume );
    }
    xTaskResumeAll();
}

UBaseType_t uxTaskPriorityGet( TaskHandle_t xTask )
{
    UBaseType_t uxReturn;

    vTaskSuspendAll();
    {
        uxReturn = xTask->uxPriority;
    }
    xTaskResumeAll();

    return uxReturn;
}

void vTaskPrioritySet( TaskHandle_t xTask, UBaseType_t uxNewPriority )
{
    vTaskSuspendAll();
    {
        if( xTask->uxPriority != uxNewPriority )
        {
            vListRemove( &( xTask->xStateListItem ) );
            xTask->uxPriority = uxNewPriority;
            prvAddTaskToReadyList( xTask );
        }
    }
    xTaskResumeAll();
}

void vTaskDelete( TaskHandle_t xTaskToDelete )
{
    vTaskSuspendAll();
    {
        vListRemove( &( xTaskToDelete->xStateListItem ) );
    }
    xTaskResumeAll();
}

TaskHandle_t xTaskCreate( TaskFunction_t pxTaskCode, const char * const pcName, const configSTACK_DEPTH_TYPE usStackDepth, void * const pvParameters, UBaseType_t uxPriority, TaskHandle_t * const pxCreatedTask )
{
    TaskHandle_t pxNewTCB;
    StackType_t *pxStack;

    pxStack = pvPortMalloc( ( ( ( size_t ) usStackDepth ) * sizeof( StackType_t ) ) );

    if( pxStack != NULL )
    {
        pxNewTCB = pvPortMalloc( sizeof( tskTCB ) );

        if( pxNewTCB != NULL )
        {
            pxNewTCB->pxStack = pxStack;
            pxNewTCB->pxTopOfStack = pxPortInitialiseStack( pxStack, pxTaskCode, pvParameters );

            for( int i = 0; i < configMAX_TASK_NAME_LEN; i++ )
            {
                pxNewTCB->pcTaskName[ i ] = pcName[ i ];
                if( pcName[ i ] == '\0' )
                    break;
            }
            pxNewTCB->pcTaskName[ configMAX_TASK_NAME_LEN - 1 ] = '\0';

            pxNewTCB->uxPriority = uxPriority;

            vListInitialiseItem( &( pxNewTCB->xStateListItem ) );
            listSET_LIST_ITEM_OWNER( &( pxNewTCB->xStateListItem ), pxNewTCB );

            vTaskSuspendAll();
            {
                prvAddTaskToReadyList( pxNewTCB );
            }
            xTaskResumeAll();

            if( pxCreatedTask != NULL )
            {
                *pxCreatedTask = pxNewTCB;
            }
        }
        else
        {
            vPortFree( pxStack );
            pxNewTCB = NULL;
        }
    }
    else
    {
        pxNewTCB = NULL;
    }

    return pxNewTCB;
}

static void prvAddTaskToReadyList( TaskHandle_t pxNewTCB )
{
    vListInsertEnd( &( pxReadyTasksLists[ pxNewTCB->uxPriority ] ), &( pxNewTCB->xStateListItem ) );
}

static void prvInitialiseTaskLists( void )
{
    UBaseType_t uxPriority;

    for( uxPriority = ( UBaseType_t ) 0U; uxPriority < ( UBaseType_t ) configMAX_PRIORITIES; uxPriority++ )
    {
        vListInitialise( &( pxReadyTasksLists[ uxPriority ] ) );
    }

    vListInitialise( &xDelayedTaskList1 );
    vListInitialise( &xDelayedTaskList2 );

    pxDelayedTaskList = &xDelayedTaskList1;
    pxOverflowDelayedTaskList = &xDelayedTaskList2;
}

static void prvIdleTask( void *pvParameters )
{
    for( ;; )
    {
        vTaskDelay( ( TickType_t ) 1 );
    }
}

void vTaskSwitchContext( void )
{
    UBaseType_t uxTopPriority = configMAX_PRIORITIES - 1;

    while( listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[ uxTopPriority ] ) ) == ( UBaseType_t ) 0 )
    {
        configASSERT( uxTopPriority );
        uxTopPriority--;
    }

    xCurrentTCB = ( TaskHandle_t ) listGET_OWNER_OF_NEXT_ENTRY( NULL, &( pxReadyTasksLists[ uxTopPriority ] ) );
}

TickType_t xTaskGetTickCount( void )
{
    static TickType_t xTickCount = 0;
    return xTickCount;
}

BaseType_t xTaskResumeAll( void )
{
    return pdTRUE;
}

void vTaskSuspendAll( void )
{
}
