# Baremetal

This directory contains the Baremetal-specific code, including drivers, platform abstraction and examples. This is used on Cascoda's Chili platforms, and can easily be ported to different MCUs.

## Directory structure

### app

Example applications

### cascoda-bm-driver

General baremetal drivers and API. This includes the EVBME (Evaluation Board Management Entity), which enables any host platform connected by USB/UART to control the device using EVBME commands.

### cascoda-bm-core

Minimal baremetal driver. This contains the minimal functionality required to control a CA-821x via SPI without any of the extended EVBME functionality.

### cascoda-bm-thread

Port layer for the OpenThread stack. This enables using OpenThread on baremetal platforms with a CA-821x radio. The [OpenThread API](https://openthread.io/reference) itself should be used for controlling the OpenThread stack.

### platform

Platform abstraction layers for different microcontrollers. These are split into seperate directories containing both vendor code from the MCU vendor and port code written by Cascoda.

### sensorif

Set of example drivers for extracting data from connected sensors and controlling actuators. This abstracts away from the base I2C/SPI bus protocols to C functions that simply return values.
