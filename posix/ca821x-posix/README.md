# ca821x-posix
Glue code for linking Cascoda's API code to the ca8210 Linux driver or a ca821x usb dongle

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
Serial-adapter is a useful program for interacting with an application running on baremetal. If a baremetal device (such as a Chili2) is running a serial application (such as cliapp-bm), then it can be controlled by running ``serial-adapter``, with no arguments, on a compatible platform.

# spam_test
A spam_test example is included. Make sure that at least one of the Hid library or ca821x kernel driver is installed as detailed below. The example programs will be built alongside the library.

The example takes a series of device IDs as arguments. These should be unique, and are used as the short addresses. At least 2 devices should be used at a time with this program.
```bash
cd example
#higher privileges may be needed if your user does not have permission to access devices
./spam_test 1 2 3
```

## Kernel Driver
The kernel driver must be installed in order to use the kernel exchange. The driver can be found at https://github.com/Cascoda/ca8210-linux

In order to expose the ca8210 device node to this library the Linux debug file system must be mounted. debugfs can be mounted with the command:
```
mount -t debugfs none /sys/kernel/debug
```

## USB
In order to be able to connect to a dongle, hid-api must be installed. Follow the documentation in usb-exchange/hidapi to install as a shared library. On debian/ubuntu, it can be installed using ```sudo apt install libhidapi-dev``` instead.

## UART
UART can be used for any posix serial ports. In order to use UART, the environment variable ``CASCODA_UART`` must be set up with a list of available UART ports that are connected to a supported cascoda module. The environment variable should consist of a list of colon separated values, each containing a path to the UART device file and the baud rate to be used.
eg: ``CASCODA_UART=/dev/ttyS0,115200:/dev/ttyS1,9600:/dev/ttyS2,4000000``

For example, on the raspberry pi 3 running raspbian, you can setup the uart as follows (this overrides the uart terminal):
```bash
# First prevent the uart for being used for a linux terminal, and enable it
sudo sed -i 's/console=serial0,115200 //g' /boot/cmdline.txt
echo "enable_uart=1" | sudo tee -a /boot/config.txt
# reboot so changes take effect
sudo reboot
# Then add environment variable 
# (warning, will not persist reboots unless you add to a startup script)
export CASCODA_UART=/dev/serial0,1000000
