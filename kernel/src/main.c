/// Copyright (C) StrawberryHacker

#include "scheduler.h"
#include "kernel_entry.h"
#include "print.h"

int main(void) {
	kernel_entry();

	

	while (1);
	scheduler_start();
}
