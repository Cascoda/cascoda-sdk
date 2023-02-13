# cascoda-bm-core
This directory contains the core portable firmware for baremetal platforms using a CA-821x transceiver.

The cascoda-bm-core module contains the base functionality for communicating with a CA-821x using the ca821x-api. This is the minimum functionality required for the ca821x-api to be used on a baremetal platform. The cascoda-bm-driver module contains extended functionality including the device side of the EVBME commands and other system control.

Please see the [CA-8211 datasheet](https://www.cascoda.com/wp-content/uploads/2019/06/CA-8211_datasheet_0119.pdf) for more detailed information on the SPI protocol.

## Usage
Usually, and in all of the examples, this module is controlled by the extended cascoda-bm-drivers module. However, if this not the case, for instance - integrating a CA-821x into an existing system, then the functions inside the ``cascoda_interface_core.h`` file should be implemented. The ``DISPATCH_ReadCA821x`` function should be called when an IRQ request arrives from the CA-821x.

## Porting to a new platform
When porting to a new baremetal platform, there are are also requirements that must be fulfilled in order to run
the SPI driver for the CA-821x, and to run the basic example applications.
- All functions in `cascoda-bm/cascoda_interface_core.h` should be implemented.
