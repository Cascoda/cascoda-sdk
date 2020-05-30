# LwIP FreeRTOS demo #

This demonstration shows LwIP capabilities in combination with freeRTOS. This enables the posix-style Socket API, which means that existing posix applications can be easily ported to run on the Chili2 and Thread. This demo is compatible with ot-cli-lwip.

This demo provides the standard openthread CLI as documented here: https://github.com/Cascoda/openthread/tree/ext-mac-dev/src/cli

The additional 'lwip' command can be used to open and send data over TCP connections:

```
> lwip
> lwip - print help
lwip tcp - print status
lwip tcp con <ip> - open tcp connection to port 51700
lwip tcp sen <msg> - send text string over tcp connection
lwip tcp clo - close tcp connection
lwip dns <hostname> - Resolve IP address of hostname using DNS
lwip dns ser <ipv6 addr> - Set the IPv6 address of the DNS server
```

The demo is listening on port 51700 for incoming connections, and will print any data received on this port as text.

## How to build ##
```bash
make ot-cli-lwip-freertos
```

## How to flash ##
You must flash both the secure binary and the nonsecure one. You may encounter difficulties whilst doing so, depending on the tools you use, since flashing two binaries is unusual and the nonsecure .text lies in the M2351's nonsecure address space, which is offset by `0x1000 0000`. Note: Some oddities have been observed when flashing with J-Link, so if behaviour is abnormal, it may be worth trying the Nu-Link.

If you are using GDB, you may flash the secure firmware as expected:
```gdb
(gdb) load bin/chili2-default-secure
```
The nonsecure firmware requires a negative offset in order to point to the correct part of the flash.
```gdb
(gdb) load bin/ot-cli-lwip-freertos -0x10000000
```

If you are using the Nu-Link and [the Nuvoton ICP Programming
Tool](https://www.nuvoton.com/tool-and-software/software-development-tool/programmer/) then the APROM file should be ``chili2-default-secure.bin``, the APROM_NS file should be ``ot-cli-lwip-freertos.bin`` and the chip settings should place the Non-Secure region base address at 0x10000.

If you want to flash the binaries with a different tool, ensure that the .text section of the nonsecure firmware starts at `0x10000`. You must do this because the first 64kB of Flash are reserved for the secure firmware.

## GDB One-Liner ##

```bash
arm-none-eabi-gdb -ex "target remote $GDB_SERVER_IP" -ex "file bin/chili2-default-secure" -ex "load bin/ot-cli-lwip-freertos -0x10000000" -ex "add-symbol-file bin/ot-cli-lwip-freertos" -ex "load" -ex "monitor reset"
```

The above one-liner automates the flashing and testing of the demonstration. It connects to the GDB server, flashes the latest secure and nonsecure firmware and resets the chip, which is now ready to debug. This command must be run from the Cascoda build directory.

Before running the script, you must set the `GDB_SERVER_IP` environment variable to the IP and port of your J-Link GDB server, e.g. `$ GDB_SERVER_IP=192.168.10.212:2331`. Alternatively, you can modify the one-liner by replacing `$GDB_SERVER_IP` with the server's IP and port.

Once you run the command and accept the prompts, the demo is ready to be tested.
