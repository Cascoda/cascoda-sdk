# Sleepy End Device FreeRTOS demo #

This demonstration shows the OpenThread sleepy end device capabilities, and how FreeRTOS can enhance them.

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