# Sleepy End Device EPaper demo #

This target contains an application which requests compressed images from a
server (`ot-eink-server`) and displays them onto a Waveshare 2.9" e-Paper
module. See [the server
documentation](../../../posix/app/ot-eink-server/README.md) for a high
level overview of the application as a whole, and for a description of how to
generate images compatible with this application.

## How to run & test

This demonstration requires another Chili to act as the server. Flash the
server Chili with the `mac-dongle` binary. You must also build the
`ot-eink-server` POSIX application, from within the Cascoda POSIX SDK
build directory.

Once the devices are all ready, plug the server Chili into a Linux machine
and run the `ot-eink-server` executable. Then, power the end device
Chili and [commission it](../../../docs/guides/thread-commissioning.md). After 10 to 20 seconds, the server will receive discover requests, as
well as image requests from the end device.
