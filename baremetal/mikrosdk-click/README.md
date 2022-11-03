# mikrosdk-click

Example of MikroElectronika Click interfaces for adding external sensors or actuator support to applications. These modules abstract away the underlying I2C/SPI bus and present easy-to-use functions in C.

### List of supported Mikroelectronikca devices:

Device               | Interface | Type | On-chip Sensors | Declarations for Interface Functions |
:--------------------| :-------- | :--- |:-------------|:-------------|
[Air Quality 4 Click](https://www.mikroe.com/air-quality-4-click)  | I2C | Air Quality (Gas) sensor |SGP30| airquality4.h  |
[Environment2 Click](https://www.mikroe.com/environment-2-click)   | I2C | Air Quality, Temperature, and Humidity sensor | SHT40, SGP40 | environment2.h |
[HVAC Click Bundle](https://www.mikroe.com/hvac-click-bundle)    | I2C | Particulate Matter, Air Quality sensor |SCD41, SPS30|  hvac.h |
[SHT Click](https://www.mikroe.com/sht-click)    | I2C | Temperature and Humidity |SHT3x|  sht.h |
[Motion Click](https://www.mikroe.com/motion-click)         | GPIO | Motion sensor |  PIR500B | motion.h |
[Relay Click](https://www.mikroe.com/relay-click)    | GPIO | Relay |  G6D-1A-ASI DC5 | relay.h |
[Thermo Click](https://www.mikroe.com/thermo-click)         | SPI | Thermocouple Temperature sensor | MAX31855K|  thermo.h |
[Thermo 3 Click](https://www.mikroe.com/thermo-3-click)        | I2C | Digital Temperature sensor | TMP102| thermo3.h |

### List of devices:
Manufacturer | Device  | Click board | Usage | Tested |
:--------------------|:--------------------|:--------------------| :-------- | :-------------|
Sensirion | SGP30 | Air Quality 4 Click| TVOC, CO2 | Yes |
Sensirion | SHT40 | Environment2 Click | Humidity, Temperature | Yes |
Sensirion | SGP40 | Environment2 Click | VOC | Yes |
Sensirion | SCD41 | HVAC Click Bundle | CO2 | Yes |
Sensirion | SPS30 | HVAC Click Bundle | PM | No |
Sensirion | SHT3x | SHT Click  | Humidity, Temperature | Yes |
| - | PIR500B | Motion Click | Motion | Yes
OMRON G6D | G6D-1A-ASI DC5 | Relay Click | Relay | Yes
Maxim Integrated | MAX31855 | Thermo Click | Temperature | Yes |
Texas Instruments | TMP102 | Thermo 3 Click | Temperature | Yes |



 > The SPS30 files that reside in include and source folders are not tested due to malfunctioning of SPS30. They were devloped intended for the use of testing SPS30 via UART communcation.


 > The ``MAP_MIKROBUS`` function in all sensor header files gives the user a clear sense of the pin configuration of the current system. 

 For simple devices that are restricted to the use of GPIO configuration, the set up for pins can be modified directly in the top layer sensor file by changing values in ``MAP_MIKROBUS``. 

  > The available pins for Chili2 are described in ``cascoda_chili_gpio.h`` in baremetal/platform/cascoda-nuvoton-chili2/port/source. 

 However, sensors which communicate via communication protocols such as I2C or SPI, the pin configuration can not simply be changed in the top layer.


The underlying structure of I2C and SPI interface can be found in baremetal/mikrosdk-lib and ``cascoda_sensorif.h`` in baremetal/cascoda-bm-driver. The pins that are used in I2C and SPI communication between the sensor and chili2 are internally configured in ``cascoda_sensorif_m2351.c`` and ``cascoda_sensorif_secure``. Hence, to modify interface number, change the pin configuration in both files. 

These interfaces are used in ``mikrosdk-bm`` in baremetal/app and can easily be used in other applications.

mikrosdk-click is intended to be used as a library and additional sensors produced by MikroElektronika can be added as new files. 


### A summary of the important files/folders that are related to the sernsor drivers:
| Vendor |Types      | Location of the files | Description|
| :------| :---------------- | :--------------------| :-------- | 
| MikroElektronika | Drv layer | mikrosdk-lib/drv | This layer is the interface between hardware abstraction layer and the user interface layer.|
| MikroElektronika & Cascoda | Hal layer | mikrosdk-lib/hal | This layer maps functions in MikroElektronika library to Cascoda library.
| Cascoda | GPIO | cascoda-bm-driver/cascoda_interface.h </br> platform/cascoda-nuvoton-chili2/port/source/cascoda_gpio_chili.c | Implementation of GPIO related instructions to hardware and accessibility of pins on Chili2.|
| Cascoda | I2C&SPI | cascoda-bm-driver/cascoda_sensorif.h </br> platform/cascoda-nuvoton-chili2/port/source/cascoda_sensorif_secure.c | Initiliase the communication interface between clients (slaves) and the server (master).|
| Cascoda | I2C&SPI | cascoda-bm-driver/cascoda_sensorif.h </br> platform/cascoda-nuvoton-chili2/port/source/cascoda_sensorif_m2351.c | Transmit and receive data from clients (slaves).|
---


## Step-by-step guide for modifying mikrosdk click files
<br>

This is a guide for making sensor driver files from MikroElektronika library compatible with our testing file in ``mikrosdk-bm``.

1. Add a ``<sensor>.h`` and a ``<sensor>.c`` files for the sensor from MikroElektronika library into ``mikrosdk-click`` folder.
2. In the ``<sensor>.h`` and ``<sensor>.c`` files, create an initialisation function. The codes for the initialisation function can be found in ``application_init`` in the ``main.cpp`` in MikroElektronika library. The codes and variables that are related to initialise a logger should not be added to the initialisation function.
3. Modify the `MAP_MIKROBUS` function to contain only one argument, cfg. The value that is assigned to each variable should be the module pin number.
4. <a id="declare-global-variables"></a> Global variables that are declared in ``main.cpp`` and are used in ``application_init`` should be declared in the ``<sensor>.c`` file globally. 
5. In ``main.cpp``, functions that are called and are passed in the global variables declared in ``main.cpp`` should be modified. The variables that are used as an argument for those functions should be replaced with the global variables that are declared in the ``<sensor>.c``.
6. Add the ``<sensor>.c`` file to the makefile.
7. In the ``mikrosdk_app.h`` in baremetal/app, add a flag and declare a handler function for the sensor.
8. In the ``mikrosdk_app.c`` file, define the handler function. The codes for the handler function can be found in ``application_task`` in ``main.cpp``. The ``log_printf`` function should be replaced by ``printf`` and the delay function should be removed. Any variable that has not been declared should be declared in this function.
9. Wrap the codes you just wrote in the if statement below:
```c
if (((TIME_ReadAbsoluteTime() % MIKROSDK_MEASUREMENT_PERIOD) < MIKROSDK_MEASUREMENT_DELTA) && (!handled))
{
    //Your codes should be here
}
if ((TIME_ReadAbsoluteTime() % MIKROSDK_MEASUREMENT_PERIOD) >
        (MIKROSDK_MEASUREMENT_PERIOD - MIKROSDK_MEASUREMENT_DELTA))
{
    handled = 0;
}
```
---

 **NOTE** : 
- Dependent on the I2C client device, each time before an I2C read function, a write function or a set slave address function sholud be performed.
- If a user would like to access functions additional to that of being implemented in the ``application_task`` in MikroElektronika library, the first argument of the function namely, ``ctx``, has to be replaced by the click object variable declared in the ``<sensor>.c`` file in [step 4](#declare-global-variables).





