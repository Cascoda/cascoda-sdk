# FreeRTOS Demo #

This folder contains a brief demonstration of FreeRTOS on the M2351, showcasing the capabilities of the FreeRTOS MPU abstraction, and of TrustZone. It is based on the [MPU M23 Nuvoton NuMaker demo](https://www.freertos.org/RTOS-Cortex-M23-NuMaker-PFM-M2351-Keil.html) that is shipped with FreeRTOS. However, it has been ported in order to work with the GCC toolchain.

## How to build ##
```bash
make freertos-secure-demo
make freertos-nonsecure-demo
```
## How to flash ##
You must flash both the secure binary and the nonsecure one. You may encounter difficulties whilst doing so, depending on the tools you use, since flashing two binaries is unusual and the nonsecure .text lies in the M2351's nonsecure address space, which is offset by `0x1000 0000`.

If you are using GDB, you may flash the secure firmware as expected:
```gdb
(gdb) load bin/freertos-secure-demo
```
The nonsecure firmware requires a negative offset in order to point to the correct part of the flash.
```gdb
(gdb) load bin/freertos-nonsecure-demo -0x10000000
```

If you want to flash the binaries with a different tool, ensure that the .text section of the nonsecure firmware starts at `0x10000`. You must do this because the first 64kB of Flash are reserved for the secure firmware.

## GDB One-Liner ##

```bash 
arm-none-eabi-gdb -ex "target remote $GDB_SERVER_IP" -ex "file bin/freertos-secure-demo" -ex "load bin/freertos-nonsecure-demo -0x10000000" -ex "add-symbol-file bin/freertos-nonsecure-demo" -ex "load" -ex "monitor reset"
```

The above one-liner automates the flashing and testing of the demonstration. It connects to the GDB server, flashes the latest secure and nonsecure firmware and resets the chip, which is now ready to debug. This script must be run from the Cascoda build directory.

Before running the script, you must set the `GDB_SERVER_IP` environment variable to the IP and port of your J-Link GDB server, e.g. `$ GDB_SERVER_IP=192.168.10.212:2331`. Alternatively, you can modify the one-liner by replacing `$GDB_SERVER_IP` with the server's IP and port.

Once you run the command and accept the prompts, the demo is ready to be tested with GDB.
## How to run & test

To verify that FreeRTOS is working, set breakpoints at the tasks that the demonstration is running, namely `prvROAccessTask`, `prvRWAccessTask` and `prvSecureCallingTask`. These functions should all periodically be called by the RTOS.

Because this demo only tests the functionality of FreeRTOS, without interacting with the platform it is running on, there is no external behaviour that can be observed without a debugger.