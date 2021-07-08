#ifndef __QUEUEARRAY_H__
#define __QUEUEARRAY_H__

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

typedef enum TorF
{
    false,
    true
}RESULT;

typedef struct Data
{
    int id;
    /* add elements in here */
}QItem;

typedef struct Queue
{
    int head;
    int tail;
    QItem *item;
    int QLen;   /* set this before init a Q */
                /* QLen-1 is the useful size */
}Queue;

RESULT InitQueue(Queue *Q);
RESULT DestroyQueue(Queue *Q);
RESULT ClearQueue(Queue *Q);
RESULT QueueEmpty(Queue *Q);
RESULT QueueFull(Queue *Q);
int    QueueLength(Queue *Q);
RESULT EnQueue(Queue *Q, QItem *Item);
RESULT DeQueue(Queue *Q, QItem *Item);

#endif
