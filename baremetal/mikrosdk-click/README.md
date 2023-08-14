# mikrosdk-click

MikroE Click board<sup>TM</sup> driver interfaces for adding external sensors or actuator support to applications. These modules abstract away the underlying I2C/SPI bus and present easy-to-use functions in C. The drivers in this folder are adapted from the [MikroE GitHub repository](https://github.com/MikroElektronika/mikrosdk_click_v2), with functions such as alarm support and low power operation added.
The drivers in mikrosdk-click interface directly with the mikrosdk-lib driver layer (drv) and the hardware abstraction layer (hal). When implementing additional devices the code in mikrosdk-lib should not be modified.

## List of supported Click Boards:

Click Board          | Interface | Supply | Type | Declarations for Interface Functions |
:--------------------| :-------- | :----- | :--- |:-------------|
[Thermo Click](https://www.mikroe.com/thermo-click)         | SPI | 3.3V | Thermocouple Temperature Sensor |  thermo_click.h |
[Thermo 3 Click](https://www.mikroe.com/thermo-3-click)        | I2C | 3.3V/5V | Digital Temperature Sensor | thermo3_click.h |
[Air Quality 4 Click](https://www.mikroe.com/air-quality-4-click)  | I2C | 3.3V/5V | Air Quality (Gas) Sensor | airquality4_click.h  |
[Environment2 Click](https://www.mikroe.com/environment-2-click)   | I2C | 3.3V/5V | Air Quality, Temperature, and Humidity Sensor | environment2_click.h |
[SHT Click](https://www.mikroe.com/sht-click)    | I2C | 3.3V/5V | Temperature and Humidity Sensor | sht_click.h |
[HVAC Click](https://www.mikroe.com/hvac-click)    | I2C | 3.3V/5V<sup>1)</sup> | Particulate Matter, Air Quality Sensor | hvac_click.h |
[Motion Click](https://www.mikroe.com/motion-click)         | GPIO | 3.3V<sup>2)</sup> | Motion Sensor | motion_click.h |
[Relay Click](https://www.mikroe.com/relay-click)    | GPIO | 5V | Relay Actuator | relay_click.h |
[Ambient 8 Click](https://www.mikroe.com/ambient-8-click)    | I2C | 3.3V | Illuminance Sensor | ambient8_click.h |
[Fan Click](https://www.mikroe.com/fan-click)    | I2C | 3.3V+5V | Fan Control Actuator | fan_click.h |
[Buzz 2 Click](https://www.mikroe.com/buzz-2-click)    | PWM | 3.3V/5V | Buzzer | buzz2_click.h |
[LED Driver 3 Click](https://www.mikroe.com/led-driver-3-click)    | I2C | 3.3V+5V | RGB LED | led3_click.h |

For boards marked as 3.3V/5V the supply voltage can be selected by a solder jumper on the Click Board.<br>
<sup>1)</sup>: It is recommended to supply the Click Board with 5V due to sensitivity to supply noise at 3.3V.<br>
<sup>2)</sup>: A clean 3.3V supply is recommended for the Click Board due to sensitivity to supply noise.<br>
If any supply noise issues persist, it is recommended to add additional decoupling capacitance on the 3.3V supply close to the Click Board.<br>

## Sensor / Actuator Devices supported:

Click Board          | Sensor Manufacturer | Sensor Device | Variables measured / Actuator Type | Alarm Function |
:--------------------| :-------- |:-------------|:-------------|:---|
Thermo Click         | Maxim Integrated | MAX31855 | Thermocouple Temperature, Junction Temperature | No |
Thermo 3 Click       | Texas Instruments | TMP102 | Temperature | Yes |
Air Quality 4 Click  | Sensirion | SGP30 | CO2, TVOC | No |
Environment2 Click   | Sensirion | SHT40, SGP40 | Humidity, Temperature, VOC Index | No |
SHT Click            | Sensirion | SHT3x | Humidity, Temperature | Yes |
HVAC Click           | Sensirion | SCD41 | CO2, Humidity, Temperature | No |
Motion Click         | Silvan Chip Electronics | BISS0001 | Motion | Yes | 
Relay Click          | Omron | G6D | Dual Relay Actuator | No |
Ambient 8 Click      | Liteon | LTR-329ALS | Illuminance (Visible, IR, Ambient) | No |
Fan Click            | Microchip | EMC2301 | 5V 4-Wire Fan Control Actuator (Open Loop, Closed Loop) | Yes |
Buzz 2 Click         | CUI Devices | CMT-8540S-SMT | Magnetic Buzzer Transducer | No |
LED 3 Driver Click   | ON Semiconductor | NCP5623B | RGB LED Actuator | No |


## Functions required for Interfacing with an Application:

All driver implementations contain the following functions required to interface with an application:
```c
uint8_t MIKROSDK_<DEVICE>_Initialise(void);
```
Initialisation function which should be called at startup. Initialises the Click board and returns status.
```c
uint8_t MIKROSDK_<DEVICE>_Acquire(params); (Sensors)
uint8_t MIKROSDK_<DEVICE>_Driver(params); (Actuators)
```
Sensor data acqusition function / Actuator Driver function which reads the sensor values or controls the actuator. Returns status.
```c
uint8_t MIKROSDK_<DEVICE>_alarm_triggered(void);
```
This function is implemented for sensors supporting alarms and can by polled or used in interrupts to determine if an alarm has been raised.
```c
void MIKROSDK_<DEVICE>_pin_mapping(params);
```
For CLICK boards using GPIO pins, this function maps the GPIO pins to the specific mikroBUS<sup>TM</sup> signals and should be called before the initialisation function described above. For Chili2D/Chili2S devices the module pin numbers can be taken from the corresponding datasheet.

For sensors which communicate via I2C or SPI, the functions SENSORIF_I2C_Config(u32_t portnum) or SENSORIF_SPI_Config(u32_t portnum) have to be called before initialisation to configure the I2C or SPI port number. 

Other functions implemented in the drivers are device specific. All declarations can be found in ```<DEVICE>_click.h```

For examples of how to use the drivers in an application please refer to the Click example (devboard_app_click.c) for the Cascoda development board in [baremetal/cascoda-bm-devboard/examples](../cascoda-bm-devboard/examples)

## How to add Support for a new Click Board:

It is strongly recommended to get familiar with an existing implementation (i.e. the Thermo3 click). The code from the [MikroE GitHub repository](https://github.com/MikroElektronika/mikrosdk_click_v2) can be used as starting point.

The new driver should contain two separate include files:

```<DEVICE>_drv.h``` for interfacing with the lower layers (mikrosdk-lib/drv, mikrosdk-lib/hal)

```<DEVICE>_click.h``` for interfacing with the application.
