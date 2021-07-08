#include <stdio.h>
#include <stdlib.h>
#include "queueLinkList.h"

int main(int argc, int **argv)
{
    QNode node;

    Queue *Q = (Queue *)malloc(sizeof(Queue));

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

    for(int i=0; i<5; i++)
    {
        node.id = i;
        res = EnQueue(Q, &node);
        if(!res)
        {
            printf("The queue is full, times = %d\n", i+1);
            break;
        }
    }

    for(int i=0; i<10; i++)
    {
        res = DeQueue(Q, &node);
        if(res)
            printf("get the head value = %d\n", node.id);
        else
        {
            printf("The queue is empty\n");
            break;
        }
    }

    return 0;
}
