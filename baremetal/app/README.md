# app
Example applications for the CA-821x baremetal drivers. These link with the cascoda-bm-driver and ca821x-api.

## eink-bm
Simple non-connected demo for the Chili2 which displays an image on a connected epaper display

## ot-actuatordemo
A simple demo that demonstrates the control of several actuators (running ot-cli-actuator), by a central controller (also running ot-cli-actuator). This demo provides a CLI for interaction (serial-adapter).

## ot-sensordemo
A simple demo, similar to ot-actuatordemo, that demonstrates control of several sensors. Exists in ../cascoda-bm-thread as part of ot-cli.

## ot-baremetal
Extremely simplistic demo of openthread, using cascoda EVBME commands for very basic control of the Thread stack for testing.

## ot-sed-eink-freertos
A scalable demo for connecting 1000s of nodes to a Thread network using ephemeral connectivity. Each node periodically receives an update from a central server (ot-eink-server on posix) which will cause it to update the image displayed on its attached epaper display.

## ot-sed-thermometer-freertos
An alternative to sed-standalone that runs on Trustzone and FreeRTOS on the Chili2. Sends periodic temperature updates to server-standalone.

## ot-sed-sensorif
A similar demo to sed-standalone that sends additional sensor readings from connected sensors such as humidity, light level, PIR count using the cascoda sensorif library.

## ot-sed-thermometer
A temperature sensing demo for SEDs that works with server-standalone in posix thread directory. The SED connects to a server and sends periodic temperature updates.

## sensorif-bm
A simple non-connected demo that simply prints the values from connected sensors using the cascoda sensorif library.

## mac-tempsense
Temperature sensor application which uses the IEEE802.15.4 MAC layer functionality to establish a star network with one central
coordinator and up to 32 temperature sensor end nodes. The coordinator uses USB HID or UART for host communications to Cascoda's
Wing Commander GUI.

## mac-dongle 
Baremetal implementation of IEEE802.15.4 physical layer (PHY) test functions. The application uses USB HID or UART for host
communications to Cascoda's Wing Commander GUI for control and reporting.