# test15_4-baremetal
Baremetal implementation of IEEE802.15.4 physical layer (PHY) test functions for:
- Transmitter testing
- Packet Error Rate (PER) test function for receiver sensitivity testing
- Packet sniffer

The application uses USB HID or UART for host communications to Cascoda's Wing Commander GUI for control and reporting. The tests
can be controlled and the results can be analysed using the Wing Commander "Device Testing" section.
The code is structured so that it can be implemented in parallel and combined with other applications.
