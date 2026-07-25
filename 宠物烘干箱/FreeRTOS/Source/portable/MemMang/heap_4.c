#include "FreeRTOS.h"
#include "task.h"

#define heapSTRUCT_SIZE ( ( size_t ) ( sizeof( BlockLink_t ) + portBYTE_ALIGNMENT - 1 ) & ~( portBYTE_ALIGNMENT - 1 ) )
#define heapMINIMUM_BLOCK_SIZE ( ( size_t ) ( heapSTRUCT_SIZE + ( 2 * portBYTE_ALIGNMENT ) ) )

typedef struct A_BLOCK_LINK
{
    struct A_BLOCK_LINK *pxNextFreeBlock;
    size_t xBlockSize;
} BlockLink_t;

static BlockLink_t xStart;
static BlockLink_t *pxEnd = NULL;
static uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];

void *pvPortMalloc( size_t xWantedSize )
{
    BlockLink_t *pxBlock, *pxPreviousBlock, *pxNewBlockLink;
    void *pvReturn = NULL;

    if( xWantedSize > 0 )
    {
        xWantedSize += portBYTE_ALIGNMENT;

        if( ( xWantedSize & portBYTE_ALIGNMENT_MASK ) != 0x00 )
        {
            xWantedSize += ( portBYTE_ALIGNMENT - ( xWantedSize & portBYTE_ALIGNMENT_MASK ) );
        }
    }

    vTaskSuspendAll();
    {
        for( pxPreviousBlock = &xStart, pxBlock = xStart.pxNextFreeBlock; pxBlock != NULL; pxPreviousBlock = pxBlock, pxBlock = pxBlock->pxNextFreeBlock )
        {
            if( pxBlock->xBlockSize >= xWantedSize )
            {
                pvReturn = ( void * ) ( ( ( uint8_t * ) pxPreviousBlock->pxNextFreeBlock ) + heapSTRUCT_SIZE );

                pxPreviousBlock->pxNextFreeBlock = pxBlock->pxNextFreeBlock;

                if( pxBlock->xBlockSize > xWantedSize + heapMINIMUM_BLOCK_SIZE )
                {
                    pxNewBlockLink = ( void * ) ( ( ( uint8_t * ) pxBlock ) + xWantedSize );
                    pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
                    pxBlock->xBlockSize = xWantedSize;
                    pxPreviousBlock->pxNextFreeBlock = pxNewBlockLink;
                    pxNewBlockLink->pxNextFreeBlock = pxBlock->pxNextFreeBlock;
                }

                break;
            }
        }
    }
    xTaskResumeAll();

    return pvReturn;
}

void vPortFree( void *pv )
{
    uint8_t *puc = ( uint8_t * ) pv;
    BlockLink_t *pxLink;

    if( pv != NULL )
    {
        puc -= heapSTRUCT_SIZE;
        pxLink = ( void * ) puc;

        vTaskSuspendAll();
        {
            pxLink->pxNextFreeBlock = xStart.pxNextFreeBlock;
            xStart.pxNextFreeBlock = pxLink;
        }
        xTaskResumeAll();
    }
}

void vPortInitialiseBlocks( void )
{
    BlockLink_t *pxFirstFreeBlock;
    uint8_t *pucAlignedHeap;

    pucAlignedHeap = ( uint8_t * ) ( ( ( portPOINTER_SIZE_TYPE ) ucHeap + portBYTE_ALIGNMENT - 1 ) & ~( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK );

    xStart.pxNextFreeBlock = ( void * ) pucAlignedHeap;
    xStart.xBlockSize = ( size_t ) 0;

    pxFirstFreeBlock = ( void * ) pucAlignedHeap;
    pxFirstFreeBlock->xBlockSize = configTOTAL_HEAP_SIZE - ( ( size_t ) ( pucAlignedHeap - ucHeap ) );
    pxFirstFreeBlock->pxNextFreeBlock = pxEnd;
}

#define portBYTE_ALIGNMENT_MASK ( portBYTE_ALIGNMENT - 1 )
#define portPOINTER_SIZE_TYPE ( sizeof( void * ) )
