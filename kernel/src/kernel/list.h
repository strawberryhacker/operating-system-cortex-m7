/* Copyright (C) StrawberryHacker */

#ifndef LIST_H
#define LIST_H

#include "types.h"

struct list_node {
    struct list_node* next;
    void* obj;
};

struct list {
    struct list_node* first;
    u32 size;
};

void list_insert_first(struct list_node* node, struct list* list);

void list_insert_last(struct list_node* node, struct list* list);

struct list_node* list_remove_first(struct list* list);

struct list_node* list_remove_last(struct list* list);

#endif