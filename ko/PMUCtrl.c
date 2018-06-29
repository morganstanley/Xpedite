///////////////////////////////////////////////////////////////////////////////////////////////
//
// Device driver to support programming hardware perfromance counters
// 
// The module creates a device, for recieving request from user space applications. 
// Core and offcore PMU Events (3 Fixed, 8 General Purpose and 2 Offcore) are supported.
// 
// The module also set flag in CR4 register to permit rdpmc calls from userspace
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/mutex.h>
#include <linux/cpumask.h>
#include "PMUCtrl.h"
#include "PMUArch.h"
#define  DEVICE_NAME "xpedite"
#define  CLASS_NAME  "xpedite"

MODULE_AUTHOR("Manikandan Dhamodharan");
MODULE_DESCRIPTION("Xpedite module to program PMU unit in Intel processors");
MODULE_VERSION("2.0");

static int    majorNumber;
static struct class*  pmuClass  = NULL;
static struct device* pmuDevice = NULL;
static int    numberOpens = 0;

static DEFINE_MUTEX(pmu_mutex);

cpumask_t activeCpuSet;

static int     pmu_open(struct inode *, struct file *);
static int     pmu_release(struct inode *, struct file *);
static ssize_t pmu_read(struct file *, char *, size_t, loff_t *);
static ssize_t pmu_write(struct file *, const char *, size_t, loff_t *);


/*************************************************************************
* CR4.pce management 
**************************************************************************/

static u64 loadCR4(void) {
  volatile u64 value;
  __asm__ __volatile__ ("\t mov %%cr4,%0" : "=r"(value));
  return value;
}

static void enablePCE(void) {
  volatile u64 vPrev;
  volatile u64 v;
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
  printk(KERN_INFO "PMUCtrl - enabled CR4.PCE in core %d - [0x%08llx] -> [0x%08llx]", smp_processor_id(), vPrev, v);
}

static void clearPCE(void) {
  volatile u64 vPrev;
  volatile u64 v;
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
  printk(KERN_INFO "PMUCtrl - cleared CR4.PCE in core %d - [0x%08llx] -> [0x%08llx]", smp_processor_id(), vPrev, v);
}

/*************************************************************************
* Device management
**************************************************************************/

static struct file_operations fops = {
  .open = pmu_open,
  .read = pmu_read,
  .write = pmu_write,
  .release = pmu_release,
};

static int __init pmu_init(void) {
  // Try to dynamically allocate a major number for the device
  majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
  if (majorNumber<0) {
    printk(KERN_ALERT "PMUCtrl: Failed to register a major number\n");
    return majorNumber;
  }

  // Register the device class
  pmuClass = class_create(THIS_MODULE, CLASS_NAME);
  if (IS_ERR(pmuClass)) {           // Check for error and clean up if there is
    unregister_chrdev(majorNumber, DEVICE_NAME);
    printk(KERN_ALERT "Failed to register device class\n");
    return PTR_ERR(pmuClass);
  }

  // Register the device driver
  pmuDevice = device_create(pmuClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
  if (IS_ERR(pmuDevice)) {          // Clean up if there is an error
    class_destroy(pmuClass);
    unregister_chrdev(majorNumber, DEVICE_NAME);
    printk(KERN_ALERT "PMUCtrl: Failed to create the device\n");
    return PTR_ERR(pmuDevice);
  }

  printk(KERN_INFO "PMUCtrl: char device driver loaded with major number %d\n", majorNumber);
  mutex_init(&pmu_mutex);
  cpumask_clear(&activeCpuSet);
  return 0;
}

static void __exit pmu_exit(void) {
  mutex_destroy(&pmu_mutex);
  device_destroy(pmuClass, MKDEV(majorNumber, 0));
  class_unregister(pmuClass);
  class_destroy(pmuClass);
  unregister_chrdev(majorNumber, DEVICE_NAME);
  printk(KERN_INFO "PMUCtrl: exiting!\n");
}

static int pmu_open(struct inode *inodep, struct file *filep) {
  if(!mutex_trylock(&pmu_mutex)) {                  // Try to acquire the mutex (returns 0 on fail)
    printk(KERN_ALERT "PMUCtrl: device in use by another process");
    return -EBUSY;
  }
  ++numberOpens;
  printk(KERN_INFO "PMUCtrl: device has been opened %d time(s)\n", numberOpens);
  cpumask_clear(&activeCpuSet);
  return 0;
}

static ssize_t pmu_read(struct file *filep, char *buffer, size_t len, loff_t *offset) {
  printk(KERN_INFO "PMUCtrl: read %zd bytes from userspace\n", len);
  return -EFAULT;
}

static void print_pmuctrl_request(unsigned ctrIndex_, PMUGpEvent* e_, uint32_t b_) {
  printk(KERN_INFO 
    "PMUCtrl: eventSelect = 0x%02X | unitMask = 0x%02X | user = 0x%02X | kernel = 0x%02X |"
    " invertCMask = 0x%02X | counterMask = 0x%02X | -> PerfEvtSel%u [0x%08llX]", 
    0xff & e_->_eventSelect, 
    0xff & e_->_unitMask, 
    0xff & e_->_user, 
    0xff & e_->_kernel, 
    0xff & e_->_invertCMask, 
    0xff & e_->_counterMask,
    ctrIndex_,
    (uint64_t)b_
  );
}


static void print_offcore_request(unsigned ctrIndex_, uint64_t e_) {
  printk(KERN_INFO "PMUCtrl: setting MSR_OFFCORE_RSP_%u -> %llx\n", ctrIndex_, e_);
}

/*************************************************************************
* Logic to build bitmask for programming pmu events
**************************************************************************/

uint32_t buildPerfEvtSelBitmask(PMUGpEvent* e_) {
  typedef struct
  {
    union {
      uint32_t    _value;
      struct {
        unsigned char _eventSelect  : 8;
        unsigned char _unitMask     : 8;
        unsigned char _user         : 1;
        unsigned char _kernel       : 1;
        unsigned char _edgeDetect   : 1;
        unsigned char _pinControl   : 1;
        unsigned char _interruptEn  : 1;
        unsigned char _anyThread    : 1;
        unsigned char _enable       : 1;
        unsigned char _invertCMask  : 1;
        unsigned char _counterMask  : 8;
      } _f;
    };
  } PerfEvtSelReg;
 
  PerfEvtSelReg r;
  r._f._eventSelect  = e_->_eventSelect;
  r._f._unitMask     = e_->_unitMask;
  r._f._user         = e_->_user != 0;
  r._f._kernel       = e_->_kernel != 0;
  r._f._edgeDetect   = e_->_edgeDetect != 0;
  r._f._pinControl   = 0;
  r._f._interruptEn  = 0;
  r._f._anyThread    = e_->_anyThread != 0;
  r._f._enable       = 1;
  r._f._invertCMask  = e_->_invertCMask != 0;
  r._f._counterMask  = e_->_counterMask;
  return r._value;
}

PMUFixedEvent* findFixedEvtForCtr(unsigned char ctrIndex_, PMUFixedEvent* fixedEvents_, unsigned char fixedEvtCount_) {
  unsigned i;
  for(i=0; i < fixedEvtCount_; ++i) {
    if(fixedEvents_[i]._ctrIndex == ctrIndex_) {
      return &fixedEvents_[i];
    }
  }
  return NULL;
}

unsigned char feEnablemask(PMUFixedEvent* fixedEvents_) {
  return (
    fixedEvents_->_user && fixedEvents_->_kernel ? 3 : (
      fixedEvents_->_user ? 2 : 1
    )
  );
}

uint32_t buildFixedEvtSelBitmask(PMUFixedEvent* fixedEvents_, unsigned char fixedEvtCount_) {
  typedef struct
  {
    union {
      uint32_t    _value;
      struct {
        unsigned char _enable0       : 2;
        unsigned char _anyThread0    : 1;
        unsigned char _interruptEn0  : 1;
        unsigned char _enable1       : 2;
        unsigned char _anyThread1    : 1;
        unsigned char _interruptEn1  : 1;
        unsigned char _enable2       : 2;
        unsigned char _anyThread2    : 1;
        unsigned char _interruptEn2  : 1;
        uint32_t      _reservedBits  : 20;
      } _f;
    };
  } FixedEvtSelReg;

  PMUFixedEvent* evt0 = findFixedEvtForCtr(0, fixedEvents_, fixedEvtCount_);
  PMUFixedEvent* evt1 = findFixedEvtForCtr(1, fixedEvents_, fixedEvtCount_);
  PMUFixedEvent* evt2 = findFixedEvtForCtr(2, fixedEvents_, fixedEvtCount_);

  FixedEvtSelReg r;
  r._f._enable0      = evt0 ? feEnablemask(evt0) : 0;
  r._f._anyThread0   = 0;
  r._f._interruptEn0 = 0;
  r._f._enable1      = evt1 ? feEnablemask(evt1) : 0;
  r._f._anyThread1   = 0;
  r._f._interruptEn1 = 0;
  r._f._enable2      = evt2 ? feEnablemask(evt2) : 0;
  r._f._anyThread2   = 0;
  r._f._interruptEn2 = 0;
  r._f._reservedBits = 0;
  return r._value;
}

uint32_t buildFixedEvtGlobalCtrlBitmask(PMUFixedEvent* fixedEvents_, unsigned char fixedEvtCount_) {
  int i;
  uint32_t value = 0;
  for(i=0; i < fixedEvtCount_; ++i) {
    if(fixedEvents_[i]._ctrIndex < XPEDITE_PMC_CTRL_FIXED_EVENT_MAX) {
      value |= 0x1 << fixedEvents_[i]._ctrIndex;
    }
    else {
      printk(KERN_INFO "PMUCtrl: invalid request - Fixed event counter index(%u) excceds %d\n", 
        (unsigned)fixedEvents_[i]._ctrIndex, XPEDITE_PMC_CTRL_FIXED_EVENT_MAX);
      return 0;
    }
  }
  return value;
}

static void __pmuEnableEventSet(void *info) {
  ssize_t err = 0;
  PMUEventSet *eventSet = (PMUEventSet*) info;
  enablePCE();
  err = pmuEnableEventSet(eventSet);
  if(err) {
    printk(KERN_ALERT "PMUCtrl: Failed to enable PMU counters on core %d", smp_processor_id());
  }
}

static void __pmuClearEventSet(void *info) {
  clearPCE();
  pmuClearEventSet();
}

/*************************************************************************
* Logic to process pmu requests form userspace
**************************************************************************/

static ssize_t processRequest(PMUCtrlRequest* r_) {
  unsigned i;
  PMUEventSet eventSet;

  memset(&eventSet, 0, sizeof(eventSet));

  if (r_->_cpu >= nr_cpu_ids || !cpu_online(r_->_cpu)) {
    printk(KERN_INFO "PMUCtrl: invalid request - cpu %d not active\n", (unsigned)r_->_cpu);
    return -ENXIO;  /* No such CPU */
  }

  if(r_->_fixedEvtCount > XPEDITE_PMC_CTRL_FIXED_EVENT_MAX) {
    printk(KERN_INFO "PMUCtrl: invalid request - max available fixed event counters %d, recieved (%u)\n",
      XPEDITE_PMC_CTRL_FIXED_EVENT_MAX, (unsigned)r_->_fixedEvtCount);
    return -EFAULT;
  }

  if(r_->_gpEvtCount > XPEDITE_PMC_CTRL_GP_EVENT_MAX) {
    printk(KERN_INFO "PMUCtrl: invalid request - general purpose event cannot exeed %d, recieved (%u)\n", 
      XPEDITE_PMC_CTRL_GP_EVENT_MAX, (unsigned)r_->_gpEvtCount);
    return -EFAULT;
  }

  if(r_->_offcoreEvtCount > XPEDITE_PMC_CTRL_OFFCORE_EVENT_MAX) {
    printk(KERN_INFO "PMUCtrl: invalid request - offcore event cannot exeed %d, recieved (%u)\n", 
      XPEDITE_PMC_CTRL_OFFCORE_EVENT_MAX, (unsigned)r_->_offcoreEvtCount);
    return -EFAULT;
  }

  for(i=0; i< r_->_gpEvtCount; ++i) {
    eventSet._gpEvtSel[i] = buildPerfEvtSelBitmask(&r_->_gpEvents[i]);
    print_pmuctrl_request(i, &r_->_gpEvents[i], eventSet._gpEvtSel[i]);
  }
  eventSet._gpEvtCount = r_->_gpEvtCount;

  for(i=0; i< r_->_offcoreEvtCount; ++i) {
    eventSet._offcoreEvtSel[i] = r_->_offcoreEvents[i];
    print_offcore_request(i, r_->_offcoreEvents[i]);
  }
  eventSet._offcoreEvtCount = r_->_offcoreEvtCount;

  if(r_->_fixedEvtCount) {
    eventSet._fixedEvtGlobalCtrl = buildFixedEvtGlobalCtrlBitmask(r_->_fixedEvents, r_->_fixedEvtCount);
    if(!eventSet._fixedEvtGlobalCtrl) {
      return -EFAULT;
    }
    eventSet._fixedEvtSel = buildFixedEvtSelBitmask(r_->_fixedEvents, r_->_fixedEvtCount);
  }

  if(smp_call_function_single(r_->_cpu, __pmuEnableEventSet, &eventSet, 1)) {
    printk(KERN_INFO "PMUCtrl: failed to enable event counter in core %d", (unsigned)r_->_cpu);
    return -EFAULT;
  }

  if(eventSet._err) {
    return -EFAULT;
  }

  cpumask_set_cpu(r_->_cpu, &activeCpuSet);
  return sizeof(PMUCtrlRequest);
}

static ssize_t pmu_write(struct file *filep, const char *buffer, size_t len, loff_t *offset) {

  PMUCtrlRequest request;
  if(len != sizeof(PMUCtrlRequest)) {
    printk(KERN_INFO "PMUCtrl: invalid request (expected %zd bytes) | recieved %zd bytes\n", sizeof(PMUCtrlRequest), len);
    return -EFAULT;
  }

  printk(KERN_INFO "PMUCtrl: processing PMU Ctrl request (%zd bytes)\n", len);
  if (copy_from_user(&request, buffer, sizeof(request))) {
    printk(KERN_INFO "PMUCtrl: failed to copy request from user space");
    return -EFAULT;
  }
  return processRequest(&request);
}

static int pmu_release(struct inode *inodep, struct file *filep) {
  int cpuId;
  for_each_cpu(cpuId, &activeCpuSet) {
    if(smp_call_function_single(cpuId, __pmuClearEventSet, NULL, 1)) {
      printk(KERN_INFO "PMUCtrl: failed to clear event counter in core %d", (unsigned)cpuId);
    }
  }
  cpumask_clear(&activeCpuSet);
  mutex_unlock(&pmu_mutex);
  printk(KERN_INFO "PMUCtrl: device successfully closed\n");
  return 0;
}

module_init(pmu_init);
module_exit(pmu_exit);
