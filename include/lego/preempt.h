/*
 * Copyright (c) 2016-2017 Wuklab, Purdue University. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/*
 * Macros for accessing and manipulating preempt_count
 * Used for kernel preemption
 */

#ifndef _LEGO_PREEMPT_H_
#define _LEGO_PREEMPT_H_

#include <lego/irq.h>
#include <lego/percpu.h>

/*
 * Disable preemption until the scheduler is running
 * Reset by sched_init()->init_idle()->init_idle_preempt_count()
 */
#define INIT_PREEMPT_COUNT	1
#define PREEMPT_ENABLED		0

#define init_idle_preempt_count(p, cpu) do { \
	per_cpu(__preempt_count, (cpu)) = PREEMPT_ENABLED; \
} while (0)

DECLARE_PER_CPU(int, __preempt_count);

static __always_inline int preempt_count(void)
{
	return this_cpu_read(__preempt_count);
}

static __always_inline void preempt_count_add(int val)
{
	raw_cpu_add_4(__preempt_count, val);
}

static __always_inline void preempt_count_sub(int val)
{
	raw_cpu_add_4(__preempt_count, -val);
}

#define preempt_count_inc()	preempt_count_add(1)
#define preempt_count_dec()	preempt_count_sub(1)

#ifdef CONFIG_PREEMPT

#define preempt_disable()				\
do {							\
	preempt_count_inc();				\
	barrier();					\
} while (0)

#define preempt_enable()				\
do {							\
	barrier();					\
	if (unlikely(preempt_count_dec_and_test()))	\
		__preempt_schedule();			\
} while (0)

#define preempt_enable_no_resched()			\
do {							\
	barrier();					\
	preempt_count_dec();				\
} while (0)

#define preemptible()			(preempt_count() == 0 && !irqs_disabled())

#else /* !CONFIG_PREEMPT */
#define preempt_disable()		barrier()
#define preempt_enable()		barrier()
#define preempt_enable_no_resched()	barrier();
#define preemptible()			0
#endif /* CONFIG_PREEMPT */

#endif /* _LEGO_PREEMPT_H_ */