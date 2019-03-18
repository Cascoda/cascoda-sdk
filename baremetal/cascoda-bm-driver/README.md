# cascoda-bm-driver
This repository contains portable firmware for baremetal platforms using Cascoda communications devices.

Cascoda's CA821X API project is included as a CMake subproject and hooked in to the SPI driver in this package.

## Usage
When using this package in an application, there are some requirements that must be fulfilled in addition to those of ca821x-api.
- The function `cascoda_serial_dispatch` should be populated with a function to handle commands received over the serial interface. This pointer declaration can be found in include/cascoda_serial.h.
- The function `EVBMEInitialise` must be called before using the transceiver. This function accepts an argument of a string describing the calling application e.g. "Example App v1.0".
- The function `cascoda_io_handler` must be called regularly from the main program. This function processes messages received over available interfaces and calls the relevant dispatch functions.

## Porting to a new platform
When porting to a new baremetal platform, there are are also requirements that must be fulfilled in order to run
the SPI driver for the CA-821x, and to run the basic example applications.
- All required functions in `cascoda-bm/cascoda_interface.h` should be implemented. All non-required optional features should be implemented as stubs.
- The callback functions in `cascoda-bm/cascoda_interface.h` such as `button1_pressed` and should be called when the given event occurs - but only if they are populated and not-null.
- The TIME_1msTick() Function should be called once per millisecond by the BSP - except when in an EVME sleep mode.
- If I2C sensor connectivity is desired, then the functions in `cascoda-bm/cascoda_sensorif.h` should be implemented. If not, they can be implemented as stubs returning `SENSORIF_I2C_ST_NOT_IMPLEMENTED`.
