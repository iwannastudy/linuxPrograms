#ifndef __QUEUELINKLIST__H__
#define __QUEUELINKLIST__H__

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

typedef enum TorF
{
    false,
    true
}RESULT;

typedef struct QNode
{
    int id;
    /* add elements in here */
    struct QNode *next;
}QNode;

typedef struct Queue
{
    QNode *head;
    QNode *tail;
}Queue;


RESULT InitQueue(Queue *Q);
RESULT isEmpty(Queue *Q);
RESULT EnQueue(Queue *Q, QNode *node);
RESULT DeQueue(Queue *Q, QNode *node);

#endif
