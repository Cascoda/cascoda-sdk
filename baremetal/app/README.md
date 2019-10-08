# app
Example applications for the CA-821x baremetal drivers. These link with the cascoda-bm-driver and ca821x-api.

## eink-bm
Simple non-connected demo for the Chili2 which displays an image on a connected epaper display

## ot-baremetal
Baremetal interface of the OpenThread Thread network stack. 

Includes thread-bm, which is used to make cliapp-bm and ncpapp-bm, pure openthread demos with commandline / network coprocessor interfaces.

## ot-sed-eink
A scalable demo for connecting 1000s of nodes to a Thread network using ephemeral connectivity. Each node periodically receives an update from a central server (ot-server-eink on posix) which will cause it to update the image displayed on its attached epaper display.

## ot-sed-freertos
An alternative to sed-standalone that runs on Trustzone and FreeRTOS on the Chili2. Sends periodic temperature updates to server-standalone.

## ot-sed-sensorif
A similar demo to sed-standalone that sends additional sensor readings from connected sensors such as humidity, light level, PIR count using the cascoda sensorif library.

## ot-sed-standalone
A temperature sensing demo for SEDs that works with server-standalone in posix thread directory. The SED connects to a server and sends periodic temperature updates.

## sensorif-bm
A simple non-connected demo that simply prints the values from connected sensors using the cascoda sensorif library.

## tempsense-bm
Temperature sensor application which uses the IEEE802.15.4 MAC layer functionality to establish a star network with one central
coordinator and up to 32 temperature sensor end nodes. The coordinator uses USB HID or UART for host communications to Cascoda's
Wing Commander GUI.

## test15_4-baremetal
Baremetal implementation of IEEE802.15.4 physical layer (PHY) test functions. The application uses USB HID or UART for host
communications to Cascoda's Wing Commander GUI for control and reporting.