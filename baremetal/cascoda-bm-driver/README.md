# cascoda-bm-driver
This directory contains portable firmware for baremetal platforms using Cascoda communications devices.

This driver acts as the underlying platform layer for ca821x-api on baremetal platforms.

## Usage
When using this package in an application, there are some requirements that must be fulfilled in addition to those of ca821x-api. See any of the example apps in the app/ subdirectory as a guide.
- The function `cascoda_serial_dispatch` can be populated with a function to handle commands received over the serial interface (UART/USB). This pointer declaration can be found in cascoda-bm/cascoda_serial.h. Commands must be in the standard Cascoda TLV format.
- The function `EVBMEInitialise` must be called before using the transceiver. This function accepts an argument of a string describing the calling application e.g. "Example App v1.0".
- The function `cascoda_io_handler` must be called regularly from the main program. This function processes messages received over available interfaces and calls the relevant dispatch functions.
- Call any of the functions in the `include/cascoda-bm` subdirectory in order to control the underlying hardware.

## Porting to a new platform
When porting to a new baremetal platform, there are are also requirements that must be fulfilled in order to run
the SPI driver for the CA-821x, and to run the basic example applications.
- All required functions in `cascoda-bm/cascoda_interface.h` should be implemented. All non-required optional features should be implemented as stubs. Any function that is prefixed by `BSP_` must be implemented by the platform layer.
- If I2C sensor connectivity is desired, then the functions in `cascoda-bm/cascoda_sensorif.h` should be implemented. If not, they can be implemented as stubs returning `SENSORIF_I2C_ST_NOT_IMPLEMENTED`.
