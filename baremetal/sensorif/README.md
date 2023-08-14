# sensorif
Example sensor interfaces for adding external sensor or actuator support to applications. These modules abstract away the underlying I2C/SPI bus and present easy-to-use functions in C.

These interfaces are used in some of the example applications in baremetal/app, such as ``sensorif-bm`` and ``ot-sed-sensorif`` and can be easily used in other applications.

sensorif is intended to be used as a library and additional sensors can be added as new files.

List of supported devices:

| Manufacturer      | Device        | Interface | Type | Declarations for Interface Functions |
| :---------------- | :------------ | :-------- | :--- | :------------- |
| Silicon Labs      | Si7021        | I2C | Humidity and Temperature Sensor | sif_si7021.h |
| Maxim Integrated  | MAX30205      | I2C | Human Body Temperature Sensor | sif_max30205.h |
| LITEON            | LTR-303ALS-01 | I2C | Ambient Light Sensor | sif_ltr303als.h         | 
| Good Display      | IL3820        | SPI | E-Paper Display Driver | sif_il3820.h |
| Solomon Systech   | SSD1681       | SPI | E-Paper Display Driver | sif_ssd1681.h |
| Solomon Systech   | SSD1608       | SPI | E-Paper Display Driver | sif_ssd1608.h |

Note that the gfx library is working on the E-Paper Display only.