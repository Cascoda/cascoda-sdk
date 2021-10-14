# ca821x-posix-thread

A POSIX platform layer for OpenThread, and some examples for how to use it on Linux.

In the examples folder is an example program which presents a command line interface to test the Thread interface. For more documentation on how to use this, and also information on the openthread API, see the main openthread repo:
<https://github.com/Cascoda/openthread>

## ot-cli-posix(-ftd/-mtd)

An example application running the OpenThread stack and exposing the OpenThread CLI on stdin.
This application runs the entire OpenThread stack on the host platform, and uses a connected Chili
running [mac-dongle](../../baremetal/app/mac-dongle) for its radio operations.

The ftd/mtd suffix on these binaries refer to the "Full Thread Device" and "Minimal Thread Device" versions of this application
respectively.

The CLI Documentation can be found here:
<https://github.com/Cascoda/openthread/tree/master/src/cli>

## ot-ncp-posix

An example application running the OpenThread stack in NCP (Network Co-Processor) mode.

Note that the terminology here is a little confusing, but originates from the terms that OpenThread
itself uses. The NCP application and full OpenThread stack are **running locally on the host system**.
This means that the NCP application can take advantage of the resources of the host system.

The NCP application uses a connected Chili running [mac-dongle](../../baremetal/app/mac-dongle) for its radio operations.

If an NCP that is actually running as a coprocessor is required, the baremetal [ot-ncp](../../baremetal/cascoda-bm-thread/example)
can be used instead.

## Using wpantund to enable as linux network interface

On a posix system, a thread node can act as a linux network interface using the wpantund tool available from https://github.com/openthread/wpantund/

In order to install, follow the guide here (use the latest master commit, not full/latest-release): https://github.com/openthread/wpantund/blob/master/INSTALL.md#wpantund-installation-guide

then start wpantund using a command of the form:
```bash
sudo /usr/local/sbin/wpantund -o Config:NCP:SocketPath "system:/home/pi/ca8210-posix-thread/example/ot-ncp-posix" -o SyslogMask " -info" -o Config:TUN:InterfaceName utun6
```

where /home/pi/ca8210-posix-thread/example/ot-ncp-posix is a path to a thread application using the ncp library (call otNcpInit and include the NCP library in the application)

Then follow a similar process as used in this tutorial to start the control panel and connect: https://github.com/openthread/wpantund/wiki/OpenThread-Simulator-Tutorial

