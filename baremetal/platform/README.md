# platform
Platform abstraction layer code linking cascoda-bm-driver to vendor libraries for specific modules and MCUs.<br>
Implements the interface functions declared in cascoda-bm/cascoda_interface.h.

The following platforms are currently implemented:

| Platform | Module / Board | MCU |
| :--- | :--- | :--- |
| chili | Chili 1 | Nuvoton Nano120 |
| chili2 | Chili 2 | Nuvoton M2351 |
| dummy-posix |dummy posix platform for test purposes | -|

Support for additional MCUs and modules will be added here.
