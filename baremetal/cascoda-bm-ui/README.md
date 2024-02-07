# sensorif
Example UI interfaces for adding E-Paper Display and Button Functions (intrinsic and external using GPIO extender drivers).

Intended to be used as a library and additional sensors can be added as new files.

List of supported devices:

| Manufacturer      | Device        | Interface | Type | Declarations for Interface Functions |
| :---------------- | :------------ | :-------- | :--- | :------------- |
| Good Display      | IL3820        | SPI | E-Paper Display Driver | sif_il3820.h |
| Solomon Systech   | SSD1681       | SPI | E-Paper Display Driver | sif_ssd1681.h |
| Solomon Systech   | SSD1608       | SPI | E-Paper Display Driver | sif_ssd1608.h |
| Diodes/Pericom    | PI4IOE5V6408  | I2C | GPIO Extender Driver   | sif_btn_ext_pi4ioe5v6408.h |
| Diodes/Pericom    | PI4IOE5V96248 | I2C | GPIO Extender Driver   | sif_btn_ext_pi4ioe5v96248.h |

Note that the gfx library is working on the E-Paper Display only.