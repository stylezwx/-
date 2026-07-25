#include "FreeRTOS.h"
#include "list.h"

void vListInitialise( List_t *pxList )
{
    pxList->pxIndex = ( ListItem_t * ) &( pxList->xListEnd );
    pxList->xListEnd.pxNext = ( ListItem_t * ) &( pxList->xListEnd );
    pxList->xListEnd.pxPrevious = ( ListItem_t * ) &( pxList->xListEnd );
    pxList->xListEnd.xItemValue = portMAX_DELAY;
    pxList->uxNumberOfItems = ( UBaseType_t ) 0U;
}

void vListInitialiseItem( ListItem_t *pxItem )
{
    pxItem->pxContainer = NULL;
}

void vListInsertEnd( List_t *pxList, ListItem_t *pxNewListItem )
{
    ListItem_t *pxIndex = pxList->pxIndex;

    pxNewListItem->pxNext = pxIndex;
    pxNewListItem->pxPrevious = pxIndex->pxPrevious;
    pxIndex->pxPrevious->pxNext = pxNewListItem;
    pxIndex->pxPrevious = pxNewListItem;

    pxNewListItem->pxContainer = pxList;

    pxList->uxNumberOfItems++;
}

void vListInsert( List_t *pxList, ListItem_t *pxNewListItem )
{
    ListItem_t *pxIterator;
    TickType_t xValueOfInsertion = pxNewListItem->xItemValue;

    if( xValueOfInsertion == portMAX_DELAY )
    {
        pxIterator = pxList->xListEnd.pxPrevious;
    }
    else
    {
        for( pxIterator = ( ListItem_t * ) &( pxList->xListEnd ); pxIterator->pxNext->xItemValue <= xValueOfInsertion; pxIterator = pxIterator->pxNext )
        {
        }
    }

    pxNewListItem->pxNext = pxIterator->pxNext;
    pxNewListItem->pxNext->pxPrevious = pxNewListItem;
    pxNewListItem->pxPrevious = pxIterator;
    pxIterator->pxNext = pxNewListItem;

    pxNewListItem->pxContainer = pxList;

    pxList->uxNumberOfItems++;
}

UBaseType_t uxListRemove( ListItem_t *pxItemToRemove )
{
    List_t *pxList = pxItemToRemove->pxContainer;

    pxItemToRemove->pxNext->pxPrevious = pxItemToRemove->pxPrevious;
    pxItemToRemove->pxPrevious->pxNext = pxItemToRemove->pxNext;

    if( pxList->pxIndex == pxItemToRemove )
    {
        pxList->pxIndex = pxItemToRemove->pxPrevious;
    }

    pxItemToRemove->pxContainer = NULL;
    pxList->uxNumberOfItems--;

    return pxList->uxNumberOfItems;
}
