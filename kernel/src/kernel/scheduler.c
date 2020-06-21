#include "scheduler.h"
#include "nvic.h"
#include "systick.h"
#include "cpu.h"
#include "panic.h"
#include "debug.h"

#include <stddef.h>

volatile struct tcb* curr_thread;
volatile struct tcb* next_thread;

void sched_run(void);
void scheduler(void);

extern struct tcb* thread_add(const char* name, void(*thread_ptr)(void* arg),
	u32 stack_size, u32* arg);


extern struct sched_class rt_class;
extern struct sched_class app_class;
extern struct sched_class background_class;
extern struct sched_class idle_class;

static void idle_thread(void* arg) {
	debug_print("xD ");
	while (1) {
		// Do nothing really
	}
}

void scheduler_start(void) {
	// Systick interrupt should be disabled
	systick_set_priority(NVIC_PRI_6);
	pendsv_set_priority(NVIC_PRI_7);
	
	// Add the IDLE to the list
	struct tcb* idle = thread_add("IDLE", idle_thread, 32, NULL);

	debug_print("OK\n");
	debug_flush();

	idle_class.enqueue(idle);

	cpsid_f();
	systick_set_rvr(300000);
	systick_enable(1);

	scheduler();

	if (idle != (struct tcb *)next_thread) {
		panic("Something is wrong");
	}
	debug_print("IDLE: \t%4h\nNext: \t%4h\n", idle, next_thread);
	// Update the first thread to run

	next_thread = idle;
	curr_thread = idle;

	sched_run();
}

void systick_handler(void) {
	
	//scheduler();
	next_thread = curr_thread;
	
	// Pend the PendSV handler
	pendsv_set_pending();
}

void scheduler(void) {
	// This is the scheduler	

	next_thread = rt_class.pick_thread();
	if (next_thread == NULL) {
		// The real-time scheduler runqueue is empty
		next_thread = app_class.pick_thread();
		if (next_thread == NULL) {
			// No application threads need to be scheduled
			next_thread = background_class.pick_thread();
			if (next_thread == NULL) {
				// No background threads need to run
				next_thread = idle_class.pick_thread();
				if (next_thread == NULL) {
					panic("No threads can run");
				}
			}
		}
	}
}
