///////////////////////////////////////////////////////////////////////////////////////////////
//
// The module provides logic to enable/disable collection of pmc from userspace 
//
// The use of RDPMC instruction in user space is controlled by PCE flag in x86 CR4 register
//
// This module provides methods to enable/disable CR4.pce flag from kernel space
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#include <linux/sched.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/cpumask.h>
#include <xpedite/pmu/PCECtl.h>

/*************************************************************************
* CR4.pce management 
**************************************************************************/

uint64_t loadCR4(void) {
  volatile uint64_t value;
  __asm__ __volatile__ ("\t mov %%cr4,%0" : "=r"(value));
  return value;
}

void enablePCE(void) {
  volatile uint64_t vPrev;
  volatile uint64_t v;
  // Set CR4.pce to enable reading pmc from userspace
  vPrev = loadCR4();
  __asm__ __volatile__ (
    "push %rax              \n"
    "mov  %cr4, %rax;       \n"
    "or   $(1 << 8), %rax;  \n"
    "mov  %rax, %cr4;       \n"
    "pop  %rax"
  );
  v = loadCR4();
  printk(KERN_INFO "Xpedtie - enabled CR4.PCE in core %d - [0x%08llx] -> [0x%08llx]", smp_processor_id(), vPrev, v);
}

void clearPCE(void) {
  volatile uint64_t vPrev;
  volatile uint64_t v;
  vPrev = loadCR4();
  __asm__ __volatile__ (
    "push %rax              \n"
    "push %rbx              \n"
    "mov  %cr4, %rax;       \n"
    "mov  $(1 << 8), %rbx   \n"
    "not  %rbx              \n"
    "and  %rbx, %rax;       \n"
    "mov  %rax, %cr4;       \n"
    "pop  %rbx              \n"
    "pop  %rax              \n"
  );
  v = loadCR4();
  printk(KERN_INFO "Xpedite - cleared CR4.PCE in core %d - [0x%08llx] -> [0x%08llx]", smp_processor_id(), vPrev, v);
}
