# KNX IoT - Getting Started Guide with KNX tooling

This guide will get you up and running with KNX Tooling! Prior KNX knowledge is needed.

Note: this document will change!!.

## overview

To use ETS, one has to have product data

                                | actual devices 
                                V
     -------                 -------
    |       |               |       |
    |   MT  |   -------->   |  ETS  |
    |       |  product      |       |
     -------     data        -------
             of actual device

The tool to create product data is the MT tool.
The MT tool has a few input files:

- Application file
  - The mask version to use is: KNX IoT
- Hardware file
- Catalog file

## Step 0: Requirements

Please ensure you have the following software:

- MT, Manufactorer tool, version with KNX IoT Enabled.
- ETS, version with KNX IoT enabled
- Cascoda's development Kit, see: xxxx
- Devices on the thread network, see xxxx
- border router attached to the PC, see xxx

## Step 1: Creating ETS project with MT

- download the configuration files for the specific application
  - or make them by hand
- run manufactorer tool to create project that can be used by ETS
  - run MT as batch
  - run MT as interactive program:
    - Compile the solution
    - Export as project as test solution.

## Step 2: Using ETS

To use ETS one has to have:
- actual devices
- (test) project that describes the actual device.
  
To create a configuration one has to create a topology first:
- backbone is IPV4
- add IoT area to the backbone
- copy devices to the IoT area
- create links between the devices as usual
- download the application (configuration) to the device