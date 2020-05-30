# ca821x-posix
Glue code for linking Cascoda's API code to the ca8210 Linux driver, a ca821x usb dongle, or a ca821x uart dongle.

## Kernel Driver
The kernel driver must be installed in order to use the kernel exchange. The driver can be found at https://github.com/Cascoda/ca8210-linux

In order to expose the ca8210 device node to this library the Linux debug file system must be mounted. debugfs can be mounted with the command:
```
mount -t debugfs none /sys/kernel/debug
```

## USB
In order to be able to connect to a dongle, hid-api must be installed. On debian/ubuntu, it can be installed using ```sudo apt install libhidapi-dev```. Also, in order to not require sudo, the permissions for cascoda devices should be loosened. This can be done by running the commands:
```bash
echo 'SUBSYSTEMS=="usb", ATTRS{idVendor}=="0416", ATTRS{idProduct}=="5020", ACTION=="add", MODE="0666"' | sudo tee /etc/udev/rules.d/99-cascoda.rules > /dev/null
sudo udevadm control --reload-rules && sudo udevadm trigger
```

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
