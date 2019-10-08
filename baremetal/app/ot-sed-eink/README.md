# Sleepy End Device EInk demo #

This target contains an application which requests compressed images from a
server (`ot-server-eink`) and displays them onto a Waveshare 2.9" e-Paper
module. See [the server
documentation](../../../posix/ca821x-posix-thread/example/Readme-eink.md) for a high
level overview of the application as a whole, and for a description of how to
generate images compatible with this application.

## How to build ##
```bash make sed-eink ``` Note that the nonsecure binary (`sed-eink`)
has a dependency upon the secure binary (`freertos-default-secure`).
Therefore, building the non-secure binary also builds the secure one as a
side effect.

## How to flash ##
You must flash both the secure binary and the nonsecure one. You may
encounter difficulties whilst doing so, depending on the tools you use, since
flashing two binaries is unusual and the nonsecure .text lies in the M2351's
nonsecure address space, which is offset by `0x1000 0000`.

If you are using GDB, you may flash the secure firmware as expected:
```gdb
(gdb) load bin/freertos-default-secure
```
The nonsecure firmware requires a negative offset in order to point to the correct part of the flash.
```gdb
(gdb) load bin/sed-eink -0x10000000
```

If you want to flash the binaries with a different tool, ensure that the
.text section of the nonsecure firmware starts at `0x10000`. You must do this
because the first 64kB of Flash are reserved for the secure firmware.

## GDB One-Liner ##

```bash
arm-none-eabi-gdb -ex "target remote $GDB_SERVER_IP" -ex "file bin/freertos-default-secure" -ex "load bin/sed-eink -0x10000000" -ex "add-symbol-file bin/sed-eink" -ex "load" -ex "monitor reset"
```

The above one-liner automates the flashing and testing of the demonstration.
It connects to the GDB server, flashes the latest secure and nonsecure
firmware and resets the chip, which is now ready to debug. This script must
be run from the Cascoda build directory.

Before running the script, you must set the `GDB_SERVER_IP` environment
variable to the IP and port of your J-Link GDB server, e.g. `$
GDB_SERVER_IP=192.168.10.212:2331`. Alternatively, you can modify the
one-liner by replacing `$GDB_SERVER_IP` with the server's IP and port.

Once you run the command and accept the prompts, the demo is ready to be tested.
## How to run & test

This demonstration requires another Chili to act as the server. Flash the
server Chili with the `test15-4-app` binary. You must also build the
`ot-server-eink` POSIX application, from within the Cascoda POSIX SDK
build directory.

Once the devices are all ready, plug the server Chili into a Linux machine
and run the `ot-server-eink` executable. Then, power the end device
Chili. After 10 to 20 seconds, the server will receive discover requests, as
well as image requests from the end device.
