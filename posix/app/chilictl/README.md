# chilictl

A Chili control application for listing and flashing connected Chili devices.

It can run on Windows or Posix and can be used for:
- Listing all connected chilis
- Filtering listed chilis by certain parameters (currently only serialno and availability)
- Flashing new applications to connected chili devices over USB
- Flashing new DFU (Device Firmware Update) firmware to connected chili devices over USB

Prebuilt Windows binaries of chilictl can be found in the [Windows release of the Cascoda SDK.](https://github.com/Cascoda/cascoda-sdk/releases/)

Please make sure that you have set up the USB exchange [as detailed in the development setup guide.](../../../docs/guides/development-setup.md)

Note that this tool is still under development, so may have undiscovered issues.

Run ./chilictl with the ``--help`` argument to print the help page. This option is also available on the sub-commands.

```
$ ./chilictl -h
--- Chili Control Application ---
SYNOPSIS
        ./chilictl [options] <command> [command options]
OPTIONS
        -h, --help
                Print this message to stdout and exit.
        -v, --version
                Print version information to stdout and exit.
COMMANDS
        list
                Utility for listing connected chili devices
        flash
                Utility for flashing new binaries to connected chili devices
        pipe
                Utility for piping binary commands to/from a connected chili device
```

## Listing

In order to discover connected chili devices and their meta-information, the 'list' sub command can be used.

### List help page

```
# Check the output of --help for the latest help page
$ ./chilictl list -h
--- Chili Control: List Sub-Application ---
SYNOPSIS
        chilictl [options] list [command options]
COMMAND OPTIONS
        -h, --help
                Print this message to stdout and exit.
        --available[=<yes|no>]
                Filter the list to only include available devices, or unavailable devices if 'no'.
        -s <serialno>, --serialno=<serialno>
                Filter the list to only include the device with the given serial number.
```

### List output

```
$ ./chilictl list
2020-12-16 12:53:35.897 NOTE:  Cascoda SDK v0.14 Dec 16 2020
Device Found:
        Device: Chili2
        App: mac-dongle
        Version: v0.12-100-g4e923e99
        Serial No: B36620361EDFCBA6
        Path: 0003:000f:00
        Available: Yes
Device Found:
        Device: Chili2
        App: mac-dongle
        Version: v0.12-100-g4e923e99
        Serial No: 1656A3C17B8F7664
        Path: 0003:000e:00
        Available: Yes
```

## Flashing

Note that currently the flashing functionality is only available over USB, and requires firmware v0.14 or newer.
A DFU firmware is required to be flashed before the application can be reflashed. See the '-d' flag.
The flashing process is designed to be failsafe. If the application is incorrectly flashed or not working, the device
should fail into a stable state where it can be power cycled and reflashed. However, as a general precaution, the process
should not be intentionally interrupted while it is in progress.

To get the serial number for the 's' argument, the chilictl ``list`` sub command can be used.

### Flash help page

```
# Check the output of --help for the latest help page
$ ./chilictl flash -h
--- Chili Control: Flashing Sub-Application ---
SYNOPSIS
        chilictl [options] flash [command options]
COMMAND OPTIONS
        -h, --help
                Print this message to stdout
        -s <serialno>, --serialno=<serialno>
                Flash the device with the given serial number.
        -b, --batch
                Permit the flashing of multiple devices at a time.
        -f <filepath>, --file=<filepath>
                Set the .bin file to flash to the device(s)
        -d, --dfu-update
                Update the DFU region itself, rather than the application.
```

### DFU region update

To use the reflashing functionality, a DFU firmware must be flashed alongside the application. This only has to be done once, or to update it. The ``--dfu-update`` or ``-d`` argument is used for this.

```
$ ./chilictl flash -s FBC647CDB300A0DA -df "~/sdk-chili2/bin/ldrom-hid.bin"
2020-12-16 12:56:46.510 NOTE:  Cascoda SDK v0.14 Dec 16 2020
Flasher [FBC647CDB300A0DA]: INIT -> REBOOT
Flasher [FBC647CDB300A0DA]: REBOOT -> ERASE
Flasher [FBC647CDB300A0DA]: ERASE -> FLASH
Flasher [FBC647CDB300A0DA]: FLASH -> VERIFY
Flasher [FBC647CDB300A0DA]: VERIFY -> VALIDATE
Flasher [FBC647CDB300A0DA]: VALIDATE -> COMPLETE
```

### Application flashing

If a DFU firmware is flashed as described above, then the application can be replaced with a new binary by specifying a serial number
and file as arguments.

```
$ ./chilictl flash -s FBC647CDB300A0DA -f "~/sdk-chili2/bin/mac-dongle.bin"
2020-12-16 12:59:12.130 NOTE:  Cascoda SDK v0.14 Dec 16 2020
Flasher [FBC647CDB300A0DA]: INIT -> REBOOT
Flasher [FBC647CDB300A0DA]: REBOOT -> ERASE
Flasher [FBC647CDB300A0DA]: ERASE -> FLASH
Flasher [FBC647CDB300A0DA]: FLASH -> VERIFY
Flasher [FBC647CDB300A0DA]: VERIFY -> VALIDATE
Flasher [FBC647CDB300A0DA]: VALIDATE -> COMPLETE
```

On a high level, the different phases are:

| Phase    | Description |
| -------- | ----------- |
| INIT     | Finding a connected device to flash
| REBOOT   | Rebooting the device into DFU mode, set bootmode as DFU
| ERASE    | Erase the application flash
| FLASH    | Write the application flash
| VERIFY   | Verify that the flash was written correctly
| VALIDATE | Validate that the device can successfully boot into application and communicate with chilictl. Set bootmode to application.

## Piping

The ``pipe`` subcommand can be used to pipe binary data to and from a connected chili device.

### Pipe help page

```bash
# Check the output of --help for the latest help page
$ ./chilictl.exe pipe -h
--- Chili Control: Binary piping Sub-Application ---
SYNOPSIS
        chilictl [options] pipe [command options]
COMMAND OPTIONS
        -h, --help
                Print this message to stdout
        -s <serialno>, --serialno=<serialno>
                Pipe the device with the given serial number.
        -a, --any
                Pick any matching device, rather than throwing error if more than one match.
```

### Pipe example: MLME Get

The pipe subcommand is intended to be used in a binary pipeline such as a script or host program.

```bash
# Note: xxd converts between binary and hexadecimal
$ echo '45020000' | xxd -r -p | ./chilictl.exe pipe -s FBE1D029210B662B | xxd -p
2021-04-12 19:03:11.661 NOTE:  Host Cascoda SDK v0.16-56-g16333291-dirty Apr 12 2021
68050000000112
```