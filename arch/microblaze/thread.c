/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <debug.h>
#include <trace.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <kernel/thread.h>
#include <arch/microblaze.h>

#define LOCAL_TRACE 0

struct thread *_current_thread;

static void initial_thread_func(void) __NO_RETURN;
static void initial_thread_func(void)
{
    thread_t *ct = get_current_thread();

#if LOCAL_TRACE
    LTRACEF("thread %p calling %p with arg %p\n", ct, ct->entry, ct->arg);
    dump_thread(ct);
#endif

    /* exit the implicit critical section we're within */
    exit_critical_section();

    int ret = ct->entry(ct->arg);

    LTRACEF("thread %p exiting with %d\n", ct, ret);

    thread_exit(ret);
}

void arch_thread_initialize(thread_t *t)
{
    LTRACEF("t %p (%s)\n", t, t->name);

    /* some registers we want to clone for the new thread */
    register uint32_t r2 asm("r2");
    register uint32_t r13 asm("r13");
    register uint32_t r14 asm("r14");
    register uint32_t r16 asm("r16");
    register uint32_t r17 asm("r17");

    /* zero out the thread context */
    memset(&t->arch.cs_frame, 0, sizeof(t->arch.cs_frame));

    t->arch.cs_frame.r1 = (vaddr_t)t->stack + t->stack_size;
    t->arch.cs_frame.r2 = r2;
    t->arch.cs_frame.r13 = r13;
    t->arch.cs_frame.r14 = r14;
    t->arch.cs_frame.r15 = (vaddr_t)initial_thread_func - 8; // rtsd in context switch expects this
    t->arch.cs_frame.r16 = r16;
    t->arch.cs_frame.r17 = r17;
}

void arch_context_switch(thread_t *oldthread, thread_t *newthread)
{
    LTRACEF("old %p (%s), new %p (%s)\n", oldthread, oldthread->name, newthread, newthread->name);

    microblaze_context_switch(&oldthread->arch.cs_frame, &newthread->arch.cs_frame);
}

