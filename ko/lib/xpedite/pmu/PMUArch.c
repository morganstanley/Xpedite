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

#include <linux/kernel.h>
#include <linux/sched.h>
#include <asm/msr.h>
#include <linux/smp.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <xpedite/pmu/PMUArch.h>

static ssize_t resetGlobalCtl(void) {
  ssize_t err = wrmsr_safe(PMU_MSR_IA32_PERF_GLOBAL_CTRL, 0, 0);
  if(err) {
    printk(KERN_ALERT "Xpedite: Failed to reset IA32_PERF_GLOBAL_CTRL on core %d\n", smp_processor_id());
  }
  return err;
}

static ssize_t resetPEBSEnable(void) {
  ssize_t err = wrmsr_safe(PMU_MSR_IA32_PEBS_ENABLE, 0, 0);
  if(err) {
    printk(KERN_ALERT "Xpedite: Failed to reset IA32_PEBS_ENABLE on core %d\n", smp_processor_id());
  }
  return err;
}

static ssize_t resetFixedCounters(void) {
  ssize_t err = wrmsr_safe(PMU_MSR_IA32_FIXED_CTR_CTRL, 0, 0);
  if(!err) err = wrmsr_safe(PMU_MSR_IA32_FIXED_CTR0, 0, 0);
  if(!err) err = wrmsr_safe(PMU_MSR_IA32_FIXED_CTR1, 0, 0);
  if(!err) err = wrmsr_safe(PMU_MSR_IA32_FIXED_CTR2, 0, 0);

  if(err) {
    printk(KERN_ALERT "Xpedite: Failed to reset IA32_FIXED_CTR_CTRL/IA32_FIXED_CTRx msr on core %d\n", smp_processor_id());
  }
  return err;
}

static ssize_t resetGpCounters(unsigned char gpEvtCount_) {
  ssize_t err = 0;
  unsigned i;
  for(i=0; i< gpEvtCount_ && !err; ++i) {
    err = wrmsr_safe(PMU_MSR_PerfEvtSel0 + i, 0, 0);
    if(!err) err = wrmsr_safe(PMU_MSR_IA32_PMC0 + i, 0, 0);
  }
  if(err) {
    printk(KERN_ALERT "Xpedite: Failed to reset PerfEvtSelx/IA32_PMCx %d on core %d\n", i, smp_processor_id());
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
    printk(KERN_ALERT "Xpedite: Failed to reset MSR_OFFCORE_RSP_N on core %d\n", smp_processor_id());
  }
  return err;
}

static ssize_t enableFixedCtrCtl(EventSelect *eventSelect_) {
  ssize_t err = wrmsr_safe(PMU_MSR_IA32_FIXED_CTR_CTRL, eventSelect_->_fixedEvtSel, 0 /*high*/);
  if(err) {
    printk(KERN_ALERT "Xpedite: Failed to reset IA32_FIXED_CTR_CTRL on core %d\n", smp_processor_id());
  }
  return err;
}

static ssize_t enableGpCounters(EventSelect *eventSelect_) {
  ssize_t err = 0;
  unsigned i;
  for(i=0; i < eventSelect_->_gpEvtCount; ++i) {
    err = wrmsr_safe(PMU_MSR_PerfEvtSel0 + i, eventSelect_->_gpEvtSel[i], 0);
    if(err) {
      printk(KERN_ALERT "Xpedite: Failed to reset PerfEvtSel(%u) on core %d\n", i, smp_processor_id());
      break;
    }
  }
  return err;
}

static ssize_t enableOffcoreCounters(EventSelect *eventSelect_) {
  ssize_t err = 0;
  unsigned i;
  u32 low, high;
  for(i=0; i < eventSelect_->_offcoreEvtCount; ++i) {
    low = eventSelect_->_offcoreEvtSel[i];
    high = eventSelect_->_offcoreEvtSel[i] >> 32;
    printk(KERN_INFO "Xpedite: setting MSR_OFFCORE_RSP_%u -> %x | %x\n", i, high, low);
    err = wrmsr_safe(PMU_MSR_OFFCORE_RSP_0 + i, low, high);
    if(err) {
      printk(KERN_ALERT "Xpedite: Failed to reset MSR_OFFCORE_RSP_%u on core %d\n", i, smp_processor_id());
      break;
    }
  }
  return err;
}

static ssize_t enableGlobalCtl(u32 low_, u32 high_) {
  ssize_t err = wrmsr_safe(PMU_MSR_IA32_PERF_GLOBAL_CTRL, low_, high_);
  if(err) {
    printk(KERN_ALERT "Xpedite: Failed to reset IA32_PERF_GLOBAL_CTRL on core %d\n", smp_processor_id());
  }
  return err;
}

ssize_t pmuClearEventSet(unsigned char gpEvtCount_) {
  ssize_t err = resetGlobalCtl();
  if(!err) err = resetPEBSEnable();
  if(!err) err = resetFixedCounters();
  if(!err) err = resetGpCounters(gpEvtCount_);
  if(!err) err = resetOffcoreCounters();
  if(!err) {
    printk(KERN_INFO "Xpedite: cleared %d core and all fixed pmu counters on core %d", gpEvtCount_, smp_processor_id());
  }
  return err;
}

ssize_t pmuEnableEventSet(EventSelect *eventSelect_) {
  u32 low, high;

  if(eventSelect_->_gpEvtCount > XPEDITE_PMC_CTRL_GP_EVENT_MAX) {
    eventSelect_->_err = -EFAULT;
    return eventSelect_->_err;
  }

  eventSelect_->_err = pmuClearEventSet(eventSelect_->_gpEvtCount);
  if(eventSelect_->_err) {
    return eventSelect_->_err;
  }

  eventSelect_->_err = enableGpCounters(eventSelect_);

  if(!eventSelect_->_err) {
    eventSelect_->_err = enableOffcoreCounters(eventSelect_);
  }

  if(!eventSelect_->_err) {
    eventSelect_->_err = enableFixedCtrCtl(eventSelect_);
  }

  low = (1 << eventSelect_->_gpEvtCount) -1;
  high = eventSelect_->_fixedEvtGlobalCtl;
  if(!eventSelect_->_err) {
    eventSelect_->_err = enableGlobalCtl(low, high);
  }

  if(eventSelect_->_err) {
    pmuClearEventSet(eventSelect_->_gpEvtCount);
  }
  else {
    uint64_t globalCtrCtl = ((uint64_t)high << 32) | low;
    printk(KERN_INFO "Xpedite: enabled pmu counters | IA32_FIXED_CTR_CTRL [0x%08llX] | IA32_PERF_GLOBAL_CTRL [0x%016llX] | on core %d", 
      (uint64_t)eventSelect_->_fixedEvtSel, globalCtrCtl, smp_processor_id());
  }
  return eventSelect_->_err;
}
