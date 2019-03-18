# sensorif
Example sensor interfaces for adding external sensor or actuator support to applications. Sensor measurement functions and actuator control are implemented in simple examples so they can be ported to other applications.<br>
This code is intended as a library in which additional sensors can be added.

List of supported devices:<br>

| Manufacturer | Device | Interface | Type | Declarations for Interface Functions |
| :----------- | :----- | :-------- | :--- | :------------- |
| Silicon Labs | Si7021 | I2C | Humidity and Temperature Sensor | sif_si7021.h |
| Maxim Integrated | MAX30205 | I2C | Human Body Temperature Sensor | sif_max30205.h |
