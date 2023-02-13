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
