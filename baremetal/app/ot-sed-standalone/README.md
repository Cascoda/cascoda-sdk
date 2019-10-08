# Sleepy End Device baremetal demo #

This demonstration shows the OpenThread sleepy end device capabilities.

## How to build ##
```bash
make sed-standalone
```

## GDB One-Liner ##

```bash
arm-none-eabi-gdb -ex "target remote $GDB_SERVER_IP" -ex "file bin/sed-standalone" -ex "load" -ex "monitor reset"
```

The above one-liner automates the flashing and testing of the demonstration. It connects to the GDB server, flashes the latest secure and nonsecure firmware and resets the chip, which is now ready to debug. This script must be run from the Cascoda build directory.

Before running the script, you must set the `GDB_SERVER_IP` environment variable to the IP and port of your J-Link GDB server, e.g. `$ GDB_SERVER_IP=192.168.10.212:2331`. Alternatively, you can modify the one-liner by replacing `$GDB_SERVER_IP` with the server's IP and port.

Once you run the command and accept the prompts, the demo is ready to be tested.
## How to run & test

This demonstration requires another Chili to act as the server. Flash the server Chili with the `test15-4-app` binary. You must also build the `ot-server-standalone` POSIX application, from within the Cascoda POSIX SDK build directory.

Once the devices are all ready, plug the server Chili into a Linux machine and run the `ot-server-standalone` executable. Then, power the end device Chili. After 10 to 20 seconds, the server will receive discover requests, as well as temperature readouts from the end device.
```
alexandru@CASCODA211:~/chili-sdk$ ../posix-sdk/bin/ot-server-standalone
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