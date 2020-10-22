# Test applications for Posix

## serial-test
serial-test is a simple program which tests connectivity to attached Cascoda Chili devices. It can be used for checking comms interfaces and stress testing them. It can also be used for displaying the debug log of connected Chili devices without stressing the interface. Run ``serial-test`` with no arguments to print the help.

## evbme-get
evbme-get is a simple program which will connect to an attached Cascoda Chili device, and print out all of the available EVBME attributes. It can be useful for identifying a device and its application.

## stress-test
stress-test is a simple program which generates a lot of IEEE 802.15.4 traffic between devices for stress testing.

The example takes a series of device IDs as arguments. These should be unique, and are used as the short addresses. At least 2 devices should be used at a time with this program.

```bash
cd example
#higher privileges may be needed if your user does not have permission to access devices
./stress-test 1 2 3
```