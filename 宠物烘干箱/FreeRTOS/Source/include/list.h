#ifndef LIST_H
#define LIST_H

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"

typedef struct xLIST_ITEM
{
    TickType_t xItemValue;
    struct xLIST_ITEM *pxNext;
    struct xLIST_ITEM *pxPrevious;
    void *pvOwner;
    struct xLIST *pxContainer;
} ListItem_t;

typedef struct xMINI_LIST_ITEM
{
    TickType_t xItemValue;
    struct xLIST_ITEM *pxNext;
    struct xLIST_ITEM *pxPrevious;
} MiniListItem_t;

typedef struct xLIST
{
    UBaseType_t uxNumberOfItems;
    ListItem_t *pxIndex;
    MiniListItem_t xListEnd;
} List_t;

#define listSET_LIST_ITEM_OWNER( pxListItem, pxOwner )    ( pxListItem )->pvOwner = ( void * ) ( pxOwner )
#define listGET_LIST_ITEM_OWNER( pxListItem )             ( pxListItem )->pvOwner
#define listSET_LIST_ITEM_VALUE( pxListItem, xValue )     ( pxListItem )->xItemValue = ( xValue )
#define listGET_LIST_ITEM_VALUE( pxListItem )             ( pxListItem )->xItemValue
#define listGET_NEXT( pxListItem )                        ( pxListItem )->pxNext
#define listGET_END_MARKER( pxList )                      ( &( ( pxList )->xListEnd ) )
#define listGET_HEAD_ENTRY( pxList )                      ( ( pxList )->xListEnd.pxNext )
#define listGET_NEXT_ENTRY( pxIterator )                  ( ( pxIterator )->pxNext )

void vListInitialise( List_t *pxList );
void vListInitialiseItem( ListItem_t *pxItem );
void vListInsertEnd( List_t *pxList, ListItem_t *pxNewListItem );
void vListInsert( List_t *pxList, ListItem_t *pxNewListItem );
UBaseType_t uxListRemove( ListItem_t *pxItemToRemove );

#ifdef __cplusplus
}
#endif

#endif /* LIST_H */
