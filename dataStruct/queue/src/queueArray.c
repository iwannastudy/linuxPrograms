#include "queueArray.h"

RESULT InitQueue(Queue *Q)
{
    assert(Q != NULL);

    Q->item = (QItem *)malloc(sizeof(QItem) * Q->QLen);
    Q->head = Q->tail = 0;
    return true;
}

RESULT QueueEmpty(Queue *Q)
{
    assert(Q != NULL);

    if(Q->head == Q->tail)
        return true;
    else
        return false;
}

RESULT QueueFull(Queue *Q)
{
    assert(Q != NULL);

    if((Q->tail+1)%Q->QLen == Q->head)
        return true;
    else
        return false;
}

RESULT EnQueue(Queue *Q, QItem *Item)
{
    assert(Q != NULL);
    assert(Item != NULL);

    if(!QueueFull(Q))
    {
        Q->item[Q->tail].id = Item->id;
        Q->tail = (Q->tail+1)%Q->QLen;
        return true;
    }
    else
    {
        return false;
    }
}

RESULT DeQueue(Queue *Q, QItem *Item)
{
    assert(Q != NULL);
    assert(Item != NULL);

    if(!QueueEmpty(Q))
    {
       Item->id = Q->item[Q->head].id;
       Q->head = (Q->head+1)%Q->QLen;
       return true;
    }
    else
    {
        return false;
    }
}
