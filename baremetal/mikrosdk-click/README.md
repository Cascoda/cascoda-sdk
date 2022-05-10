# mikrosdk-click

Example of MikroElectronika Click interfaces for adding external sensors or actuator support to applications. These modules abstract away the underlying I2C/SPI bus and present easy-to-use functions in C.

### List of supported devices:

| Manufacturer      | Device               | Interface | Type | Declarations for Interface Functions | Tested |
| :---------------- | :--------------------| :-------- | :--- | :------------- | :-------------|
| MikroElektronika  | Air Quality 4 Click  | I2C | Air Quality (Gas) sensor | airquality4.h | Yes |
| MikroElektronika  | Environment2 Click   | I2C | Air Quality, Temperature, and Humidity sensor | environment2.h | No |
| MikroElektronika  | Motion Click         | GPIO | Motion sensor | motion.h| Yes |
| MikroElektronika  | Relay Click          | GPIO | Relay | relay.h | Yes |
| MikroElektronika  | Thermo Click         | SPI | Thermocouple Temperature sensor | thermo.h | Yes |
| MikroElektronika  | Thermo3 Click        | I2C | Digital Temperature sensor | thermo3.h | Yes |

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





