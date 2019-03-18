# app
Example applications for the CA-821x baremetal drivers. These link with the cascoda-bm-driver and ca821x-api.

## test15_4-baremetal
Baremetal implementation of IEEE802.15.4 physical layer (PHY) test functions. The application uses USB HID or UART for host
communications to Cascoda's Wing Commander GUI for control and reporting.

## tempsense-bm
Temperature sensor application which uses the IEEE802.15.4 MAC layer functionality to establish a star network with one central
coordinator and up to 32 temperature sensor end nodes. The coordinator uses USB HID or UART for host communications to Cascoda's
Wing Commander GUI.

## ot-baremetal
Baremetal interface of the OpenThread Thread network stack.

## sensorif-bm
Example code for adding external sensors to applications.
