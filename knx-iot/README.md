# KNX IoT

This folder contains the required code for KNX IoT apps to run on our embedded Chili platform. 


The applications in this directory are compiled only if the `CASCODA_BUILD_KNX` CMake cache variable is set to `ON`.

## Example apps
The `apps` directory contains KNX IoT example applications.

All example applications contain the following shared source files:
- `knx_iot_wakeful_main.c`, as a main application.
- `knx_iot_wakeful_main_extern.h`, as an interface implemented for the application-specific code to implement, e.g. registering LEDs and setting callback functions. Its functions are called by `knx_iot_wakeful_main.c`.

### knx-iot-chilidev-pb

This application acts as a `push button` which can be used to send on/off messages to a corresponding `switch actuator`. It listens for button presses on the [Thread Development Board](../docs/how-to/howto-devboard.md), and toggles the corresponding endpoint on the `switch actuator`.

It contains these files (in addition to the shared source files listed above):
- `knx_iot_chilidev_pb.c`, implements the `knx_iot_wakeful_main_extern.h` interface - intialises the buttons as inputs, and declares that they should be polled once per 'main loop'.
- `knx_iot_virtual_pb.c`, provides a generic set of functionality for KNX IoT push button applications (created by code generation).


### knx-iot-chilidev-sa

This application acts as a `switch actuator` which receives on/off messages from a `push button` application and toggles the 4 onboard LEDs accordingly.

It contains these files (in addition to the shared source files listed above):
- `knx_iot_chilidev_sa.c`, implements the `knx_iot_wakeful_main_extern.h` interface - intialises the LEDs as outputs, and creates callbacks to set the LEDs.
- `knx_iot_virtual_sa.c`, provides a generic set of functionality for KNX IoT switch actuator applications (created by code generation).

## Requirements for development board configuration

These apps are built assuming the onboard pin jumpers are positioned in the `JUMPER_POS_2` position. This can be changed in the source code, as the pin jumper positions are specified in the `hardware_init()` function of `knx_iot_chilidev_pb.c` and `knx_iot_chilidev_sa.c`.