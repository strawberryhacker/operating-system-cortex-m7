#include "sched.h"
#include "nvic.h"
#include "systick.h"
#include "cpu.h"

volatile struct tcb* curr_thread;
volatile struct tcb* next_thread;

void sched_run(void);

volatile u8 thread_index = 0;

extern struct tcb* threads[5];

void sched_start(void) {
	// Systick interrupt should be disabled
	systick_set_priority(NVIC_PRI_6);
	pendsv_set_priority(NVIC_PRI_7);
	
	// Pick with first thread of course
	next_thread = threads[1];
	curr_thread = threads[1];
	
	cpsid_f();
	systick_set_rvr(300000);
	systick_enable(1);
	
	// Update the first thread to run
	sched_run();
}



void systick_handler(void) {
	// This is the scheduler
	next_thread = threads[thread_index++];

	
	if (thread_index > 1) {
		thread_index = 0;
	}
	
	// Pend the PendSV handler
	pendsv_set_pending();
}
