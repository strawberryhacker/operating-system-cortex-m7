/* Copyright (C) StrawberryHacker */

/*
 * This list interface will implement a double linked circular list. The size
 * of the list is therefore not included. The difference between this list
 * interface and the dlist.h is the way the list includes the object. In the 
 * dlist.h implementation every list node contains a poitner to the object. 
 * This introduces many bug sources if one forgets to initialize the object when
 * creating the list node. 
 * 
 * This list interface uses the list_node struct offsett to compute the object
 * pointer.
 */

#ifndef LIST_H
#define LIST_H

#include "types.h"
#include "print.h"
#include <stddef.h>

/*
 * This structure will represent both a list and a list node. This implies that
 * the list will be circular
 */
struct list_node {
    struct list_node* next;
    struct list_node* prev;
};

/*
 * Returns a pointer to the struct entry in which the list is embedded
 */
#define list_get_entry(node, type, member) \
    (type *)((u8 *)node - offsetof(type, member))

/*
 * Initializes a list
 */
static inline void list_init(struct list_node* list) 
{
    list->prev = list;
    list->next = list;
}

static inline void list_node_init(struct list_node* list)
{
    /* TODO: add MPU protected address, issue #50 */
    list->next = NULL;
    list->prev = NULL;
}

/*
 * Inserts a list node between two consecutive entries
 */
static inline void _list_add(struct list_node* new, struct list_node* prev,
    struct list_node* next)
{
    prev->next = new;
    next->prev = new;
    new->next = next;
    new->prev = prev;
}

/*
 * Delets a node from the list between two consecutive entries
 */
static inline void _list_delete(struct list_node* prev, struct list_node* next)
{
    prev->next = next;
    next->prev = prev;
}

/*
 * Inserts a list node first in the list
 */
static inline void list_add_first(struct list_node* new, struct list_node* list)
{
    _list_add(new, list, list->next);
}

/*
 * Inserts a list node last in the list
 */
static inline void list_add_last(struct list_node* new, struct list_node* list)
{
    _list_add(new, list->prev, list);
}

/*
 * Deletes a node in the list
 */
static inline void list_delete_node(struct list_node* node)
{
    _list_delete(node->prev, node->next);

    /* TODO: add MPU protected address, issue #50 */
    node->next = NULL;
    node->prev = NULL;
}

/*
 * Deletes the first node in the list
 */
static inline void list_delete_first(struct list_node* list)
{
    if (list->next == list) {
        return;
    }
    list_delete_node(list->next);
}

/*
 * Deletes the last node in the list
 */
static inline void list_delete_last(struct list_node* list)
{
    if (list->next == list) {
        return;
    }
    list_delete_node(list->prev);
}

/*
 * Defines for iterating lists
 */
#define list_iterate(node, list) \
    for (node = (list)->next; node != (list); node = node->next)

#endif