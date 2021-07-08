#include "queueLinkList.h"

RESULT InitQueue(Queue *Q)
{
    assert(Q != NULL);

    Q->head = Q->tail = (QNode *)malloc(sizeof(QNode));
    if(!Q->head)
    {
        printf("%s-%d:malloc error\n", __FILE__,__LINE__);
        return false;
    }
    Q->tail->next = NULL;

    return true;
}

RESULT isEmpty(Queue *Q)
{
    assert(Q != NULL);

    if(Q->tail == Q->head)
        return true;
    else
        return false;
}

RESULT EnQueue(Queue *Q, QNode *node)
{
    assert(Q != NULL);
    assert(node != NULL);
    
    Q->tail->next = (QNode *)malloc(sizeof(QNode));
    if(!Q->tail->next)
        return false;
    Q->tail = Q->tail->next;

    // start to operate the new node;
    Q->tail->next = NULL;
    Q->tail->id = node->id;
    // add opeartions in here
    
    return true;
}

RESULT DeQueue(Queue *Q, QNode *node)
{
    assert(Q != NULL);
    assert(node != NULL);

    if(!isEmpty(Q))
    {
        QNode *temp = Q->head->next;// get the target node
        // start to operate the target node
        node->id = temp->id;
        // add operations in here

        // if the node is last one, let tail=head in case
        // lose tail
        if(Q->tail == Q->head->next)
            Q->tail = Q->head;

        // then release the node
        Q->head->next = temp->next;
        free(temp);
        temp = NULL;

        return true;
    }
    else
        return false;

}
