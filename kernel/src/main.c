/// Copyright (C) StrawberryHacker

#include "types.h"
#include "scheduler.h"
#include "kernel_entry.h"
#include "print.h"
#include "dlist.h"
#include "mm.h"

#include <stddef.h>

void print_dlist(struct dlist* list) {
	struct dlist_node* iter = list->first;
	print("\n");
	print("First: %4h\tLast: %4h\n", list->first, list->last);
	while (iter) {
		print("Addr: %4h\tNext: %4h\tPrev: %4h\tObj: %d\n", iter, iter->next,
			iter->prev, iter->obj);

		iter = iter->next;
	}
}

int main(void) {
	kernel_entry();

	struct dlist dlist = { .first = NULL, .last = NULL, .size = 0 };

	struct dlist_node* nodes = mm_alloc(sizeof(struct dlist_node) * 10, SRAM);
	print_dlist(&dlist);
	nodes[6].obj = 6;
	nodes[1].obj = 1;
	dlist_insert_last(&nodes[6], &dlist);
	print_dlist(&dlist);

	dlist_insert_before(&nodes[1], &nodes[6], &dlist);
	print_dlist(&dlist);

	while (1);
	scheduler_start();
}
