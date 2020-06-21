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
	scheduler_start();
}
