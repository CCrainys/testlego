/*
 * Copyright (c) 2016-2017 Wuklab, Purdue University. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <lego/mm.h>
#include <lego/sched.h>
#include <lego/cpumask.h>
#include <asm/tlbflush.h>

void switch_mm_irqs_off(struct mm_struct *prev, struct mm_struct *next,
			struct task_struct *tsk)
{
	if (likely(prev != next))
		load_cr3(next->pgd);
}

/*
 * TODO:
 * This function should be optimized. Maybe we should follow the linux
 * mm and borrowed active_mm way. At first, I totally did not get why
 * using 2 mms. Now I understand, but need sometime to patch the mm
 * and active_mm code.
 *
 * Now just use this way.
 */
void switch_mm(struct mm_struct *prev, struct mm_struct *next,
	       struct task_struct *tsk)
{
	unsigned long flags;

	local_irq_save(flags);
	switch_mm_irqs_off(prev, next, tsk);
	local_irq_restore(flags);
}

/*
 * TLB flush funcation:
 * Flush the tlb entries if the cpu uses the mm that's being flushed.
 */
static void flush_tlb_func(void *info)
{
	struct flush_tlb_info *f = info;

	if (f->flush_end == TLB_FLUSH_ALL) {
		local_flush_tlb();
	} else {
		unsigned long addr;
		unsigned long nr_pages =
			(f->flush_end - f->flush_start) / PAGE_SIZE;
		addr = f->flush_start;
		while (addr < f->flush_end) {
			__flush_tlb_single(addr);
			addr += PAGE_SIZE;
		}
	}
}

void flush_tlb_others(const struct cpumask *cpumask, struct mm_struct *mm,
		      unsigned long start, unsigned long end)
{
	struct flush_tlb_info info;

	if (end == 0)
		end = start + PAGE_SIZE;
	info.flush_mm = mm;
	info.flush_start = start;
	info.flush_end = end;

	smp_call_function_many(cpumask, flush_tlb_func, &info, 1);
}

/* TODO */
static inline const cpumask_t *mm_cpumask(struct mm_struct *mm)
{
	return cpu_active_mask;
}

void flush_tlb_current_task(void)
{
	struct mm_struct *mm = current->mm;

	preempt_disable();

	/* This is an implicit full barrier that synchronizes with switch_mm. */
	local_flush_tlb();

	if (cpumask_any_but(mm_cpumask(mm), smp_processor_id()) < nr_cpu_ids)
		flush_tlb_others(mm_cpumask(mm), mm, 0UL, TLB_FLUSH_ALL);

	preempt_enable();
}

/*
 * See Documentation/x86/tlb.txt for details.  We choose 33
 * because it is large enough to cover the vast majority (at
 * least 95%) of allocations, and is small enough that we are
 * confident it will not cause too much overhead.  Each single
 * flush is about 100 ns, so this caps the maximum overhead at
 * _about_ 3,000 ns.
 *
 * This is in units of pages.
 */
static unsigned long tlb_single_page_flush_ceiling __read_mostly = 33;

void flush_tlb_mm_range(struct mm_struct *mm,
			unsigned long start, unsigned long end)
{
	unsigned long addr;
	/* do a global flush by default */
	unsigned long base_pages_to_flush = TLB_FLUSH_ALL;

	if (end != TLB_FLUSH_ALL)
		base_pages_to_flush = (end - start) >> PAGE_SHIFT;

	preempt_disable();

	/*
	 * Both branches below are implicit full barriers (MOV to CR or
	 * INVLPG) that synchronize with switch_mm.
	 */
	if (base_pages_to_flush > tlb_single_page_flush_ceiling) {
		base_pages_to_flush = TLB_FLUSH_ALL;
		local_flush_tlb();
	} else {
		/* flush range by one by one 'invlpg' */
		for (addr = start; addr < end;	addr += PAGE_SIZE)
			__flush_tlb_single(addr);
	}

	if (base_pages_to_flush == TLB_FLUSH_ALL) {
		start = 0UL;
		end = TLB_FLUSH_ALL;
	}

	if (cpumask_any_but(mm_cpumask(mm), smp_processor_id()) < nr_cpu_ids)
		flush_tlb_others(mm_cpumask(mm), mm, start, end);

	preempt_enable();
}
