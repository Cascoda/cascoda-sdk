# OpenThread Baremetal Example

This directory contains the simple source for two example applications:

- ot-cli, a command line application that can be controlled by running [serial adapter](../../../posix/app/serial-adapter) on a host platform.
- ot-ncp, a special application that acts as an openthread 'Network Coprocessor', that communicates using the 'spinel' protocol to software running on a Linux system.

## ot-cli

This application presents the [OpenThread CLI](https://github.com/Cascoda/openthread/blob/ext-mac-dev/src/cli/README.md)
over its serial interface in a way that can be accessed by the [serial adapter](../../../posix/app/serial-adapter) host
program.

In addition to this, the ot-cli application has some extra commands implemented for demonstration and testing purposes.

| Command | Description 
| ------- | -----------
| sensordemo [sensor/server/stop] | Start or stop the simple sensordemo, as a sensor, transmitting sensor readings, or a server, which receives them. Simple example coap & cbor example.
| autostart [enable/disable]      | Enable or disable the autostart feature, which will cause the device to automatically boot up and attach to a thread network when powered - useful for using without the CLI.
| join [info]                     | Get the joining credentials (if info is specified). Otherwise, start the autojoining procedure, as specified [in the commissioning document.](../../../docs/dev/thread-commissioning.md)
| dnsutil <host.to.resolve>       | Test DNS resolution of a hostname.
