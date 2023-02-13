# cascoda-bm-driver
The cascoda-bm-core module contains the base functionality for communicating with a CA-821x using the ca821x-api. This module contains extended functionality including the device side of the EVBME commands and other system control.

## Usage
When using this package in an application, there are some requirements that must be fulfilled in addition to those of ca821x-api. See any of the example apps in the app/ subdirectory as a guide.
- The function `cascoda_serial_dispatch` can be populated with a function to handle commands received over the serial interface (UART/USB). This can be used in order to handle application-specific commands in addition to the base set provided by the EVBME and CA-821x. Commands must be in the standard Cascoda TLV format.
- The function `EVBMEInitialise` must be called before using the transceiver. This function accepts an argument of a string describing the calling application e.g. "Example App v1.0".
- The function `cascoda_io_handler` must be called regularly from the main program. This function processes messages received over available interfaces and calls the relevant dispatch functions.
- Call any of the functions in the `include/cascoda-bm` subdirectory in order to control the underlying hardware.

## Porting to a new platform
When porting to a new baremetal platform, there are are also requirements that must be fulfilled in order to run
the SPI driver for the CA-821x, and to run the basic example applications.
- The ``cascoda-bm-core`` module should be ported ([see here](../cascoda-bm-core/README.md))
- All required functions in `cascoda-bm/cascoda_interface.h` should be implemented. All non-required optional features should be implemented as stubs.
- If I2C sensor connectivity is desired, then the functions in `cascoda-bm/cascoda_sensorif.h` should be implemented. If not, they can be implemented as stubs returning `SENSORIF_I2C_ST_NOT_IMPLEMENTED`.
