# Flashing Guide

For flashing binaries built for embedded targets, or downloaded from the [GitHub releases page](https://github.com/Cascoda/cascoda-sdk/releases) there are multiple approaches.

1. Using the [chilictl](../../posix/app/chilictl) command line application to flash over USB or UART
2. Using a [SEGGER J-Link](https://www.segger.com/products/debug-probes/j-link/)
3. Using a [Nu-Link Pro](https://direct.nuvoton.com/en/nu-link-pro)

For flashing the Chili 2D with TrustZone (CSME) binaries, please refer to the [TrustZone development guide](M2351-TrustZone-development-guide.md#flashing-trustzone). This guide is only for non-TrustZone binaries.

** Warning: If a device has previously been flashed with a TrustZone binary, it must be fully erased using the Nu-Link Pro and the ICP tool. **

## USB and UART Flashing with chilictl

For simple evaluation and reflashing of Chili devices over USB or UART, the [chilictl](../../posix/app/chilictl) application can be used.
This is a command line application, that allows high level control of connected Chili devices, including listing, inspecting, flashing and issuing them commands. More detailed information is available [here.](../../posix/app/chilictl)

In order to flash a connected Chili, it must have a version of the Cascoda SDK firmware on it of v0.14 or later, with either USB or UART enabled. The new firmware has the same requirement, which is met by every example application that isn't sleepy.

Chilictl is failsafe, so if anything goes wrong during flashing, simply power cycle the device, and it will reboot into DFU mode where it can be reflashed.

Software requirements:

- A version of chilictl for your platform. For windows, it is in the ``Windows-SDK.zip`` folder on the [releases page](https://github.com/Cascoda/cascoda-sdk/releases).
  - For other platforms, the Cascoda SDK can be built natively from source, as [detailed in the build guide.](../../README.md#building)
- For USB flashing, the chilictl DFU firmware 'ldrom_hid.bin' and a USB enabled binary to flash, available in the ``Chili2D-USB.zip`` folder on the [releases page](https://github.com/Cascoda/cascoda-sdk/releases).
- For UART flashing, the chilictl DFU firmware 'ldrom_uart.bin' and a UART enabled binary to flash. These can be built from source.

Hardware requirements:

- A USB/UART Chili2 or a devboard, and a cable to connect it to a PC

### Procedure

Chilictl is a command line application used for working with Chili devices. A binary can be flashed using the `chilictl flash` command, with the `-f` flag specifying the binary file to flash.


```bash
# The -s argument is optional, and specifies the specific chili to flash (by serial number)
# Replace the path with your own path to the desired binary
$ ./chilictl.exe flash -s FBC647CDB300A0DA -f "../Chili2D-USB/ot-cli.bin"
2020-12-16 12:56:12.130 NOTE:  Cascoda SDK v0.14 Dec 16 2020
Flasher [FBC647CDB300A0DA]: INIT -> REBOOT
Flasher [FBC647CDB300A0DA]: REBOOT -> ERASE
Flasher [FBC647CDB300A0DA]: ERASE -> FLASH
Flasher [FBC647CDB300A0DA]: FLASH -> VERIFY
Flasher [FBC647CDB300A0DA]: VERIFY -> VALIDATE
Flasher [FBC647CDB300A0DA]: VALIDATE -> COMPLETE
```

Chili2Ds ship with the USB bootloader pre-installed - in normal operation you will not need to flash the bootloader. However, Chilictl allows you to do so using the `-df` flag.

```bash
$ ./chilictl.exe flash -s FBC647CDB300A0DA -df "../Chili2D-USB/ldrom_hid.bin"
2020-12-16 12:59:46.510 NOTE:  Cascoda SDK v0.14 Dec 16 2020
Flasher [FBC647CDB300A0DA]: INIT -> REBOOT
Flasher [FBC647CDB300A0DA]: REBOOT -> ERASE
Flasher [FBC647CDB300A0DA]: ERASE -> FLASH
Flasher [FBC647CDB300A0DA]: FLASH -> VERIFY
Flasher [FBC647CDB300A0DA]: VERIFY -> VALIDATE
Flasher [FBC647CDB300A0DA]: VALIDATE -> COMPLETE
```

## SEGGER J-Link

_Note that when using the SEGGER J-Link, it is also possible to [flash and debug from GDB.](debug-with-segger-jlink.md)_

The SEGGER J-Link in combination with J-Flash Lite can be used to simply flash a device with a given binary.

Software Requirements:

- The latest [SEGGER J-Link Software & Documentation Pack](https://www.segger.com/downloads/jlink/)
- Can be run on Windows, macOS or Linux

Hardware Requirements:

- A suitable SEGGER J-Link
- An adapter from the 20-pin JTAG to 9-pin or 10-pin swd connector
- The target platform (This guide assumes Chili2D)

### Procedure

1. Connect J-Link to Chili2D via debug adapter
2. Power the Chili2D
3. Connect the J-Link to the the host PC
4. Run the SEGGER J-Flash Lite
5. Set the target device (M2351... For Chili2, NANO120... for Chili1)
6. Set the target interface to SWD
7. Click OK

<p align="center"><img src="img/jlink/jflashlite.png" align="center"></p>

8. Browse to the desired binary file using the [...] button, and then click 'Program Device'.
9. Verify that there are no errors in the Log pane

## Nu-Link Pro

The Nu-Link Pro in combination with the Nuvoton ICP programming tool can be used to flash a device, change chip settings and fully factory erase a device.

Software Requirements:

- The latest [Nuvoton ICP programming tool](https://www.nuvoton.com/tool-and-software/software-development-tool/programmer/)
- Can only be run on Windows

Hardware Requirements:

- A [Nu-Link Pro](https://direct.nuvoton.com/en/nu-link-pro)
- An adapter from the NuLink to 9-pin or 10-pin swd connector (Can be wired into jlink adapter or contact us to obtain one)
- The target platform (This guide assumes Chili2D)

### Procedure

1. Connect the Nu-Link Pro to the Chili2D via debug adapter
2. Do NOT power the Chili2D (It is powered by the Nu-Link Pro)
3. Start the [Nuvoton NuMicro ICP Programming Tool](https://www.nuvoton.com/tool-and-software/software-development-tool/programmer/)
4. Select M2351 Series target chip
5. Click 'Connect' to connect to M2351
6. Click the 'APROM' button to browse to the APROM binary file
7. Ensure that only the 'APROM' checkbox is selected
8. Click 'Start' to program the device
9. Click 'No' on the batch programming prompt

<p align="center"><img src="img/icp/main-notz.png" width="50%" align="center"></p>

---
_Copyright (c) 2021 Cascoda Ltd._
