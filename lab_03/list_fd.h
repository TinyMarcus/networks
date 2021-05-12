#ifndef __LIST_FD_H__
#define __LIST_FD_H__

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

typedef struct node
{
    int fd;
    struct node *next;
} node_t;

void push(node_t **head, int fd);
node_t* pop(node_t **head);
void free_lst(node_t **head);

#endif