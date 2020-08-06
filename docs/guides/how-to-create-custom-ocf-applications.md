# How to Create Custom OCF Applications #

The IoTivity-Lite development kit enables users to automatically generate
code for an OCF Server. This is a great starting point for developing an OCF
application. This guide describes how to generate such applications and
import them into the Cascoda SDK.

## Prerequisites ##

In order to follow this guide, you must set up the IoTivity-Lite SDK as
described in the [IoTivity-Lite Device Simulation
Guide.](https://iotivity.org/getting-started-iotivity-device-simulation) We
recommend going through the entire guide so that you can gain an
understanding of how OCF applications work in general. If you have the
necessary hardware, we also recommend going through the [IoTivity-Lite
Raspberry Pi
Guide](https://iotivity.org/getting-started-iotivity-raspberry-pi-kit) before
proceeding.

## Generating an Application ##

After running the `./gen.sh` script, as described in the Device Simulation
guide, you should find the auto-generated source files inside the
`device_output/code` folder. The files that need to be imported into the
Cascoda SDK are `server_introspection.dat.h`, and `simpleserver.c`

`server_introspection.dat.h` contains introspection data used by an OCF
server in order to advertise its capabilities. It describes the various
resources present on the server, and gives information on how to use them.
Having accurate introspection data is necessary in order to pass OCF
certification. The introspection data is generated based on the
`example.json` file in the top-level `iot-lite` directory. It is stored as
CBOR data, and can be translated to a human-readable format using
[cbor.me](http://cbor.me/).

`simpleserver.c` contains the actual behaviour of the server, which enables
it to respond to requests & updates. When developing an end product, the
source file needs to be modified to communicate with the hardware (such as a
relay to turn on a smart light, or various sensors within a sensing device).
The comments near the top of the file contain information on how to do this.

You can find more information about the contents of these files within [OCF's
documentation of the template used to generate
them.](https://openconnectivityfoundation.github.io/swagger2x/src/templates/IOTivity-lite/)
Additionally, more information on the IoTivity-Lite SDK [can be found
here](https://openconnectivity.github.io/IOTivity-Lite-setup/).

## Importing an Application into the Cascoda SDK ##

Once you have generated your application, you must move `simpleserver.c` to
the `ocf/apps` directory, and rename it to something more descriptive of your
application. You must also create an include directory that corresponds to
the application, and move `server_introspection.dat.h` into that directory.

You must edit `ocf/apps/CMakeLists.txt` in order to make the build system
aware of the new source files. Follow the example laid out by the other
applications, such as `ocf-cli-thermometer`.

You must also modify the `.c` so that it can work as an embedded
OCF-over-Thread device. Firstly, you must modify the `main` function so that
it initializes Thread and the Cascoda SDK. For most applications, you should
be able copy the `main` function used in `example_thermometer.c`. Secondly,
you must modify the handlers for the OCF methods so that they can communicate
to sensors or actuators connected to the Chili.

## Customizing an Application using DeviceBuilder ##

By default, the IoTivity SDK generates an OCF Server intended to control a
light, with a `oic.r.switch.binary` resource. If you would like to generate a
different type of device, you must change the device type inside `gen.sh`, as
well as the list of resources within `example.json`. The list of resources
must match the device type of the server in order to pass OCF certification.
A list of the mandatory resources for each standard OCF device type can be
found within the [OCF Device
Specification](https://openconnectivity.org/developer/specifications/).
