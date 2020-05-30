# Sleepy End Device FreeRTOS demo #

This demonstration shows the OpenThread sleepy end device capabilities, and how FreeRTOS can enhance them.

## How to build ##
```bash
make ot-sed-thermometer-freertos
```

## How to flash ##
You must flash both the secure binary and the nonsecure one. You may encounter difficulties whilst doing so, depending on the tools you use, since flashing two binaries is unusual and the nonsecure .text lies in the M2351's nonsecure address space, which is offset by `0x1000 0000`.

If you are using GDB, you may flash the secure firmware as expected:
```gdb
(gdb) load bin/chili2-default-secure
```
The nonsecure firmware requires a negative offset in order to point to the correct part of the flash.
```gdb
(gdb) load bin/ot-sed-thermometer-freertos -0x10000000
```

If you want to flash the binaries with a different tool, ensure that the .text section of the nonsecure firmware starts at `0x10000`. You must do this because the first 64kB of Flash are reserved for the secure firmware.

## GDB One-Liner ##

```bash
arm-none-eabi-gdb -ex "target remote $GDB_SERVER_IP" -ex "file bin/chili2-default-secure" -ex "load bin/ot-sed-thermometer-freertos -0x10000000" -ex "add-symbol-file bin/ot-sed-thermometer-freertos" -ex "load" -ex "monitor reset"
```

The above one-liner automates the flashing and testing of the demonstration. It connects to the GDB server, flashes the latest secure and nonsecure firmware and resets the chip, which is now ready to debug. This command must be run from the Cascoda build directory.

Before running the script, you must set the `GDB_SERVER_IP` environment variable to the IP and port of your J-Link GDB server, e.g. `$ GDB_SERVER_IP=192.168.10.212:2331`. Alternatively, you can modify the one-liner by replacing `$GDB_SERVER_IP` with the server's IP and port.

Once you run the command and accept the prompts, the demo is ready to be tested.
## How to run & test

This demonstration requires another Chili to act as the server. Flash the server Chili with the `mac-dongle` binary. You must also build the `ot-sensordemo-server` POSIX application, from within the Cascoda POSIX SDK build directory.

Once the devices are all ready, plug the server Chili into a Linux machine and run the `ot-sensordemo-server` executable. Then, power the end device Chili. After 10 to 20 seconds, the server will receive discover requests, as well as temperature readouts from the end device.
```
alexandru@CASCODA211:~/chili-sdk$ ../posix-sdk/bin/ot-sensordemo-server
Server received discover from [fdde:ad00:beef:0:7ee4:f542:d39b:3577]
Server received temperature 21.7*C from [fdde:ad00:beef:0:7ee4:f542:d39b:3577]
Server received temperature 21.7*C from [fdde:ad00:beef:0:7ee4:f542:d39b:3577]
Server received temperature 22.1*C from [fdde:ad00:beef:0:7ee4:f542:d39b:3577]
Server received temperature 21.7*C from [fdde:ad00:beef:0:7ee4:f542:d39b:3577]
Server received temperature 22.1*C from [fdde:ad00:beef:0:7ee4:f542:d39b:3577]
Server received temperature 22.1*C from [fdde:ad00:beef:0:7ee4:f542:d39b:3577]
Server received temperature 21.7*C from [fdde:ad00:beef:0:7ee4:f542:d39b:3577]
Server received temperature 21.2*C from [fdde:ad00:beef:0:7ee4:f542:d39b:3577]
Server received temperature 21.7*C from [fdde:ad00:beef:0:7ee4:f542:d39b:3577]
```