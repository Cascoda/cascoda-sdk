# serial-adapter

Serial-adapter is a useful program for interacting with an application running on baremetal. If a baremetal device (such as a Chili2) is running a serial application (such as ot-cli), then it can be controlled by running ``serial-adapter``, with no arguments, on a posix or windows system.

The serial-adapter program, like all cascoda host programs, uses the [ca821x-posix module](../../ca821x-posix/README.md), so is compatible with both UART and USB connected Chilis.
