# mikrosdk-click

Example of MikroElectronika Click interfaces for adding external sensors or actuator support to applications. These modules abstract away the underlying I2C/SPI bus and present easy-to-use functions in C.

## List of supported Mikroelectronikca devices:

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

## List of devices:

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

 > The ``MAP_MIKROBUS`` function in all sensor header files gives the user a clear sense of the pin configuration of the current system. 

 For simple devices that are restricted to the use of GPIO configuration, the set up for pins can be modified directly in the top layer sensor file by changing values in ``MAP_MIKROBUS``. 

  > The available pins for Chili2 are described in ``cascoda_chili_gpio.h`` in baremetal/platform/chili2/port/source. 

 However, for sensors which communicate via I2C or SPI, the pin configuration can not simply be changed in the top layer. The underlying structure of I2C and SPI interface can be found in ``baremetal/mikrosdk-lib`` and ``baremetal/cascoda-bm-driver/cascoda_sensorif.h`` in baremetal/cascoda-bm-driver. The pins that are used in I2C and SPI communications between the sensor and Chili2 are internally configured in ``cascoda_sensorif_m2351.c`` and ``cascoda_sensorif_secure``. Hence, to modify the interface number, change the pin configuration in both files. 

These interfaces are used in ``baremetal/app/mikrosdk-bm/`` and can easily be used in other applications.

``mikrosdk-click`` is intended to be used as a driver library and additional sensors produced by MikroElektronika can be added as new files. 


## A summary of the important files/folders that are related to the sensor drivers:

| Vendor |Types      | Location of the files | Description|
| :------| :---------------- | :--------------------| :-------- | 
| MikroElektronika | Drv layer | mikrosdk-lib/drv | This layer is the interface between hardware abstraction layer and the user interface layer.|
| MikroElektronika & Cascoda | Hal layer | mikrosdk-lib/hal | This layer maps functions in MikroElektronika library to Cascoda library.
| Cascoda | GPIO | cascoda-bm-driver/cascoda_interface.h </br> platform/chili2/port/source/cascoda_gpio_chili.c | Implementation of GPIO-related instructions to hardware and accessibility of pins on Chili2.|
| Cascoda | I2C&SPI | cascoda-bm-driver/cascoda_sensorif.h </br> platform/chili2/port/source/cascoda_sensorif_secure.c | Initiliase the communication interface between Chili2 and external sensors/actuators.|
| Cascoda | I2C&SPI | cascoda-bm-driver/cascoda_sensorif.h </br> platform/chili2/port/source/cascoda_sensorif_m2351.c | Transmit to and receive data from external sensors/actuators.
---


## How to add support for a new MikroElectronika device

1. Create a ``<device>.h`` file in ``mikrosdk-click/include`` and a ``<device>.c`` file in ``mikrosdk-click/source`` for the new device you want to add.

2. Go to the [MikroElektronika mikrosdk click repository](https://github.com/MikroElektronika/mikrosdk_click_v2), navigate to the folder ``clicks/<device>``. Copy the contents of the file ``lib/include/<device>.h`` into the ``<device>.h`` file that you created in the previous step. Copy the contents of the file ``lib/src/<device>.c`` into the ``<device>.c`` file that you created in the previous step.


3. In the ``<device>.h`` created in step 1, add a declaration for an initialisation function adhering to the following naming scheme:
```c
uint8_t MIKROSDK_<DEVICE>_Initialise(void);
```

4. In the ``<device>.c`` created in step 1, add a function definition with an empty body for the initialisation function declared in step 3, as such:
```c
uint8_t MIKROSDK_<DEVICE>_Initialise(void)
{
    // Empty for now
}
```

5. In the [MikroElektronika mikrosdk click repository](https://github.com/MikroElektronika/mikrosdk_click_v2), navigate to the folder ``clicks/<device>/example``. Open the file ``main.c``, there you will find a function called ``application_init()``. Copy the contents of that function, and paste them into the empty body of the initialisation function definition which was added in step 4.

6. Delete all the code in that function which has anything to do with logging, because we are not using the logger.

7. In that initialisation function, you will also find a function-like macro called ``<DEVICE>_MAP_MIKROBUS()``, which has 2 arguments, one of them being the argument ``cfg``. Delete the argument that is not ``cfg``.

8. Go to the definition of the function-like macro which was mentioned in step 7. The definition will be in the file ``<device>.h``. Remove the parameter which is not ``cfg``, and modify the definition of the macro so that the members of ``cfg`` are hardcoded to the module pin numbers that you want, rather than using the values that were previously being passed via the other parameter of that macro. Here is an example of the transformation:
```c
// Before
#define <DEVICE>_MAP_MIKROBUS( cfg, mikrobus ) \
  cfg.scl  = MIKROBUS( mikrobus, MIKROBUS_SCL ); \
  cfg.sda  = MIKROBUS( mikrobus, MIKROBUS_SDA )

// After
#define <DEVICE>_MAP_MIKROBUS(cfg) \
	cfg.scl = 31;                     \
	cfg.sda = 32
```

9. In the [MikroElektronika mikrosdk click repository](https://github.com/MikroElektronika/mikrosdk_click_v2), navigate to the folder ``clicks/<device>/example``. Open the file ``main.c``. You will find there some global variables which are being used in the ``application_init()`` function. Copy those global variables into the ``<device>.c`` file.

10. In ``baremetal/app/mikrosdk-bm/include/mikrosdk_app.h``, add a flag to the list of pre-existing flags (in the form of an object-like preprocessor macro). This will look like this:
```c
/* flag which sensors to include in test */
#define MIKROSDK_TEST_<DEVICE> 0
#define MIKROSDK_TEST_AIRQUALITY4 0
#define MIKROSDK_TEST_ENVIRONMENT2 0
...
```
 
11. In ``baremetal/app/mikrosdk-bm/include/mikrosdk_app.h``, add a declaration for a handler function for the new device to the list of pre-existing handler declarations. This will look like this:
```c
/* Function handlers */
void MIKROSDK_Handler_<DEVICE>(void);
void MIKROSDK_Handler_AIRQUALITY4(void);
void MIKROSDK_Handler_ENVIRONMENT2(void);
...
```

12. In ``baremetal/app/mikrosdk-bm/source/mikrosdk_app.c``, add a function definition with an empty body for the handler function declared in step 11, and wrap it within guards that check the flag that was defined in step 10. This will look like this:
```c
#if (MIKROSDK_TEST_<DEVICE>)
void MIKROSDK_Handler_<DEVICE>(void)
{
    // Empty for now
}
#endif
```

13. In the [MikroElektronika mikrosdk click repository](https://github.com/MikroElektronika/mikrosdk_click_v2), navigate to the folder ``clicks/<device>/example``. Open the file ``main.c``, there you will find a function called ``application_task()``. Copy the contents of that function, and paste them into the empty body of the handler function definition which was added in step 12.

15. In that function, make the following modifications:
    - Replace function calls to ``log_printf()`` with function calls to ``printf()`` instead, because we are not using the logger.
    - Remove the call to the delay function ``Delay_ms()``.
    - At the top of the body of the function, declare any variables which are passed as arguments in any of the function calls, and which haven't been declared anywhere.

16. In the body of the function, notice that some of the function calls will take in as arguments one or more variables which you have added global declarations of in ``<device>.c``. Remove those arguments from the function calls, because those functions are defined in ``<device>.c`` and thus have access to the global variables defined in that file.

17. Following from step 16, in the concerned functions, remove the parameter(s) corresponding to the arguments that you removed from the function calls in the body of the handler. Do that for both the function declarations in ``<device>.h`` and definitions in ``<device>.c``. In the function definitions of those functions whose parameter(s) you have removed, use the global variable directly wherever those parameters were used before you removed them. See the example below:
```c
// Original function definition before modification
void <device>_set_baseline(<device_t> *ctx)
{
    ...
    i2c_master_write(&ctx->i2c, ...);
    ...
}

// New function definition, using global variable instead
static <device_t> <device>;
void <device>_set_baseline()
{
    ...
    i2c_master_write(&<device>->i2c, ...);
    ...
}
```

18. You are now done writing and modifying the ``<device>.h`` and ``<device>.c`` files. Add the ``<device>.c`` file as an argument to the ``add_library()`` CMake function in ``mikrosdk-click/CMakeLists.txt``.

19. Finally, there remain two more modification to make to the definition of the handler function. 
    - Surround all the code in the handler (except for the variable declarations) with the following if statement:
    ```c
    if (((TIME_ReadAbsoluteTime() % MIKROSDK_MEASUREMENT_PERIOD) < MIKROSDK_MEASUREMENT_DELTA) && (!handled))
    {
        // Handler code (minus function declarations) here...
    }
    ```
    - Add the following at the end of the handler:
    ```c
    if ((TIME_ReadAbsoluteTime() % MIKROSDK_MEASUREMENT_PERIOD) >
            (MIKROSDK_MEASUREMENT_PERIOD - MIKROSDK_MEASUREMENT_DELTA))
    {
        handled = 0;
    }
    ```
---

 **NOTE** : 
- Some I2C devices will require that a "write" or a "set slave address" be performed before an I2C "read".