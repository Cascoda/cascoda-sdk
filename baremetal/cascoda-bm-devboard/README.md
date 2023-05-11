# cascoda-bm-devboard

The cascoda-bm-devboard module contains the functionality for using the [Cascoda Development Board](../../docs/how-to/howto-devboard.md). This includes interacting with the devboard LEDs and buttons, as well as interacting with externally connected peripherals, sensors and actuators.

## Examples

This module contains example applications to demonstrate the use of the APIs for interacting with the intrinsic devboard features (LEDs and buttons) and for interacting with add-on peripherals:

| Binary name | Application source code |  Description|
| --- | --- | --- |
| evboard-btn | `examples/devboard_app_btn.c` | Example app demonstrating intrinsic devboard features: LEDs and buttons. |
| devboard-click | `examples/devboard_app_click.c` | Example app demonstrating how the devboard can communicate with externally connected peripherals. |
| devboard-eink | `examples/devboard_app_eink.c` | Example app demonstrating how the devboard can show images on an eink display. Note: this only supports the 2.9 inch display. |
| devboard-gfx-x-x | `examples/devboard_app_gfx_x_x.c` | Example app demonstrating how the devboard can show graphics (menu) on an eink display (where x-x is replaced by the size, e.g. 2-9 meaning 2.9 inches). |
| devboard-sleep | `examples/devboard_app_sleep.c`  | Example app demonstrating the sleep modes. |
| devboard-batt | `examples/devboard_app_batt.c`  | Example app demonstrating the battery monitoring functions. |
## Building instructions

Assuming that you have already [set up your build directories](../../README.md#building), in order to get the devboard example binaries to build, you need to change the CMake configuration as follows: 

```CMake
CASCODA_CHILI2_CONFIG_STRING:STRING=DEV_BOARD
```
When using the Battry monitoring functions on the Development Board, the following configuration should be used:
```CMake
CASCODA_CHILI2_CONFIG_STRING:STRING=DEV_BOARD_BATT
```
The recommended method of doing this is [using the tools that come with CMake](https://cmake.org/runningcmake/). However, if you know what you are doing, then you can alternatively edit the `CMakeCache.txt` file yourself to make those changes.

After building, the example binaries will be found in `bin/devboard/` in your build directory.