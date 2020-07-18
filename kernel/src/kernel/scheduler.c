/* Copyright (C) StrawberryHacker */

#include "scheduler.h"
#include "nvic.h"
#include "systick.h"
#include "cpu.h"
#include "panic.h"
#include "gpio.h"
#include "print.h"
#include "syscall.h"
#include "dlist.h"

#include <stddef.h>

/*
 * These are shared global variables with the `context.s` file which
 * uses these to switch threads. Before the context switch `curr_thread`
 * must point to the current running thread and `next_thread` must
 * point to the next thread to run. After the context switch 
 * `curr_thread` will point to the same thread as `next_thread`
 */
struct thread* curr_thread;
struct thread* next_thread;

/* Scheduler status tells if the core scheduler is allowed to run */
volatile u8 scheduler_status;

/* Main runqueue structure */
struct rq cpu_rq = {0};

/*
 * The tick variable holds the number of CPU cycles since program start.
 * The reason it doesn't count milliseconds is the time-error from
 * threads that reschedule early in their time frame
 */
volatile u64 tick = 0;

/*
 * The kernel calculates statistics per second basis. The `stats_tick`
 * are used to keep track of when to calculate these staticstics.
 */
volatile u64 stats_tick = 0;
volatile u32 reschedule_pending = 0;

/*
 * The function which is starting the scheduler is defined in contex.s
 * It sets up the stack for the first thread to run, switches from MSP
 * to PSP, and places the thread function in the LR. This will start
 * executing the first thread
 */
void scheduler_run(void);

extern tid_t new_thread(struct thread_info* thread_info);

/*
 * The `idle_thread` is enqueued by the scheduler into the idle
 * scheduling class
 */
static void idle_thread(void* arg) {
	while (1);
}

/*
 * This is the main core scheduler that picks the next thread to run
 * on the system. It query all the scheduling classes in priority
 * order and return the first thread given. The last scheduling class
 * `idle` must allways have a thread to offer.
 */
static struct thread* core_scheduler(void) {
	
	/* The highest priority scheduler is the real time scheduler */
	const struct scheduling_class* class;
	for (class = &rt_class; class != NULL; class = class->next) {
		struct thread* thread = class->pick_thread(&cpu_rq);

		if (thread != NULL) {
			return thread;
		}
	}
	panic("Core scheduler error");
	return NULL;
}

/*
 * Returns a thread based on its tid number
 */
struct thread* get_thread(struct rq *rq, tid_t tid) {
	struct dlist_node *it = rq->threads.first;

	while (it) {
		struct thread *thread = (struct thread *)it->obj;
		if (thread->tid == tid) {
			return thread;
		}
		it = it->next;
	}
	return NULL;
}

/*
 * Configures the SysTick and PendSV interrupt priorities, add the
 * idle thread and starts the scheduler
 */
void scheduler_start(void) {

	/* Systick interrupt should be disabled */
	systick_set_priority(NVIC_PRI_6);
	pendsv_set_priority(NVIC_PRI_7);
	svc_set_priority(NVIC_PRI_5);

	/* Allow the scheduler to run */
	scheduler_status = 1;
	cpu_rq.tick_to_wake = 0;
	
	/* Add the IDLE thread to the system */
	struct thread_info idle_info = {
		.name       = "Idle",
		.stack_size = 100,
		.thread     = idle_thread,
		.arg        = NULL,
		.class      = IDLE
	};

	/* Make the idle and test thread */
	tid_t idle_tid = new_thread(&idle_info);
	cpu_rq.idle = get_thread(&cpu_rq, idle_tid);

	/*
	 * The `scheduler_run` does not care about the `curr_thread`.
	 * However it MUST be set in order for the cotext switch to work.
	 * If the `curr_thread` is not set, this will give an hard fault.
	 */
	next_thread = core_scheduler();
	curr_thread = next_thread;

	/* Check if the idle thread is present */
	if (cpu_rq.idle == NULL) {
		panic("Idle not present");
	}

	/*
	 * Disable interrupt and enable the systick. When the `scheduler_run`
	 * assembly code turns on fault exceptions at the very end, systick
	 * exceptions will start to fire
	 */
	cpsid_f();
	systick_set_rvr(SYSTICK_RVR);
	systick_enable(1);

	/* Start executing the first thread */
	scheduler_run();
}

/*
 * Updates the first tick to wake basen on the sleep queue. If the
 * sleep queue is empty the `tick_to_wake` will be written to zero
 */
static void tick_to_wake_update(void) {
	/* Update the first tick to wake */
	struct dlist_node* tmp = cpu_rq.sleep_q.first;
	if (tmp) {
		cpu_rq.tick_to_wake = ((struct thread *)tmp->obj)->tick_to_wake;
	} else {
		cpu_rq.tick_to_wake = 0;
	}
}

static void process_expired_delays(void) {
	
	if ((cpu_rq.tick_to_wake <= tick) && cpu_rq.tick_to_wake) {
		/*
		 * Go over the delay queue and and enqueue all expired thread
		 * in their scheduling class
		 */
		struct dlist_node* iter = cpu_rq.sleep_q.first;

		while (iter) {
			u64 tick_to_wake = ((struct thread *)iter->obj)->tick_to_wake;
			if (tick_to_wake < tick) {

				/* Remove the thread for the delay queue */
				dlist_remove(iter, &cpu_rq.sleep_q);
				((struct thread *)iter->obj)->rq_list = NULL;

				/* Place the thread back into the running list */
				struct thread* t = (struct thread *)iter->obj;
				t->class->enqueue(t, &cpu_rq);
				t->tick_to_wake = 0;
			} else {
				/*
				 * The tick to wake is higher than the current kernel tick.
				 * It should not be removed from the sleep_q
				 */
				break;
			}
			iter = iter->next;
		}
		tick_to_wake_update();
	}
}

/* 
 * Remove a thread completly from the system. This includes deleting
 * the code and the thread control block. It will also remove the
 * thread from all list. This MUST be called in the scheduler.
 */
static void scheduler_remove_thread(struct thread* thread) {

	/* Remove the threads from all the lists */
	dlist_remove(&curr_thread->thread_node, &cpu_rq.threads);

	if (curr_thread->rq_list) {
		dlist_remove(&curr_thread->rq_node, curr_thread->rq_list);
	}

	/* Verify that the thread does not exits in any list */
	if ((curr_thread->rq_node.next != NULL) || 
	    (curr_thread->rq_node.prev != NULL) ||
		(curr_thread->thread_node.next != NULL) ||
		(curr_thread->thread_node.prev != NULL)) {
		panic("Exiting thread exist in a list");
	}

	/* Delete the threads memory footprint */
	if (curr_thread->code_addr) {
		mm_free(curr_thread->code_addr);
	}
	
	/* Delete the thread control block */
	mm_free((void *)curr_thread);

	/* This tells the context switcher to skip stack saving */
	curr_thread = NULL;
}

/*
 * This functions goes over all the threads and saves the new 
 * runtime window into current runtime. 
 */
static void calculate_runtime(void) {
	struct dlist_node* iter = cpu_rq.threads.first;

	while (iter != NULL) {
		struct thread* t = (struct thread *)iter->obj;
		t->runtime_curr = t->runtime_new;
		t->runtime_new = 0;
		iter = iter->next;
	}
}

/*
 * Returns the current runtime of the current thread
 */
static u64 get_curr_thread_runtime(void) {
	/* Compute the runtime of the current running thread */
	u64 curr_runtime;
	if (reschedule_pending) {
		reschedule_pending = 0;

		/* Calculate the runtime */
		u32 cvr = systick_get_cvr();
		curr_runtime = (u64)(SYSTICK_RVR - cvr);
	} else {
		/* No reschedule is pending so the runtime is SYSTICK_RVR */
		curr_runtime = SYSTICK_RVR;
	}

	return curr_runtime;
}

/*
 * This calls the scheduler. It is called every millisecond or
 * after a reschedule.
 */
void systick_exception(void) {
	if (scheduler_status) {
		cpsid_f();

		u64 curr_runtime = get_curr_thread_runtime();

		tick += curr_runtime;
		stats_tick += curr_runtime;
		curr_thread->runtime_new += curr_runtime;

		/* Every second the scheduler will calulate the thread new runtime */
		if (stats_tick >= SYSTICK_RVR * 1000) {
			stats_tick = 0;
			calculate_runtime();
		}
		
		process_expired_delays();

		/* Enqueue the thread */
		if (curr_thread->tick_to_wake == 0) {
			/* The current thread has to be enqueued again */
			curr_thread->class->enqueue((struct thread *)curr_thread, &cpu_rq);
		}

		/*
		 * Check if the thread should be removed. No reference to
		 * curr_thread should be performed after this point. 
		 */
		if (curr_thread->exit_pending) {
			scheduler_remove_thread(curr_thread);
		}

		/* Call the core scheduler */
		next_thread = core_scheduler();
		systick_set_cvr(SYSTICK_RVR);
		cpsie_f();

		/* Pend the context switch */
		pendsv_set_pending();
	}
}

/*
 * The reschedule will pend the SysTick interrupt. This will compute
 * the actual runtime and run the scheduler
 */
void reschedule(void) {
	reschedule_pending = 1;
	systick_set_pending();
}

/*
 * This will enqueue the thread into the sorted `sleep_q` list. The
 * first node will have to lowest tick to wake
 */
void scheduler_enqueue_delay(struct thread* thread) {
	struct dlist_node* iter = cpu_rq.sleep_q.first;

	if (iter == NULL) {
		dlist_insert_first(&thread->rq_node, &cpu_rq.sleep_q);

	} else {
		while (iter != NULL) {
			/*
			 * Check if the `iter` node has a higher `tick_to_wake`
			 * value than the thread to insert. If that is the case,
			 * insert the thread before `iter`
			 */
			struct thread* t = (struct thread *)iter->obj;
			if (t->tick_to_wake > thread->tick_to_wake) {
				dlist_insert_before(&thread->rq_node, iter, &cpu_rq.sleep_q);
				break;
			}
			iter = iter->next;
		}

		if (iter == NULL) {
			dlist_insert_last(&thread->rq_node, &cpu_rq.sleep_q);
		}
	}

	/* Update the thread list */
	thread->rq_list = &cpu_rq.sleep_q;

	/*
	 * If the new thread is placed first in the sleep queue, the first
	 * tick to wake might have changed
	 */
	tick_to_wake_update();
}

void suspend_scheduler(void) {
	scheduler_status = 0;
}

void resume_scheduler(void) {
	scheduler_status = 1;
}

u64 get_kernel_tick(void) {
	return tick;
}

u64 get_idle_runtime(void) {
	return cpu_rq.idle->runtime_curr;
}
