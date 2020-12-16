# cascoda-bm-thread

This folder contains the platform code needed to port OpenThread to the baremetal cascoda-sdk. The CMake project contained within this folder defines the targets `ca821x-openthread-bm-ftd` and `ca821x-openthread-bm-mtd` - the platform layers for Full Thread Devices and Minimal Thread Devices, respectively.

One of these libraries must be linked into any application that uses OpenThread, based on the type of Thread device required. The [OpenThread API](https://openthread.io/reference) should be used for controlling the OpenThread stack.
