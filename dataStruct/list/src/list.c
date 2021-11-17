#include "list.h"
#include <stdlib.h>

LIST* create_list()
{
    LIST *head = NULL;
    return head;
}

void add_node_list(LIST *head, void *srcdata)
{
    LIST *node = NULL;
    node = (LIST*)malloc(sizeof(LIST));
    node->data = srcdata;
    node->next = head->next;
    head->next = node;

    node = NULL;
}

void delete_node_list(LIST *head, void *targetdata)
{
    LIST *p = head;
    while(p != NULL)
    {
        if(p->data = )
    }
}
