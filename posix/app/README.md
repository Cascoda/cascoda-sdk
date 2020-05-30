# Example applications for Posix
Several examples will also build for Windows using MinGW.

# Sniffer
An example program for sniffing 802.15.4 traffic on a specific channel is included. It can run on windows or linux and can:
- Capture to stdout
- Output to a pcap file
- Stream directly to wireshark/tshark

Run ./sniffer with no args to print the help page. For instance the ``-w`` argument can be used to automatically boot wireshark and connect. On unix platforms the usage is more flexible and can use command line pipes such as:
```bash
# Start sniffer on channel 21, pcap output, pipe to tshark, which is receiving on stdin.
./sniffer 21 -p | tshark -i -
```

# serial-adapter
Serial-adapter is a useful program for interacting with an application running on baremetal. If a baremetal device (such as a Chili2) is running a serial application (such as ot-cli), then it can be controlled by running ``serial-adapter``, with no arguments, on a compatible platform.

# serial-test
serial-test is a simple program which tests connectivity to attached Cascoda Chili devices. It can be used for checking comms interfaces and stress testing them. It can also be used for displaying the debug log of connected Chili devices without stressing the interface. Run ``serial-test`` with no arguments to print the help.

# evbme-get
evbme-get is a simple program which will connect to an attached Cascoda Chili device, and print out all of the available EVBME attributes. It can be useful for identifying a device and its application.

# stress-test
A stress-test example is included. Make sure that at least one of the Hid library or ca821x kernel driver is installed as detailed below. The example programs will be built alongside the library.

The example takes a series of device IDs as arguments. These should be unique, and are used as the short addresses. At least 2 devices should be used at a time with this program.
```bash
cd example
#higher privileges may be needed if your user does not have permission to access devices
./stress-test 1 2 3
```