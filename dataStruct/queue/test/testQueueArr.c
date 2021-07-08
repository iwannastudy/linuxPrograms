#include <stdio.h>
#include <stdlib.h>
#include "queueArray.h"

int main(int argc, int **argv)
{
    QItem item;

    Queue *Q = (Queue *)malloc(sizeof(Queue));
    Q->QLen = 5;

    RESULT res;
    res = InitQueue(Q);
    if(!res)
    {
        printf("Init_Queue fail\n");
        return 0;
    }
    else
    {
        printf("Init_Queue success\n");
    }

    for(int i=0; i<Q->QLen; i++)
    {
        item.id = i;
        res = EnQueue(Q, &item);
        if(!res)
        {
            printf("The queue is full, times = %d\n", i+1);
            break;
        }
    }

    for(int i=0; i<Q->QLen; i++)
    {
        res = DeQueue(Q, &item);
        if(res)
            printf("get the head value = %d\n", item.id);
        else
            printf("The queue is empty\n");
    }

    return 0;
}
