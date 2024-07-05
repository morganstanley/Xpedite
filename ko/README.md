# Xpedite kernel module is deprecated

Xpedite no longer requires kernel module to use hardware performance counters.
Xpedite now uses perf events api to program and collect PMU data from user space.

For more details checkout the section on "Hardware performance counters" in Xpedite readme docs.



# Xpedite kernel module

Xpedite provides a minimalistic kernel module to support programming and collection of performance counters from user space.


To build the module, you will need a version 2.5 or later Linux kernel.

running ```make ```, will build it in place in the same directory.

To load the kernel module run the below command, as privileged user

```
make install
```

## Certified micro architectures

The kernel module is supported in Sandy Bridge and all subsequently released micro architectures.

## Implementation

The kernel module implements a device driver that can be used to program up to 8 general purpose and 3 Fixed performance counters.

A conscious effort is made to keep the kernel module trivial and move as much functionality to python as feasible.

PMUArch - Provides facilities the safely program General purpose and fixed performance counters

PMUCtrl - Handles validation and processing of request from user space to program counters.
        - Enables flag to permit invocation of rdpmc from user space.

