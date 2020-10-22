# ot-sed-sensorif

This is a simple, self-contained Thread Sleepy End Device (SED) demo, that periodically measures temperature, humidity, light intensity and PIR count, and sends the data over the thread network to a server, using CBOR and CoAP. This demo requires that the sensors are actually connected to the board, but the demo could be expanded to use other sensors as required.

This demo communicates with a server (a linux system running ot-sensordemo-server).
