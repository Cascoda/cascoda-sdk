# CMake Configuration

The Cascoda SDK is organised to be built with CMake, which allows for natively building for Windows, Linux or macOS, as well
as cross compiling for the Chili platforms.

It is expected that building the Cascoda SDK is done 'out of source' - that is, a separate directory to the source code
is used for creating build artifacts and temporary files. This allows several different configurations to be active and
buildable from the same source code, and prevents accidentally polluting the source directories and version control.

For instance, you can clone the Cascoda SDK once, and build the Windows, Chili2-USB and Chili2-UART versions in different
build directories, without affecting the source, which is very convenient.

This reference details the available CMake configuration options for the Cascoda SDK. It expects that you already have build
directories set up as detailed [in the build guide](../../README.md#building), and are aware of how to
[change cmake configurations.](https://cmake.org/runningcmake/)

This list has some of the more important variables, but more are available, and their documentation can be viewed in the CMake tool (ccmake or cmake-gui) itself.

## General

### CASCODA_CA_VER

The version of the Cascoda hardware to build firmware for. Set to ``8211`` for the CA-8211, or ``8210`` for the CA-8210.

### CASCODA_LOG_LEVEL

The logging level to print messages for. From most verbose to least verbose:

1. DEBG
2. INFO
3. NOTE _(default)_
4. WARN
5. CRIT

## Baremetal

### CASCODA_BM_INTERFACE

Which serial interface should the device use to communicate to the host system?

| Value | Meaning |
| ----- | ------- |
| USB   | Use the Cascoda serial-over-USB-HID protocol, which allows driverless communication to programs running on the host.
| UART  | Use the [Cascoda acknowledged UART](cascoda-uart-if.md) protocol to programs running on the host.
| NONE  | Disable host communication and don't build in the drivers for it. For space saving for headless devices.

### CASCODA_BUILD_OCF

Build the [OCF](https://openconnectivity.org/) libraries and binaries, and internally modify the build of the other systems to support it. [More information](../../ocf/README.md)

Incompatible with CASCODA_BUILD_SECURE_LWM2M.

This option also disables the Test15-4 low-level debug interface globally, which breaks targets such as `mac-dongle`. It also prevents running commands with Wing Commander.

### CASCODA_BUILD_LWIP

Build the [LWIP](https://savannah.nongnu.org/projects/lwip/) libraries and binaries, and internally modify the build of other systems to support it.

### CASCODA_BUILD_SECURE_LWM2M

Build the [LWM2M](https://en.wikipedia.org/wiki/OMA_LWM2M) libraries and binaries, and internally modify the build of other systems to support it. [More information.](../../baremetal/app/ot-cli-lwm2m)

Incompatible with CASCODA_BUILD_OCF.

### CASCODA_CHILI2_CONFIG_STRING

Set the target configuration for the firmware.

| Value | Meaning |
| ----- | ------- |
| ONE_SIDED | Configured for the Chili2S
| TWO_SIDED | Configured for the Chili2D
| DEV_BOARD | Configured for the [Development Board](../how-to/howto-devboard.md)
