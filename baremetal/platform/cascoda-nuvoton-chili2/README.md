# cascoda-nuvoton-chili2
This repository contains baremetal drivers and example applications for the Cascoda 'Chili 2' module.

## Toolchains
The build system used is cmake, which handles downloading dependencies, configuration and building. CMake is cross-platform, can create project files for several popular IDEs, and supports a wide range of compilers. Currently, the applications can be built using arm-none-eabi-gcc, but support for Keil/IAR is planned for the future.<br>
Supported toolchains/IDEs include:<br>
[GNU ARM Embedded Toolchain](https://developer.arm.com/open-source/gnu-toolchain/gnu-rm)<br>

Unfortunately due to the way openthread is configured upstream, it can only be built on a unix system.

## Module structure
### test15-4, tempsense, ot-bm
Example applications. These link with the cascoda-bm-driver and ca821x-api. test15-4 is a plain application that allows USB control of the CA-821x via wing commander. tempsense is a demo application involving communicating temperature readings to a coordinator. ot-bm is the baremetal implementation of the openthread Thread stack.

### cascoda-bm-driver
Baremetal drivers for Cascoda devices and supporting host functionality. The api for this remains consistent
across all cascoda-supported baremental platforms, and is used for all of our example applications. This must be linked with cascoda-nuvoton-chili2 (or other platform), and the ca821x-api. The SPI driver logic and binding to the ca821x-api is in this module.

### cascoda-api
The abstract cascoda-api used to interface with the CA-8210 and CA-8211 on every platform. This consists of a collection of functions and structures that very closely match the primitives from IEEE-802.15.4:2006. Please read the IEEE-802.15.4:2006 specification and [our relevant datasheet](https://www.cascoda.com/product/) for more information.

### cascoda-nuvoton-chili2
Platform abstraction layer which implements the BSP functions from cascoda-bm-driver, and provides all of the platform-specific code required. This comprises of 2 sections, port and vendor.
* __port__ - Platform-specific code linking cascoda-bm-driver to vendor libraries.
* __vendor__ - Libraries provided by MCU manufacturer (Nuvoton)

## Development
CMake is required to build the cascoda SDK. A version of 3.12 or later is required, and is available on the [cmake website](https://cmake.org/download/). To get started, clone this repository and make a seperate build directory. Then cd into the build directory and run cmake.
```
mkdir cascoda-nuvoton-chili2
cd cascoda-nuvoton-chili2
mkdir build
git clone https://github.com/Cascoda/cascoda-nuvoton-chili2.git
cd build
cmake ../cascoda-nuvoton-chili2/ -DCMAKE_TOOLCHAIN_FILE=../cascoda-nuvoton-chili2/arm-none-eabi.cmake
```
This will clone all of the relevant subprojects and configure them. Following this, to access the configuration panel, use the cmake cache editor in the build directory. You can change options, then reconfigure with 'c' and generate makefiles with 'g'. After configuration and generation, you can build everything with make.
```
ccmake .
make
```

### Debugging
The Chili supports flashing and debugging via the [Segger J-Link](https://www.segger.com/products/debug-probes/j-link/) using [JTAG SWD](https://en.wikipedia.org/wiki/JTAG#Serial_Wire_Debug). The SEGGER GDB Server and arm-none-eabi-gdb can be used to flash and debug the chili. Simply setup the GDB server for the Cortex-M23 with SWD, then `target remote 127.0.0.1:2331` in gdb.
If using IAR to build and debug, there are some settings to enable JTAG debugging:
1. In "General Options"->"Debugger" on the "Setup" tab change the driver drop-down to" J-Link/J-Trace".
2. On the "Download" tab check the "Use flash loader(s)" box to enable flashing with the J-link.
3. Then in "General Options"->"Debugger"->"J-Link/J-Trace" on the "Connection" tab change "Interface" from "JTAG" to "SWD".
