# mac-dongle

mac-dongle is a simple binary that allows a Cascoda Chili platform to be used as an IEEE 802.15.4 MAC dongle. The Chili simply passes through the cascoda API commands to the CA-821x transceiver.

The application uses USB HID or UART for host communications. This is compatible with many of the 'posix' example applications
and Cascoda's Wing Commander GUI.

The 'MAC' acronym has several relevant meanings in the Cascoda SDK, so to clarify - 'MAC' in this case stands for the IEEE 802.15.4 'Message Access Control' layer.

| Meaning | Description |
| ------- | ----------- |
| __Message Access Control__ | __The Message Access Control layer of IEEE 802.15.4. Handles message security, retransmission and acknowledgement at a link layer. Implemented on the CA821x devices. This is the meaning here.__
| macOS | macOS is a supported operating system for the Cascoda SDK, and the posix applications can be built natively for macOS as described in the [build guide.](../../../README.md#building)
| Message Authentication Code | A short tag appended to a message in cryptographic systems to prove authenticity of a message. IEEE 802.15.4 refers to this as a 'Message Integrity Code' or MIC, to prevent confusion with the IEEE 802.15.4 MAC.

## IEEE 802.15.4 Test functionality
The mac-dongle application also includes an implementation of IEEE802.15.4 physical layer (PHY) test functions for:
- Transmitter testing
- Packet Error Rate (PER) test function for receiver sensitivity testing

The tests can be controlled and the results can be analysed using the Wing Commander  "Device Testing" section. The
code is structured so that it can be implemented in parallel and combined with other applications. This is possible
using the ``test15`` CMake library target.
