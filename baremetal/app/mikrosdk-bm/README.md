# mikrosdk-bm
Example code for using the mikrosdk-click libraries to interface with external sensors.

These examples are very basic and simply print the values that are read from the connected interface.

I2C clients and SPI slaves are using I2C1 and SPI1 interface to communicate with Chili2.
### Pin configuration for each device:

| Manufacturer      | Device               | Interface |Pin 31 (PB5) | Pin 32 (PB4) | Pin 33 (PB3)  | Pin 34 (PB2)
| :---------------- | :--------------------| ----------|-------------|--------------|---------------|-------------
| MikroElektronika  | Air Quality 4 Click  |  I2C      |   SCL  |   SDA  |    -    |   -
| MikroElektronika  | Environment2 Click   |  I2C      |   SCL  |   SDA  |    -    |   -
| MikroElektronika  | Motion Click         |  GPIO     |    -   |    -   |   EN    |   OUT
| MikroElektronika  | Relay Click          |  GPIO     |    -   |    -   | Relay 1 | Relay 2
| MikroElektronika  | Thermo Click         |  SPI      |  MISO  |    -   |  SCK    |   CS
| MikroElektronika  | Thermo3 Click        |  I2C      |   SCL  |   SDA  |    -    |   -

## Commands
---
### List
Each Chili device is assigned a unique serial number when manufactured. In order to obtain a device's serial number, a user is expected to enter ``chilictl.exe list`` to display all the information about the Chili device, including the device name, the app that it is currently running on, its version, a serial number, a path, its availibility, and whether it allows an external flash chip to be connected to it. 

This command is useful for finding the serial number and checking the app that is currently running on the device.

    > .\chilictl.exe list

Example:
```
> .\chilictl.exe list
Device Found:
        Device: Chili2
        App: mikrosdk-test
        Version: v0.21-599-g5c73f60f-dirty
        Serial No: 4FE331709CAFEA4A
        Path: \\?\hid#vid_0416&pid_5020#8&aae61a8&0&0000#{4d1e55b2-f16f-11cf-88cb-001111000030}
        Available: Yes
        External Flash Chip Available: No
```

### Flashing
If there is an app that you would like to run it on the Chili2 device, the Chili device has to be flashed with the bin file. During the flashing, it will restart the Chili device and replace the current running app with the new app. Alongside these process, it also performs verification and validation.

    > .\chilictl.exe flash -s <serial number> -f <file path>

Example:
```
> .\chilictl.exe flash -s 4FE331709CAFEA4A -f C:\Desktop\cascoda\sdk-chili2\bin\test\mikrosdk-test.bin
1 devices found.
Flasher [4FE331709CAFEA4A]: INIT -> REBOOT
Flasher [4FE331709CAFEA4A]: REBOOT -> ERASE
Flasher [4FE331709CAFEA4A]: ERASE -> FLASH
Flasher [4FE331709CAFEA4A]: FLASH -> VERIFY
Flasher [4FE331709CAFEA4A]: VERIFY -> VALIDATE
Flasher [4FE331709CAFEA4A]: VALIDATE -> COMPLETE
```

## A guide for testing a sensor with Chili2 on Windows OS
---

Follow the instructions on cascoda-sdk to set up the environment and build the system. Click [here](https://github.com/Cascoda/cascoda-sdk#guides) to be redirected to the guide. After the environment is properly setup, continue to the next instruction.

Open Windows PowerShell and navigate to the build system for Windows.

Find the serial number for the chili device.

    > .\chilictl.exe list

Flash the chili device with a binary file (replace the serial number and file path with appropriate value):

    > .\chilictl.exe flash -s <serial number> -f <file path>

> The file for testing mikrosdk devices is ``mikrosdk-test.bin`` which should be located in sdk-chili2/bin/test.

Display standard outputs on Window Powershell command line:

    > .\serial-adapter.exe

