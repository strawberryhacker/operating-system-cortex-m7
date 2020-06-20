#include "list.h"
#include "panic.h"

#include <stddef.h>

void list_insert_first(struct list_node* node, struct list* list) {
    node->next = list->first;
    list->first = node;
    list->size++;
}

void list_insert_last(struct list_node* node, struct list* list) {
    if (list->first != NULL) {
        struct list_node* iter = list->first;

        while (iter->next != NULL) {
            iter = iter->next;
        }

        // The `iter` is pointing to the last list element
        node->next = iter->next;
        iter->next = node;
    } else {
        node->next = list->first;
        list->first = node;
    }
    list->size++;
}

struct list_node* list_remove_first(struct list* list) {
    struct list_node* ret = NULL;
    if (list->first) {
        ret = list->first;
        list->first = list->first->next;
    } else {
        panic("List size is zero");
    }
    if (list->size == 0) {
        panic("List size is zero");
    }
    list->size--;

    return ret;
}

struct list_node* list_remove_last(struct list* list) {
    struct list_node* ret = NULL;

    if (list->first != NULL) {
        if (list->first->next != NULL) {
            struct list_node* curr = list->first->next;
            struct list_node* prev = list->first;
            while (curr->next != NULL) {
                prev = curr;
                curr = curr->next;
            }
            ret = curr;
            prev->next = NULL;
        } else {
            ret = list->first;
            list->first = NULL;
        }
    } else {
        panic("List size is zero");
    }
    if (list->size == 0) {
        panic("List size is zero");
    }
    list->size--;

    return ret;
}
