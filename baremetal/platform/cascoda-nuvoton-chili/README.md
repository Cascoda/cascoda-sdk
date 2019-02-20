# cascoda-nuvoton-chili
This repository contains baremetal drivers and example applications for the Nuvoton Chili module.

## Toolchains
The applications can be built using various toolchains. Supported toolchains/IDEs include:<br>
[GNU ARM Embedded Toolchain](https://developer.arm.com/open-source/gnu-toolchain/gnu-rm)<br>
[IAR Embedded Workbench for ARM](https://www.iar.com/iar-embedded-workbench/#!?architecture=ARM)

## Directory structure
#### apps
Example applications. Each subdirectory contains source code as well as builds for supported toolchains. Some source code takes the form of a git submodule, linked to the portable application project.

#### cascoda-bm-driver
Baremetal drivers for Cascoda devices and supporting host functionality.

#### port
Platform-specific code linking cascoda-bm-driver to vendor libraries.

#### vendor
Libraries provided by MCU manufacturer (Nuvoton)

## Development
To get started, clone this repository and checkout all submodules. This can be done in one step using the `--recursive` switch for git clone like so:
```
git clone --recursive https://github.com/Cascoda/cascoda-nuvoton-chili
```

The following symbols can be defined when compiling any of the apps:
- OLD_DESIGN - If defined, this symbol will target the first revision of the chili module. If not defined the code will be built for the latest version.
- USE_USB - If defined, the USB interface will be used for upstream communication. Otherwise, The UART interface will be used as a default alternative.

### Debugging
The Chili supports flashing and debugging via the [Segger J-Link](https://www.segger.com/products/debug-probes/j-link/) using [JTAG SWD](https://en.wikipedia.org/wiki/JTAG#Serial_Wire_Debug). If using IAR there are some settings to enable JTAG debugging: 
1. In "General Options"->"Debugger" on the "Setup" tab change the driver drop-down to" J-Link/J-Trace". 
2. On the "Download" tab check the "Use flash loader(s)" box to enable flashing with the J-link. 
3. Then in "General Options"->"Debugger"->"J-Link/J-Trace" on the "Connection" tab change "Interface" from "JTAG" to "SWD". 

## System configuration
### Interrupts
Several interrupts are used in this project with different priorities. Interrupts with a higher priority (smaller value) will interrupt lower priority ISRs. The interrupts and their priorities are as follows:

| Interrupt  | Priority |
| ---------- | -------- |
| TMR0_IRQn  | 0        |
| TMR1_IRQn  | 0        |
| TMR2_IRQn  | 0        |
| TMR3_IRQn  | 0        |
| USBD_IRQn  | 2        |
| GPABC_IRQn | 1        |
| GPDEF_IRQn | 1        |
| UART1_IRQn | 2        |
