///////////////////////////////////////////////////////////////////////////////////////////////
//
// Provides methods to safely program and reset pmu registers
// 
// The set of changes is treated as a single atomic operation.
// If the logic fails to set any one of the registers, it should
// abort and rollback or reset all the registers
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#include "PMUArch.h"
#include <linux/sched.h>
#include <asm/msr.h>
#include <linux/smp.h>        // Contains types, macros, functions for the kernel
#include <linux/kernel.h>     // Contains types, macros, functions for the kernel
#include <linux/fs.h>         // Header for the Linux file system support
#include <asm/uaccess.h>      // Required for the copy to user function

static ssize_t resetGlobalCtrl(void) {
  ssize_t err = wrmsr_safe(PMU_MSR_IA32_PERF_GLOBAL_CTRL, 0, 0);
  if(err) {
    printk(KERN_ALERT "PMUCtrl: Failed to reset IA32_PERF_GLOBAL_CTRL on core %d\n", smp_processor_id());
  }
  return err;
}

static ssize_t resetPEBSEnable(void) {
  ssize_t err = wrmsr_safe(PMU_MSR_IA32_PEBS_ENABLE, 0, 0);
  if(err) {
    printk(KERN_ALERT "PMUCtrl: Failed to reset IA32_PEBS_ENABLE on core %d\n", smp_processor_id());
  }
  return err;
}

static ssize_t resetFixedCounters(void) {
  ssize_t err = wrmsr_safe(PMU_MSR_IA32_FIXED_CTR_CTRL, 0, 0);
  if(!err) err = wrmsr_safe(PMU_MSR_IA32_FIXED_CTR0, 0, 0);
  if(!err) err = wrmsr_safe(PMU_MSR_IA32_FIXED_CTR1, 0, 0);
  if(!err) err = wrmsr_safe(PMU_MSR_IA32_FIXED_CTR2, 0, 0);

  if(err) {
    printk(KERN_ALERT "PMUCtrl: Failed to reset IA32_FIXED_CTR_CTRL/IA32_FIXED_CTRx msr on core %d\n", smp_processor_id());
  }
  return err;
}

static ssize_t resetGpCounters(void) {
  ssize_t err = 0;
  unsigned i;
  for(i=0; i< XPEDITE_PMC_CTRL_GP_EVENT_MAX && !err; ++i) {
    err = wrmsr_safe(PMU_MSR_PerfEvtSel0 + i, 0, 0);
    if(!err) err = wrmsr_safe(PMU_MSR_IA32_PMC0 + i, 0, 0);
  }
  if(err) {
    printk(KERN_ALERT "PMUCtrl: Failed to reset PerfEvtSelx/IA32_PMCx on core %d\n", smp_processor_id());
  }
  return err;
}

static ssize_t resetOffcoreCounters(void) {
  ssize_t err = 0;
  unsigned i;
  for(i=0; i< XPEDITE_PMC_CTRL_OFFCORE_EVENT_MAX && !err; ++i) {
    err = wrmsr_safe(PMU_MSR_OFFCORE_RSP_0 + i, 0, 0);
  }
  if(err) {
    printk(KERN_ALERT "PMUCtrl: Failed to reset MSR_OFFCORE_RSP_N on core %d\n", smp_processor_id());
  }
  return err;
}

static ssize_t enableFixedCtrCtrl(PMUEventSet *eventSet_) {
  ssize_t err = wrmsr_safe(PMU_MSR_IA32_FIXED_CTR_CTRL, eventSet_->_fixedEvtSel, 0 /*high*/);
  if(err) {
    printk(KERN_ALERT "PMUCtrl: Failed to reset IA32_FIXED_CTR_CTRL on core %d\n", smp_processor_id());
  }
  return err;
}

static ssize_t enableGpCounters(PMUEventSet *eventSet_) {
  ssize_t err = 0;
  unsigned i;
  for(i=0; i < eventSet_->_gpEvtCount; ++i) {
    err = wrmsr_safe(PMU_MSR_PerfEvtSel0 + i, eventSet_->_gpEvtSel[i], 0);
    if(err) {
      printk(KERN_ALERT "PMUCtrl: Failed to reset PerfEvtSel(%u) on core %d\n", i, smp_processor_id());
      break;
    }
  }
  return err;
}

static ssize_t enableOffcoreCounters(PMUEventSet *eventSet_) {
  ssize_t err = 0;
  unsigned i;
  u32 low, high;
  for(i=0; i < eventSet_->_offcoreEvtCount; ++i) {
    low = eventSet_->_offcoreEvtSel[i];
    high = eventSet_->_offcoreEvtSel[i] >> 32;
    printk(KERN_INFO "PMUCtrl: setting MSR_OFFCORE_RSP_%u -> %x | %x\n", i, high, low);
    err = wrmsr_safe(PMU_MSR_OFFCORE_RSP_0 + i, low, high);
    if(err) {
      printk(KERN_ALERT "PMUCtrl: Failed to reset MSR_OFFCORE_RSP_%u on core %d\n", i, smp_processor_id());
      break;
    }
  }
  return err;
}

static ssize_t enableGlobalCtrl(u32 low_, u32 high_) {
  ssize_t err = wrmsr_safe(PMU_MSR_IA32_PERF_GLOBAL_CTRL, low_, high_);
  if(err) {
    printk(KERN_ALERT "PMUCtrl: Failed to reset IA32_PERF_GLOBAL_CTRL on core %d\n", smp_processor_id());
  }
  return err;
}

ssize_t pmuClearEventSet(void) {
  ssize_t err = resetGlobalCtrl();
  if(!err) err = resetPEBSEnable();
  if(!err) err = resetFixedCounters();
  if(!err) err = resetGpCounters();
  if(!err) err = resetOffcoreCounters();
  if(!err) {
    printk(KERN_INFO "PMUCtrl: cleared all pmu counters on core %d", smp_processor_id());
  }
  return err;
}

ssize_t pmuEnableEventSet(PMUEventSet *eventSet_) {
  u32 low, high;

  if(eventSet_->_gpEvtCount > XPEDITE_PMC_CTRL_GP_EVENT_MAX) {
    eventSet_->_err = -EFAULT;
    return eventSet_->_err;
  }

  eventSet_->_err = pmuClearEventSet();
  if(eventSet_->_err) {
    return eventSet_->_err;
  }

  eventSet_->_err = enableGpCounters(eventSet_);

  if(!eventSet_->_err) {
    eventSet_->_err = enableOffcoreCounters(eventSet_);
  }

  if(!eventSet_->_err) {
    eventSet_->_err = enableFixedCtrCtrl(eventSet_);
  }

  low = (1 << eventSet_->_gpEvtCount) -1;
  high = eventSet_->_fixedEvtGlobalCtrl;
  if(!eventSet_->_err) {
    eventSet_->_err = enableGlobalCtrl(low, high);
  }

  if(eventSet_->_err) {
    pmuClearEventSet();
  }
  else {
    uint64_t globalCtrCtrl = ((uint64_t)high << 32) | low;
    printk(KERN_INFO "PMUCtrl: enabled pmu counters | IA32_FIXED_CTR_CTRL [0x%08llX] | IA32_PERF_GLOBAL_CTRL [0x%016llX] | on core %d", 
      (uint64_t)eventSet_->_fixedEvtSel, globalCtrCtrl, smp_processor_id());
  }
  return eventSet_->_err;
}
