# Cascoda SDK Reference Document

## Contents

<!-- TOC -->

- [Cascoda SDK Reference Document](#cascoda-sdk-reference-document)
  - [Contents](#contents)
  - [Guides](#guides)
    - [Product Guides](#product-guides)
    - [Developer Guides](#developer-guides)
    - [Complete API Reference](#complete-api-reference)
    - [Reference Documents](#reference-documents)
  - [Building](#building)
    - [Instructions](#instructions)
      - [Linux/MacOS](#linuxmacos)
      - [Windows](#windows)
  - [Example Applications](#example-applications)
    - [Embedded Applications](#embedded-applications)
      - [OpenThread CLI Applications](#openthread-cli-applications)
      - [OpenThread Standalone Applications](#openthread-standalone-applications)
      - [OCF Applications](#ocf-applications)
      - [Development Board Applications](#development-board-applications)
      - [Miscellaneous Applications](#miscellaneous-applications)
    - [Hosted Applications](#hosted-applications)
  - [Debugging](#debugging)
  - [Directory layout](#directory-layout)
    - [ca821x-api](#ca821x-api)
    - [baremetal](#baremetal)
    - [docs](#docs)
    - [toolchain](#toolchain)
    - [posix](#posix)
    - [cascoda-utils](#cascoda-utils)
    - [openthread](#openthread)
    - [etc](#etc)
  - [CMake Library Targets](#cmake-library-targets)

<!-- /TOC -->

## Guides

### Product Guides

<!-- TODO add description -->
- [Getting started with the Cascoda KNX IoT Dev Kit](../product/howto-knxiot-devkit.md): Use the Development Kit running KNX-IoT, using ETS
- [Getting Started with the Cascoda Packet Sniffer](../product/howto-sniffer.md): capture and analyze IEEE 802.15.4, Thread, CoAP, OSCORE
- [Getting Started with the Cascoda Thread Evaluation Kit](../product/howto-thread.md): Set up a Thread network and test simple communications. 
- [Getting Started with the Cascoda Thread Development Board](../product/howto-devboard.md): Learn about the features of the development board, and run two example applications.
- [Getting Started with OCF and Thread](../product/howto-ocf-thread.md): Run pre-existing OCF applications or create your own.

### Developer Guides

- [Development Environment Setup](../dev/development-setup.md)
- [Arm GNU Toolchain Installation for Linux](../dev/arm-gnu-toolchain-installation.md)
- [Thread Network Formation](../dev/thread-network-formation.md)
- [Thread Commissioning](../dev/thread-commissioning.md)
- [Flashing binaries to hardware](../dev/flashing.md)
- [Debugging with a SEGGER J-Link](../dev/debug-with-segger-jlink.md)
- [Debugging with a NuLink Pro](../dev/debug-with-a-nu-link-pro.md)
- [OCF New Application guide](../dev/create-custom-ocf-applications.md)
- [Cross-compiling for the Raspberry Pi](../dev/cross-compile-for-the-raspberry-pi.md)
- [M2351 TrustZone Development Guide](../dev/M2351-TrustZone-development-guide.md)
- [LWM2M over Thread](../dev/lwm2m-over-thread.md)
- [Building binaries for production](../dev/building-for-production.md)

### Complete API Reference

- [Cascoda SDK API Reference](https://cascoda.github.io/cascoda-sdk-doxygen/)

### Reference Documents

- [The Cascoda TLV message format](../reference/cascoda-tlv-message.md)
- [The Cascoda UART interface](../reference/cascoda-uart-if.md)
- [Configuring with CMake](../reference/cmake-configuration.md)
- [Possible system architectures](../reference/system-architecture.md)
- [Evaluation Board Management Entity](../reference/evbme.md)

## Building

The Cascoda SDK takes advantage of open source tools, such as Git, GCC, CMake and Doxygen. It is possible to build the Cascoda SDK natively, or cross compile for embedded platforms. Several different compilers and targets are supported, and this process is driven by CMake. CMake is the build tool used for configuring the build system for a given compiler and target, and also selecting build options.

### Instructions

To build the Cascoda SDK, first configure your environment as detailed [here](../dev/development-setup.md), then follow the instructions below:

#### Linux/MacOS

Either type, or copy and paste the following into a bash terminal:

```bash
# Make a working directory (this can be anywhere, named anything)
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
Constructed directory layout:
```bash
cascoda #Working directory
|-cascoda-sdk # Source directory, shared by both build directories. see 'Directory Layout' section below
|-sdk-posix   # Native build directory of SDK.
| |-lib        # Static libraries are built in here for native use.
| |-bin        # Executables are built in here for native use. See 'hosted applications' below
|-sdk-chili2  # Chili2 build directory of SDK
| |-lib        # Static libraries are built in here for Chili2 use.
| |-bin        # Firmware binaries are built in here for the Chili2. See 'Embedded applications' section.
```

#### Windows

Either type, or copy and paste the following into a git bash terminal:

(Make sure that the path is set up correctly, as explained in [the development setup guide.](../dev/development-setup.md))

```bash
# Make a working directory (this can be anywhere, named anything)
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
Constructed directory layout:
```bash
cascoda #Working directory
|-cascoda-sdk # Source directory, shared by both build directories. see 'Directory Layout' section below
|-sdk-win     # Native build directory of SDK.
| |-lib        # Static libraries are built in here for native use.
| |-bin        # Executables are built in here for native use. See 'hosted applications' below
|-sdk-chili2  # Chili2 build directory of SDK
| |-lib        # Static libraries are built in here for Chili2 use.
| |-bin        # Firmware binaries are built in here for the Chili2. See 'Embedded applications' section.
```

Libraries will be built into the ``lib/`` directory, while application binaries will be built into the ``bin/`` directory. For the Chili platforms, both elf format and binary .bin format files will be created.
These can then be flashed to the device by following the [flashing guide.](../dev/flashing.md)

For a description of the different built binaries & executables, see [the embedded applications section](#embedded-applications) for chili platforms, and [the hosted applications](#hosted-applications) for Windows/Linux/macOS.



In order to compile for the Chili 1, or to use a different compiler, the CMAKE_TOOLCHAIN_FILE argument can be pointed to a different configuration file in the toolchain directory.

## Example Applications

If you are looking to test existing functionality of the Cascoda SDK, start here. This is a list of the demos within the Cascoda SDK, alongside a short description of what each of them do.

### Embedded Applications 

#### OpenThread CLI Applications

| Binary Name | Description |
| :--- | :--- |
| ot-cli | The OpenThread command line interface running on a Chili. Interfaces with the Cascoda sensordemo application layer. Works with the serial-adapter POSIX application. [More information.](../../baremetal/cascoda-bm-thread/example/README.md)
| ot-cli-lwip | A demonstration of the Lightweight IP Stack running on top of OpenThread. [More information.](../../baremetal/app/ot-cli-lwip/README.md)
| ot-cli-lwip-freertos | Same as above, but taking advantage of the POSIX socket API and FreeRTOS. [More information.](../../baremetal/app/ot-cli-lwip-freertos/README.md)
| ot-cli-actuator | Similar to ot-cli, but also has actuator commands. [More information.](../../baremetal/app/ot-cli-actuator/README.md)
| ot-cli-lwm2m | Similar to ot-cli, but with LWM2M client support using wakaama. [More information](../../baremetal/app/ot-cli-lwm2m/README.md)

#### OpenThread Standalone Applications

| Binary Name | Description |
| :--- | :--- |
| ot-sed-thermometer | Sleepy end device that interfaces with the Cascoda sensordemo application layer. Only reports temperature. [More information.](../../baremetal/app/ot-sed-thermometer/README.md)
| ot-sed-thermometer-freertos | Same as above, but taking advantage of FreeRTOS concurrency features. [More information.](../../baremetal/app/ot-sed-thermometer-freertos/README.md)
| ot-sed-sensorif | Sleepy end device that interfaces with the Cascoda sensordemo application layer. Connects to various I2C and SPI sensors. [More information.](../../baremetal/app/ot-sed-sensorif/README.md)
| ot-sed-eink-freertos | Display images onto an e-ink screen. The images are transmitted over a Thread network. The system sleeps while idle. Used in conjunction with ot-eink-server. [More information.](../../baremetal/app/ot-sed-eink-freertos/README.md)

#### OCF Applications 

| Binary Name | Description |
| :--- | :--- |
| ocf-light | Use a Chili as a controller for an OCF-over-Thread smart lightbulb. [More information.](../../ocf/README.md)
| ocf-reed-light | Use a Router Enabled Chili as a controller for an OCF-over-Thread smart lightbulb.
| ocf-sensorif | Use a Chili as a OCF-over-Thread device connected to various I2C and SPI sensors.
| ocf-cli-thermometer  | Use a Chili as a OCF-over-Thread smart thermometer. This demo uses the in-built thermometer of the M2351, so no external hardware is required.
| ocf-sleepy-thermometer  | Same as above, but the device sleeps between temperature readings.
| ocf-storage-test  | Test the flash storage API of Iotivity-Lite. Useful for developers.

#### Development Board Applications

| Binary Name | Description|
| --- |  --- |
| devboard-blinky |  Example app demonstrating intrinsic devboard features: LEDs and buttons. [More information](../../baremetal/cascoda-bm-devboard/README.md).
| devboard-click |  Example app demonstrating how the devboard can communicate with externally connected peripherals. [More information](../../baremetal/cascoda-bm-devboard/README.md).

#### Miscellaneous Applications

| Binary Name | Description |
| :--- | :--- |
| mac-dongle | Use a Chili as a IEEE 802.15.4 MAC dongle, enabling hosted platforms to communicate using this protocol. Used by most POSIX applications. [More information.](../../baremetal/app/mac-dongle/README.md)
| eink-test-x-x | Tests the connection between a Chili and a E-Ink display (where x-x is replaced by the size, e.g. 2-9 meaning 2.9 inches) by showing an image. Does not commmunicate over IEEE 802.15.4. [More information.](../../baremetal/app/eink-bm/README.md)
| sensorif-test | Tests the sensorif API by attempting communication with all connected sensors. [More information.](../../baremetal/app/sensorif-bm/README.md)
| mikrosdk-test | Tests the mikrosdk API by attempting communication with all connected MikroElekronica sensors/actuators. [More information.](../../baremetal/app/mikrosdk-bm/README.md)
| pwm-led-dimming | Tests and demonstrates the PWM API. [More information.](../../baremetal/app/pwm-led-dimming/README.md)
| mac-tempsense | Legacy temperature demo running directly on top of IEEE 802.15.4 MAC. [More information.](../../baremetal/app/mac-tempsense/README.md)
| ot-ncp |The OpenThread network co-processor interface running on a Chili. Works with the serial-adapter POSIX application.
| ot-barebone-mtd| Barebones OpenThread demo, designed for the Chili 1. [More information.](../../baremetal/app/ot-barebone/README.md)
| ot-barebone-ftd | The FTD version of ot-barebone-mtd. [More information.](../../baremetal/app/ot-barebone/README.md)
| chili2-default-secure | The basic secure FreeRTOS & TrustZone binary, containing FreeRTOS secure code and the secure parts of the BSP. [More information.](../../baremetal/platform/chili2/chili2-default-secure/README.md)
| knx-storage-benchmark | Measure the performance of the persistent storage API. Useful for optimizing the read & write speed of persistent KNX data. Use serial-adapter to access the output of the test. |
| knx-storage-check | Verify the correctness of the persistent storage API. Reads & writes data and makes sure that the data on flash exactly matches what was written. Use serial-adapter to access the result of the test |

### Hosted Applications

These applications run on POSIX, Windows or OSX systems. They interface with Chili devices connected by USB or UART. [More information.](../../posix/ca821x-posix/README.md).

| Binary Name | Description |
| :--- | :--- |
| chilictl | Generic Chili control application allowing listing and flashing of any connected Chili2 devices. [More information.](../../posix/app/chilictl/README.md)
| serial-adapter | Exposes the serial interface of examples such as: ot-cli*, ocf-*, ot-ncp. [More information.](../../posix/app/serial-adapter/README.md)
| knx-gen-data | Generate persistent manufacturer data for KNX-IoT Chilis. [More information.](../../posix/app/knx-gen-data/Readme.md) |
| sniffer |  Captures raw 802.15.4 traffic. Compatible with WireShark. Requires a mac-dongle Chili to be connected to the host. [More information.](../../posix/app/sniffer/README.md)
| ot-cli-posix-ftd | The OpenThread command line application, running on a host. Requires a mac-dongle Chili to be connected to the host.
| ot-cli-posix-mtd | Same as above, but acts as a Minimal Thread Device. Requires a mac-dongle Chili to be connected to the host.
| ot-ncp-posix | Enable a computer to act as a Thread Router. Requires a mac-dongle Chili to be connected to the host.
| ot-eink-server | Server that transmits image files, to be used with the ot-sed-eink-freertos embedded application. Requires a mac-dongle Chili to be connected to the host. [More information.](../../posix/app/ot-eink-server/README.md)
| ot-sensordemo-server | Interfaces with the Cascoda sensordemo application layer. It prints the sensor readings it receives from the network. Requires a mac-dongle Chili to be connected to the host. [More information.](../../posix/app/ot-sensordemo-server/README.md)
| ocfctl | Control application for OCF devices, useful mainly for certification. [More information.](../../posix/app/ocfctl/README.md) |
| evbme-get | Prints all the EVBME parameters of a connected Chili, including application name, version and joiner credentials. [More information.](../../posix/app/tests/README.md)
| security-test | Tests the advanced security mode of the CA-8211.
| stress-test | Requires several mac-dongle Chilis to be connected to the host. Creates heavy 802.15.4 traffic. [More information.](../../posix/app/tests/README.md)
| serial-test | Stress test of the serial connection between the host and the connected Chili. Requires a Chili to be connected to the host. [More information.](../../posix/app/tests/README.md)
| zigbee-nwk-test | Tests the currently implemented functionality of the ZigBee network layer. Requires several mac-dongle Chilis to be connected to the host.

## Debugging

The Chilis support flashing and debugging via the [Segger J-Link](https://www.segger.com/products/debug-probes/j-link/) using [JTAG SWD](https://en.wikipedia.org/wiki/JTAG#Serial_Wire_Debug). When using the GCC toolchain, the SEGGER GDB Server and arm-none-eabi-gdb can be used to flash and debug the Chili. Simply setup the JLink GDB server for the NANO120 (Chili 1) or M2351 (Chili 2) with SWD, then `target remote 127.0.0.1:2331` in arm-none-eabi-gdb to connect to it. If debugging is not required, then the Segger J-Flash lite tool can flash plain binary files.

The Chilis can also be [debugged with a NuLink Pro](../dev/debug-with-a-nu-link-pro.md). This is a cheaper alternative, but the developer experience is inferior - you cannot flash the Chili from within GDB, and setting & getting to breakpoints is slower.

## Directory layout

### ca821x-api

[ca821x-api](../../ca821x-api/README.md) contains the cross-platform API which abstracts all of the functionality of the CA-8210 and CA-8211. It is required for every project.

### baremetal
[baremetal](../../baremetal/Readme.md) contains the cross-platform baremetal drivers, some example applications, and a set of platform abstractions. The baremetal drivers implement useful, cross-platform functionality, and should be your go-to API when trying to interact with peripherals and outside world.

The platform abstractions (sometimes referred to in the source code as the Board Support Package) are what enables the baremetal drivers to be cross-platform. They provide a set of functions which abstract away the specifics of what platform you are dealing on, such as how to set up the device and how to communicate with peripherals.

While you _could_ use the BSP functions (declared in `cascoda_interface.h`) to control the chip from your top-level application, it is a lot easier to rely on the functionality provided by the baremetal drivers, such as the functions in `cascoda_time.h`, `cascoda_evbme.h` and so on.

### docs

`docs` contains the guides [outlined above.](#guides)

### toolchain

`toolchain` contains the platform configuration files to enable cross compilation for different systems and compilers. These are used to first set up the CMake toolchain for a specific platform, as seen in [the Instructions settings](#instructions). There is extra info on how to compile for different platforms in docs, such as [cross compiling for the raspberry pi.](../dev/cross-compile-for-the-raspberry-pi.md)

### posix

[posix](../../posix/ca821x-posix/README.md) contains the Posix-specific drivers and tools, as well as some example applications that can be run from a Linux system.

Useful documents:

- [Cascoda OpenThread Platform layer](../../posix/ca821x-posix-thread/Readme.md)

### cascoda-utils

`cascoda-utils` Contains a cross-platform library of useful utilities, such as tasklets for simple scheduling and hash functions for basic hashing functionality.

### openthread

`openthread` contains the glue configuration to download the openthread repository from [https://github.com/Cascoda/openthread](https://github.com/Cascoda/openthread), and configure it to be built with the SDK.

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

