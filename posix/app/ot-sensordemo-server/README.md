# ot-sensordemo-server

This example hosts a Thread stack and application on a posix system, using a connected ``mac-dongle`` Chili for IEEE 802.15.4 communication. The application can communicate over Thread and receive sensor readings using the Cascoda sensordemo application layer. This includes the baremetal examples ``ot-sed-sensorif``, ``ot-sed-thermometer`` & ``ot-sed-thermometer-freertos``.

Requires a ``mac-dongle`` Chili device to be connected to the host via UART/USB.

The demo presents an OpenThread CLI, which can be used to form a Thread network and [commission end devices](../../../docs/dev/thread-commissioning.md). Sensor information from connected devices will be printed to the CLI.
