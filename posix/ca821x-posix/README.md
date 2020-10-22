# ca821x-posix

The ca821x-posix module contains the posix-specific functionality for the Cascoda SDK. It also includes partial windows support when built with MinGW. It enables communication with a CA-821x platform from a host, and also communication to embedded Cascoda platforms such as the Chili2.

ca821x-posix includes two major components:

- [Exchange](#exchange)
- [API](#api)

## Exchange

The exchange handles communication between the host and the CA-821x/Chili platform. There are several implemented exchanges for different interfaces. The actual exchange used is transparent to the application. If the ``ca821x_util_init`` function is used to initialise, a suitable interface is chosen automatically at runtime. It is therefore possible to write an application using a USB Chili2D device, and later move to a UART Chili2S without changing any C code.

The exchange used is selected at runtime, with the priority Kernel > UART > USB. The USB Exchange can be used automatically as long as there is a USB Chili plugged in that is not already in use, and the current user has the permissions to access it. The UART and Kernel exchanges require further configuration as detailed below.

### Kernel Driver Exchange
The kernel driver must be installed in order to use the kernel exchange. The driver can be found at https://github.com/Cascoda/ca8210-linux

In order to expose the ca8210 device node to this library the Linux debug file system must be mounted. debugfs can be mounted with the command:

```
mount -t debugfs none /sys/kernel/debug
```

Warning: The kernel exchange only works when the ca821x driver is installed on a compatible Linux platform.

### UART Exchange
UART can be used for any posix serial ports. In order to use UART, the environment variable ``CASCODA_UART`` must be configured with a list of available UART ports that are connected to a supported Cascoda module. The environment variable should consist of a list of colon separated values, each containing a path to the UART device file and the baud rate to be used.
eg: ``CASCODA_UART=/dev/ttyS0,115200:/dev/ttyS1,9600:/dev/ttyS2,4000000``

For example, on the Raspberry Pi 3 running Raspberry Pi OS, you can set up the UART as follows (this overrides the UART terminal):

```bash
# Prevent the UART being used as a Linux terminal, and enable it
sudo sed -i 's/console=serial0,115200 //g' /boot/cmdline.txt
echo "enable_uart=1" | sudo tee -a /boot/config.txt
# Reboot so changes take effect
sudo reboot
# Then add environment variable 
# (warning, will not persist reboots unless you add to a startup script)
export CASCODA_UART=/dev/serial0,1000000
```

Warning: UART has not currently been implemented for windows.

### USB Exchange
In order to be able to connect to a dongle, hid-api must be installed. On debian/ubuntu, it can be installed using ```sudo apt install libhidapi-dev```. Also, in order to not require sudo, the permissions for cascoda devices should be loosened. This can be done by running the commands:

```bash
echo 'SUBSYSTEMS=="usb", ATTRS{idVendor}=="0416", ATTRS{idProduct}=="5020", ACTION=="add", MODE="0666"' | sudo tee /etc/udev/rules.d/99-cascoda.rules > /dev/null
sudo udevadm control --reload-rules && sudo udevadm trigger
```

## API

The API of the ca821x-posix module is fairly minimal, as it exists mainly to enable the ``ca821x-api`` and ``cascoda-utils`` modules. It includes functionality to initialise and control the interfaces with Cascoda devices in ``ca821x-posix.h``. It also provides API functions to communicate with the EVBME of the connected Chili platform - defined in the ``ca821x-posix-evbme`` header.
