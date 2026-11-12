# serial-adapter

`serial-adapter` is a useful program for interacting with an application running on baremetal. If a baremetal device (such as a Chili2) is running a serial application (such as ot-cli), then it can be controlled by running ``serial-adapter`` on a POSIX or Windows system.

The `serial-adapter` program, like all Cascoda host programs, uses the [ca821x-posix module](../../ca821x-posix/README.md), so is compatible with both UART and USB connected Chilis. NOTE: For the best reliability, connect the device directly to your PC instead of to a USB hub.

To see how to use this program with command line arguments, run it with the -h flag, which will print the help page below.

```
===== How to use the command =====
serial-adapter.exe      Serial-adapter program for interacting with an application running on baremetal.

        -s [SERIALNO]       This application will try find the device with the serial number specified.
                            If not specified, the first device detected will be used.
                            For UART, it is also possible to specify a COM port number using this argument

        -e, --exit-on-drop  (USB only) When device drops connection, exit the program immediately, without attempting to reload.
```
