/// Copyright (C) StrawberryHacker

#ifndef DLIST_H
#define DLIST_H

#include "types.h"

/// Double linked generic list interface

struct dlist_node {
    struct dlist_node* prev;
    struct dlist_node* next;

    void* obj;
};

struct dlist {
    struct dlist_node* first;
    struct dlist_node* last;

    u32 size;
};

u32 dlist_search(struct dlist_node* node, struct dlist* list);

void dlist_insert_first(struct dlist_node* node, struct dlist* list);

void dlist_insert_last(struct dlist_node* node, struct dlist* list);

void dlist_insert_after(struct dlist_node* i_node, struct dlist_node* a_node,
    struct dlist* list);

void dlist_insert_before(struct dlist_node* i_node, struct dlist_node* a_node,
    struct dlist* list);

struct dlist_node* dlist_remove_first(struct dlist* list);

struct dlist_node* dlist_remove_last(struct dlist* list);

struct dlist_node* dlist_remove(struct dlist_node* node, struct dlist* list);

#endif
