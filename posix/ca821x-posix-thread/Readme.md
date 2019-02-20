# ca821x-posix-thread

A posix platform layer for openthread, and some examples for how to use it on Linux.

In the examples folder is an example program which presents a command line interface to test the thread interface. For more documentation on how to use this, and also information on the openthread API, see the main openthread repo:
<https://github.com/Cascoda/openthread>

The CLI Documentation can be found here:
<https://github.com/Cascoda/openthread/tree/master/src/cli>

## Using wpantund to enable as linux network interface

On a posix system, a thread node can act as a linux network interface using the wpantund tool available from https://github.com/openthread/wpantund/

In order to install, follow the guide here (use the latest master commit, not full/latest-release): https://github.com/openthread/wpantund/blob/master/INSTALL.md#wpantund-installation-guide

then start wpantund using a command of the form:
```bash
sudo /usr/local/sbin/wpantund -o Config:NCP:SocketPath "system:/home/pi/ca8210-posix-thread/example/ncpapp" -o SyslogMask " -info" -o Config:TUN:InterfaceName utun6
```

where /home/pi/ca8210-posix-thread/example/ncpapp is a path to a thread application using the ncp library (call otNcpInit and include the NCP library in the application)

Then follow a similar process as used in this tutorial to start the control panel and connect: https://github.com/openthread/wpantund/wiki/OpenThread-Simulator-Tutorial

