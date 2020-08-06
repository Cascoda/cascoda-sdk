# mac-dongle

mac-dongle is a simple binary that allows a Cascoda Chili platform to be used as an IEEE 802.15.4 MAC dongle. The Chili simply passes through the cascoda API commands to the CA-821x transceiver.

The application uses USB HID or UART for host communications. This is compatible with many of the 'posix' example applications
and Cascoda's Wing Commander GUI.

## IEEE 802.15.4 Test functionality
The mac-dongle application also includes an implementation of IEEE802.15.4 physical layer (PHY) test functions for:
- Transmitter testing
- Packet Error Rate (PER) test function for receiver sensitivity testing

The tests can be controlled and the results can be analysed using the Wing Commander  "Device Testing" section. The
code is structured so that it can be implemented in parallel and combined with other applications. This is possible
using the ``test15`` CMake library target.