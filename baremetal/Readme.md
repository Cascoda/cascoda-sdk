# Baremetal

This directory contains the Baremetal-specific code, including drivers, platform abstraction and examples. This is in contrast to `posix`, which can only run on a POSIX or Windows OS, or `ca821x-api` and `ca821x-utils`, which can be used in both OS and embedded contexts. The baremetal code is used on Cascoda's Chili platforms, and can easily be ported to different MCUs.

## Directory structure

### app

This folder contains example applications for:
- Using the OpenThread CLI (`ot-cli-*`)
- Using Thread Sleepy End Devices (`ot-sed-*`)
- Interfacing with I2C or SPI sensors (`ot-sed-sensorif`)
- Interfacing with actuators over UART (`ot-cli-actuator`) 
- Interfacing with a host application over USB or UART (see `CASCODA_BM_INTERFACE` CMake configuration variable)
- Interfacing with I2C or SPI sensors (`sensorif-bm`)
- Interfacing with MikroEletronika devices (`mikrosdk-bm`).

### cascoda-bm-driver

General baremetal drivers and API. This includes the [EVBME (Evaluation Board Management Entity)](../docs/reference/evbme.md), which enables any host platform connected by USB/UART to control the device using EVBME commands.

### cascoda-bm-core

Minimal baremetal driver. This contains the minimal functionality required to control a CA-821x via SPI without any of the extended EVBME functionality.

### cascoda-bm-thread

Port layer for the OpenThread stack. This enables using OpenThread on baremetal platforms with a CA-821x radio. The [OpenThread API](https://openthread.io/reference) itself should be used for controlling the OpenThread stack.

### mikrosdk-click

Set of example drivers for extracting data from connected sensors and controlling actuators. It abstracts away the details of the underlying I2C/SPI communications. The drivers in this folder are written by MikroElektronika, which provide libraries for their sensors.

### mikrosdk-lib

A mid-level adaptation layer between the driver API and hardware. It integrates the MikroElektronika library with the Cascoda libraries in the HAL (Hardware Abstraction Layer), allowing communication between MikroElektronika devices and Chili2 via I2C/SPI and GPIO.

### platform

Platform abstraction layers for different microcontrollers. These are split into seperate directories containing both vendor code from the MCU (Microcontroller Unit) vendor and port code written by Cascoda.

### sensorif

Set of specific (non-MikroElektronika) drivers for extracting data from connected sensors and controlling actuators. This abstracts away the details of the underlying I2C/SPI communications.

### test

Set of unit tests that are run at build time to verify common functionality.
