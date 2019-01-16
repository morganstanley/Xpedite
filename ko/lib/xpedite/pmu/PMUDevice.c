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

#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/mutex.h>
#include <linux/cpumask.h>
#include <xpedite/pmu/EventSet.h>
#include <xpedite/pmu/PMUArch.h>
#include <xpedite/pmu/PCECtl.h>
#include <xpedite/pmu/Formatter.h>

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
unsigned char gpEvtCount;

static int     pmu_open(struct inode *, struct file *);
static int     pmu_release(struct inode *, struct file *);
static ssize_t pmu_read(struct file *, char *, size_t, loff_t *);
static ssize_t pmu_write(struct file *, const char *, size_t, loff_t *);

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
    printk(KERN_ALERT "Xpedite: Failed to register a major number\n");
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
    printk(KERN_ALERT "Xpedite: Failed to create the device\n");
    return PTR_ERR(pmuDevice);
  }

  printk(KERN_INFO "Xpedite: char device driver loaded with major number %d\n", majorNumber);
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
  printk(KERN_INFO "Xpedite: exiting!\n");
}

static int pmu_open(struct inode *inodep, struct file *filep) {
  if(!mutex_trylock(&pmu_mutex)) {                  // Try to acquire the mutex (returns 0 on fail)
    printk(KERN_ALERT "Xpedite: device in use by another process");
    return -EBUSY;
  }
  ++numberOpens;
  printk(KERN_INFO "Xpedite: device has been opened %d time(s)\n", numberOpens);
  gpEvtCount = 0;
  cpumask_clear(&activeCpuSet);
  return 0;
}

static ssize_t pmu_read(struct file *filep, char *buffer, size_t len, loff_t *offset) {
  printk(KERN_INFO "Xpedite: read %zd bytes from userspace\n", len);
  return -EFAULT;
}

static void __pmuEnableEventSet(void *info) {
  ssize_t err = 0;
  EventSet *eventSet = (EventSet*) info;
  enablePCE();
  err = pmuEnableEventSet(eventSet);
  if(err) {
    printk(KERN_ALERT "Xpedite: Failed to enable PMU counters on core %d", smp_processor_id());
  }
}

static void __pmuClearEventSet(void *info) {
  clearPCE();
  pmuClearEventSet(gpEvtCount);
}

/*************************************************************************
* Logic to process pmu requests form userspace
**************************************************************************/

static ssize_t processRequest(PMUCtlRequest* request_) {
  EventSet eventSet;
  memset(&eventSet, 0, sizeof(eventSet));

  if (request_->_cpu >= nr_cpu_ids || !cpu_online(request_->_cpu)) {
    printk(KERN_INFO "Xpedite: invalid request - cpu %d not active\n", (unsigned)request_->_cpu);
    return -ENXIO;  /* No such CPU */
  }

  if(buildEventSet(request_, &eventSet)) {
    return -EFAULT;
  }

  logEventSet(request_, &eventSet);

  if(smp_call_function_single(request_->_cpu, __pmuEnableEventSet, &eventSet, 1)) {
    printk(KERN_INFO "Xpedite: failed to enable event counter in core %d", (unsigned)request_->_cpu);
    return -EFAULT;
  }

  if(eventSet._err) {
    return -EFAULT;
  }

  gpEvtCount = request_->_gpEvtCount > gpEvtCount ? request_->_gpEvtCount : gpEvtCount;
  cpumask_set_cpu(request_->_cpu, &activeCpuSet);
  return sizeof(PMUCtlRequest);
}

static ssize_t pmu_write(struct file *filep, const char *buffer, size_t len, loff_t *offset) {

  PMUCtlRequest request;
  if(len != sizeof(PMUCtlRequest)) {
    printk(KERN_INFO "Xpedite: invalid request (expected %zd bytes) | recieved %zd bytes\n", sizeof(PMUCtlRequest), len);
    return -EFAULT;
  }

  printk(KERN_INFO "Xpedite: processing PMU Ctl request (%zd bytes)\n", len);
  if (copy_from_user(&request, buffer, sizeof(request))) {
    printk(KERN_INFO "Xpedite: failed to copy request from user space");
    return -EFAULT;
  }
  return processRequest(&request);
}

static int pmu_release(struct inode *inodep, struct file *filep) {
  int cpuId;
  for_each_cpu(cpuId, &activeCpuSet) {
    if(smp_call_function_single(cpuId, __pmuClearEventSet, NULL, 1)) {
      printk(KERN_INFO "Xpedite: failed to clear event counter in core %d", (unsigned)cpuId);
    }
  }

  gpEvtCount = 0;
  cpumask_clear(&activeCpuSet);
  mutex_unlock(&pmu_mutex);
  printk(KERN_INFO "Xpedite: device successfully closed\n");
  return 0;
}

module_init(pmu_init);
module_exit(pmu_exit);
