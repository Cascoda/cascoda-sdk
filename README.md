<p align="center"><img src="etc/img/cascoda.png" width="75%"></p>

# Cascoda SDK

The Cascoda SDK includes a comprehensive set of tools for developing systems integrating Cascoda hardware. The SDK is designed to be cross platform and flexible, enabling designing on one system and porting to another with ease. Example applications are included in order to demonstrate use of the systems.

The SDK contains a general API which abstracts the functionality of the CA-8210 or CA-8211, and can be run on baremetal or Linux systems.

<p align="center"><img src="etc/img/sdk_table.jpg" width="75%" align="center"></p>

The Bare-Metal BSPs provide a set of common functionality for baremetal platforms, with a simple abstraction layer that can be easily ported to new systems. The Linux platform takes advantage of the extra functionality to enable control of multiple devices at a time, and dynamic selection of SPI/USB Cascoda devices.

## Building

### CMake
The Cascoda SDK makes full use of CMake as a build system, to enable advanced configuration and cross-platform development. In order to build the Cascoda SDK, you will require CMake version 3.12 or newer. This can be downloaded from the [CMake Website.](https://cmake.org/download/)

### Compilers
The Cascoda SDK can be built natively for Linux using any preferred native compiler.

For Cross-Compiling to baremetal, we have fully implemented the ARM GCC compilers for use. They can be downloaded [here.](https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads) The code can be built in other embedded compilers, and full integration in the CMake suite is coming soon. Currently the OpenThread library can only be built from a Linux system (Windows Subsystem for Linux also works).

### Instructions
To build the Cascoda SDK, follow the following instructions (written for Linux):
```Bash
# Make a working directory
mkdir cascoda
cd cascoda
# Clone the Cascoda SDK
git clone https://github.com/Cascoda/cascoda-sdk.git

# Create a build directory for native build, and build the SDK
mkdir sdk-posix
cd sdk-posix
cmake ../cascoda-sdk
make -j12
# Built for current system! To change configuration, the 'ccmake .' command can be used

#Go back to working directory
cd ..

#Now cross compile for the Chili2
mkdir sdk-chili2
cd sdk-chili2
cmake ../cascoda-sdk -DCMAKE_TOOLCHAIN_FILE="../cascoda-sdk/arm_none_eabi_m2351.cmake"
make -j12
# Built for Chili 2! To change configuration, the 'ccmake .' command can be used
```

## Directory layout

### ca821x-api
ca821x-api contains the cross-platform api which abstracts all of the functionality of the CA-8210 and CA-8211. It is required for every project.

### baremetal
baremetal contains the cross-platform baremetal drivers, some example applications, and a set of platform abstractions.

### toolchain
toolchain contains the platform configuration files to enable cross compilation for different systems and compilers.

### posix
posix contains the Posix-specific drivers and tools, as well as some example applications that can be run from a Linux system.

### openthread
openthread contains the glue configuration to download the openthread repository from https://github.com/Cascoda/openthread, and configure it to be built with the SDK.

### etc
etc contains miscellaneous resources.
