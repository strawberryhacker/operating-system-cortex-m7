/// Copyright (C) StrawberryHacker

#include "scheduler.h"
#include "nvic.h"
#include "systick.h"
#include "cpu.h"
#include "panic.h"
#include "print.h"

#include <stddef.h>

volatile struct thread* curr_thread;
volatile struct thread* next_thread;

void scheduler_run(void);
struct thread* core_scheduler(void);

extern struct thread* new_thread(struct thread_info* thread_info);

volatile u32 switches = 0;

static void idle_thread(void* arg) {
	print("xD ");
	while (1) {
		for (volatile u32 i = 0; i < 100000; i++) {
			asm volatile ("nop");
		}
		print("S: %d\n", switches);
	}
}

static struct thread* idle;

void scheduler_start(void) {
	// Systick interrupt should be disabled
	systick_set_priority(NVIC_PRI_6);
	pendsv_set_priority(NVIC_PRI_7);
	
	// Add the IDLE to the list
	struct thread_info idle_info = {
		.name       = "Idle",
		.stack_size = 100,
		.thread     = idle_thread,
		.arg        = NULL
	};
	idle = new_thread(&idle_info);

	print("OK\n");
	print_flush();

	idle_class.enqueue(idle);

	cpsid_f();
	systick_set_rvr(300000);
	systick_enable(1);

	// The `scheduler_run` does not care about the `curr_thread`. However it 
	// MUST be set in order for the cotext switch to work. If the `curr_thread`
	// is not set, this will give an hard fault.
	next_thread = core_scheduler();
	curr_thread = next_thread;

	if (next_thread != idle) {
		panic("Fuck");
	}
	scheduler_run();
}

void systick_handler(void) {
	
	next_thread = core_scheduler();
	if (next_thread != idle) {
		print("Error: %4h\n", next_thread);
		print_flush();
		panic("Fuck");
	}
	switches++;
	// Pend the PendSV handler
	pendsv_set_pending();
}

struct thread* core_scheduler(void) {
	// This is the core scheduler
	struct thread* thread;

	// The first scheduling class is the real-time class
	const struct scheduling_class* class = &rt_class;

	// Go through all the scheduling classes
	for (class = &rt_class; class != NULL; class = class->next) {
		thread = class->pick_thread();

		if (thread) {
			return thread;
		}
	}
	panic("Core scheduler error");
	
	return NULL;
}
