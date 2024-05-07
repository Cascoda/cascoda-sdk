# mikrosdk-bm

Example code for using the mikrosdk-click libraries to interface with external sensors.

I2C clients and SPI slaves are using I2C1 and SPI1 interface to communicate with the Chili2.

## Pin configuration for each device:

| Manufacturer      | Device               | Interface |Pin 31 (PB5) | Pin 32 (PB4) | Pin 33 (PB3)  | Pin 34 (PB2)
| :---------------- | :--------------------| ----------|-------------|--------------|---------------|-------------
| MikroElektronika  | Air Quality 4 Click  |  I2C      |   SCL  |   SDA  |    -    |   -
| MikroElektronika  | Environment2 Click   |  I2C      |   SCL  |   SDA  |    -    |   -
| MikroElektronika  | Motion Click         |  GPIO     |    -   |    -   |   EN    |   OUT
| MikroElektronika  | Relay Click          |  GPIO     |    -   |    -   | Relay 1 | Relay 2
| MikroElektronika  | Thermo Click         |  SPI      |  MISO  |    -   |  SCK    |   CS
| MikroElektronika  | Thermo3 Click        |  I2C      |   SCL  |   SDA  |    -    |   -


## Test a MikroElektronica sensor with the Chili2
---

1. [Set up the environment and build system](../../../docs/reference/full-reference.md#building).
2. [Flash the Chili device](../../../docs/dev/flashing.md) with the binary called `mikrosdk-test` located in the `bin` folder of the build directory for the Chili, which was created in the previous step.
3. Making sure the Chili device is connected to your Windows/Linux/MacOS machine, run the [serial-adapter](../../../posix/app/serial-adapter/README.md) tool (`serial-adapter.exe` on Windows).
4. `serial-adapter` will now be displaying sensor values that are being read by the MikroElectronika sensors attached.