# platform
Platform abstraction layer code linking cascoda-bm-driver to vendor libraries for specific modules and MCUs.<br>
Implements the interface functions declared in cascoda-bm/cascoda_interface.h.<br>

The following platforms are currently implemented:<br>

| Platform | Module / Board | MCU |
| :--- | :--- | :--- |
| cascoda-nuvoton-chili | Chili 1 | Nuvoton Nano120 |
| cascoda-nuvoton-chili2 | Chili 2 | Nuvoton M2351 |
| cascoda-st-h152 | Olimex STM32-H152 | ST STM32L152 |
| cascoda-dummy-posix |dummy posix platform for test purposes | -|

Support for additional MCUs and modules will be added here.
