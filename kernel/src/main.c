/// Copyright (C) StrawberryHacker
#include "scheduler.h"
#include "kernel_entry.h"

int main(void) {
	kernel_entry();
	scheduler_start();
}
