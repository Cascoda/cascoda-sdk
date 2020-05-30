<p align="center"><img src="etc/img/cascoda.png" width="75%"></p>

# Cascoda SDK

The Cascoda SDK includes a comprehensive set of tools for developing systems integrating Cascoda hardware. The SDK is designed to be cross platform and flexible, enabling designing on one system and porting to another with ease. Example applications are included in order to demonstrate use of the systems.

The SDK contains a general API which abstracts the functionality of the CA-8210 or CA-8211, and can be run on baremetal or Linux systems.

<p align="center"><img src="etc/img/sdk_table.jpg" width="75%" align="center"></p>

The Bare-Metal BSPs provide a set of common functionality for baremetal platforms, with a simple abstraction layer that can be easily ported to new systems. The Linux platform takes advantage of the extra functionality to enable control of multiple devices at a time, and dynamic selection of SPI/USB Cascoda devices.

## Contents

1. [Building](#building)
2. [Example Applications](#example-applications)
3. [Debugging](#debugging)
4. [Directory Layout](#directory-layout)
5. [CMake Library Targets](#cmake-library-targets)
6. [Useful Links](#useful-links)

## Building

### CMake
The Cascoda SDK makes full use of CMake as a build system, to enable advanced configuration and cross-platform development. In order to build the Cascoda SDK, you will require CMake version 3.13 or newer. This can be downloaded from the [CMake Website.](https://cmake.org/download/)

### Compilers
The Cascoda SDK can be built natively for Linux using any preferred native compiler.

For Cross-Compiling to baremetal, we have fully implemented the ARM GCC compilers for use. They can be downloaded [here.](https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads) The code can be built in other embedded compilers, and full integration in the CMake suite is coming soon.

### Instructions
To build the Cascoda SDK, follow the instructions below:

<details open><summary>Linux/MacOS</summary>

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
cmake ../cascoda-sdk -DCMAKE_TOOLCHAIN_FILE="toolchain/arm_gcc_m2351.cmake"
make -j12
# Built for Chili 2! To change configuration, the 'ccmake .' command can be used
```

</details>

<details><summary>Windows</summary>

```Bash
# Make a working directory
mkdir cascoda
cd cascoda
# Clone the Cascoda SDK (This must be done in git bash)
git clone https://github.com/Cascoda/cascoda-sdk.git

# After cloning the repo as above, run the following commands in powershell/cmd:
# Create a build directory for native build, and build the SDK
mkdir sdk-win
cd sdk-win
cmake.exe ../cascoda-sdk -G "MinGW Makefiles"
mingw32-make.exe -j8
# Built for current system! To change configuration, the 'cmake-gui.exe .' command can be used

#Go back to working directory
cd ..

#Now cross compile for the Chili2
mkdir sdk-chili2
cd sdk-chili2
cmake.exe ../cascoda-sdk -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE="toolchain/arm_gcc_m2351.cmake"
mingw32-make.exe -j8
# Built for Chili 2! To change configuration, the 'cmake-gui.exe .' command can be used
```

</details>

Libraries will be built into the ``lib/`` directory, while application binaries will be built into the ``bin/`` directory. For the Chili platforms, both elf format and binary .bin format files will be created.

In order to compile for the Chili 1, or to use a different compiler, the CMAKE_TOOLCHAIN_FILE argument can be pointed to a different configuration file in the toolchain directory.

## Example Applications

If you are looking to test existing functionality of the Cascoda SDK, start here. This is a list of the demos within the Cascoda SDK, alongside a short description of what each of them do.

### Embedded Targets

| CMake Target Name | Description |
| :--- | :--- |
| mac-dongle | Use a Chili as a IEEE 802.15.4 MAC dongle, enabling hosted platforms to communicate using this protocol. Used by most POSIX targets.
| ot-cli-lwip | A demonstration of the Lightweight IP Stack running on top of OpenThread. [Click here for more documentation](baremetal/app/ot-cli-lwip/README.md)
| ot-cli-lwip-freertos | Same as above, but taking advantage of the POSIX socket API and FreeRTOS. [Click here for more documentation](baremetal/app/ot-cli-lwip-freertos/README.md)
| ot-cli | The OpenThread command line interface running on a Chili. Interfaces with the Cascoda sensordemo application layer. Works with the serial-adapter POSIX application.
| ot-cli-actuator | Similar to ot-cli, but also has actuator commands. [Click here for more documentation.](baremetal/app/ot-cli-actuator/README.md)
| ot-ncp |The OpenThread network co-processor interface running on a Chili. Works with the serial-adapter POSIX application. 
| ot-sed-thermometer | Sleepy end device that interfaces with the Cascoda sensordemo application layer. Only reports temperature. [Click here for more documentation.](baremetal/app/ot-sed-thermometer/README.md)
| ot-sed-thermometer-freertos | Same as above, but taking advantage of FreeRTOS concurrency features. [Click here for more documentation.](baremetal/app/ot-sed-thermometer-freertos/README.md)
| ot-sed-sensorif | Sleepy end device that interfaces with the Cascoda sensordemo application layer. Connects to various I2C and SPI sensors. [Click here for more documentation.](baremetal/app/ot-sed-sensorif/README.md)
| ot-sed-eink-freertos | Display images onto an e-ink screen. The images are transmitted over a Thread network. The system sleeps while idle. Used in conjunction with ot-eink-server. [Click here for more documentation.](baremetal/app/ot-sed-eink-freertos/README.md)
| eink-test | Tests the connection between a Chili and a E-Ink display by showing an image. Does not commmunicate over IEEE 802.15.4.
| sensorif-test | Tests the sensorif API by attempting communication with all connected sensors.
| mac-tempsense | Legacy temperature demo running directly on top of IEEE 802.15.4 MAC. 
| ot-barebone-mtd| Barebones OpenThread demo, designed for the Chili 1.
| ot-barebone-ftd | The FTD version of ot-barebone-mtd
| chili2-default-secure | The basic secure FreeRTOS & TrustZone binary, containing FreeRTOS secure code and the secure parts of the BSP

### Hosted Targets

These applications run on POSIX, Windows or OSX systems. They interface with Chili devices connected by USB or UART. [Click here for more documentation.](posix/ca821x-posix/README.md).

| CMake Target Name | Description |
| :--- | :--- |
| serial-adapter | Exposes the serial interface of examples such as: ot-cli*, ocf-*, ot-ncp. [Click here for more documentation.](posix/app/README.md)
| ot-cli-posix-ftd | The OpenThread command line application, running on a host. Requires a mac-dongle Chili to be connected to the host.
| ot-cli-posix-mtd | Same as above, but acts as a Minimal Thread Device. Requires a mac-dongle Chili to be connected to the host.
| ot-ncp-posix | Enable a computer to act as a Thread Border Router. Requires a mac-dongle Chili to be connected to the host.
| ot-eink-server | Server that transmits image files, to be used with the ot-sed-eink-freertos embedded target. Requires a mac-dongle Chili to be connected to the host. [Click here for more documentation.](posix/app/ot-eink-server/Readme-eink.md)
| ot-sensordemo-server | Interfaces with the Cascoda sensordemo application layer. It prints the sensor readings it receives from the network. Requires a mac-dongle Chili to be connected to the host.
| sniffer |  Captures raw 802.15.4 traffic. Compatible with WireShark. Requires a mac-dongle Chili to be connected to the host. [Click here for more documentation.](posix/app/README.md)
| evbme-get | Prints all the EVBME parameters of a connected Chili, including application name, version and joiner credentials. [Click here for more documentation.](posix/app/README.md)
| security-test | Tests the advanced security mode of the CA-8211.
| stress-test | Requires several mac-dongle Chilis to be connected to the host. Creates heavy 802.15.4 traffic. [Click here for more documentation.](posix/app/README.md)
| serial-test | Stress test of the serial connection between the host and the connected Chili. Requires a Chili to be connected to the host. [Click here for more documentation.](posix/app/README.md)

## Debugging
The Chilis support flashing and debugging via the [Segger J-Link](https://www.segger.com/products/debug-probes/j-link/) using [JTAG SWD](https://en.wikipedia.org/wiki/JTAG#Serial_Wire_Debug). When using the GCC toolchain, the SEGGER GDB Server and arm-none-eabi-gdb can be used to flash and debug the Chili. Simply setup the JLink GDB server for the NANO120 (Chili 1) or M2351 (Chili 2) with SWD, then `target remote 127.0.0.1:2331` in arm-none-eabi-gdb to connect to it. If debugging is not required, then the Segger J-Flash lite tool can flash plain binary files.

The Chilis can also be [debugged with a NuLink Pro.](docs/guides/how-to-debug-with-a-nu-link-pro.md). This is a cheaper alternative, but the developer experience is inferior - you cannot flash the Chili from within GDB, and setting & getting to breakpoints is slower.

## Directory layout

### ca821x-api
[ca821x-api](ca821x-api/README.md) contains the cross-platform API which abstracts all of the functionality of the CA-8210 and CA-8211. It is required for every project.

### baremetal 
[baremetal](baremetal/cascoda-bm-driver/README.md) contains the cross-platform baremetal drivers, some example applications, and a set of platform abstractions. The baremetal drivers implement useful, cross-platform functionality, and should be your go-to API when trying to interact with peripherals and outside world.

The platform abstractions (sometimes referred to in the source code as the Board Support Package) are what enables the baremetal drivers to be cross-platform. They provide a set of functions which abstract away the specifics of what platform you are dealing on, such as how to set up the device and how to communicate with peripherals.

While you _could_ use the BSP functions (declared in `cascoda_interface.h`) to control the chip from your top-level application, it is a lot easier to rely on the functionality provided by the baremetal drivers, such as the functions in `cascoda_time.h`, `cascoda_evbme.h` and so on.

### docs
`docs` contains the following guides:

- [Thread Commissioning](docs/guides/thread-commissioning.md)
- [Cross-compiling for the Raspberry Pi](docs/guides/how-to-cross-compile-for-the-raspberry-pi.md)
- [M2351 TrustZone Development Guide](docs/guides/M2351-TrustZone-development-guide.md)
- [Debugging with a NuLink Pro](docs/guides/how-to-debug-with-a-nu-link-pro.md)

### toolchain
`toolchain` contains the platform configuration files to enable cross compilation for different systems and compilers. These are used to first set up the CMake toolchain for a specific platform, as seen in [the Instructions settings](#instructions). There is extra info on how to compile for different targets in docs, such as [cross compiling for the raspberry pi.](docs/guides/how-to-cross-compile-for-the-raspberry-pi.md)

### posix
[posix](posix/ca821x-posix/README.md) contains the Posix-specific drivers and tools, as well as some example applications that can be run from a Linux system.

### cascoda-utils
`cascoda-utils` Contains a cross-platform library of useful utilities, such as tasklets for simple scheduling and hash functions for basic hashing functionality.

### openthread
`openthread` contains the glue configuration to download the openthread repository from https://github.com/Cascoda/openthread, and configure it to be built with the SDK.

### etc
`etc` contains miscellaneous resources.

## CMake Library Targets

Useful CMake target libraries to include:

| Target | Description |
| :---   | :--- |
| ca821x-api | The core Cascoda API, including C function representations of all CA-821x functionality. |
| cascoda-bm | The baremetal Cascoda Drivers, abstracting away the platform to a common interface, and providing a suite of helper functions. |
| cascoda-utils | Cross-platform utility functions.
| ca821x-posix | The posix Cascoda driver interface, supporting USB, UART or SPI communications to a connected Cascoda device |
| sensorif | A library containing drivers for interfacing with I2C sensors |
| ca821x-openthread-bm-ftd | OpenThread baremetal library for FTDs (Full Thread Devices)|
| ca821x-openthread-bm-mtd | OpenThread baremetal library for MTDs (Minimal Thread Devices) |
| ca821x-openthread-posix-ftd | OpenThread posix library for FTDs |
| ca821x-openthread-posix-mtd | OpenThread posix library for MTDs |
| openthread-cli-ftd | OpenThread CLI library for FTDs |
| openthread-cli-mtd | OpenThread CLI library for MTDs |
| openthread-ncp-ftd | OpenThread NCP library for FTDs |
| openthread-ncp-mtd | OpenThread NCP library for MTDs |
| freertos | The main FreeRTOS code |

## Useful Links

- [Cascoda Posix Thread Description](posix/ca821x-posix-thread/Readme.md)
- [Baremetal Temperature Sensing Application](baremetal/app/mac-tempsense/README.md)
