# cascoda-nuvoton-chili2
Platform abstraction for the Chili 2 module.<br>
Module: Chili 2<br>
MCU:    Nuvoton M2351 (M2351ZIAAE1)<br>
Also supports the Nuvoton NuMaker-PFM-M2351 Development Board

## Directory structure
#### port
Platform-specific code linking cascoda-bm-driver to vendor libraries.

| File | Description |
| :--- | :--- |
|cascoda_bsp_chili.c | contains interface functions declared in cascoda-bm/cascoda_interface.h |
|cascoda_isr_chili.c | contains Interrupt Service Routines (ISRs) |
|cascoda_chili.c | functions internal to port |
|cascoda_dataflash_m2351.c | non-volatile memory access functions for data storage |
|cascoda_usb_m2351.c | Device USB HID driver functions for communications with host (if USE_USB is defined) |

#### vendor
Libraries provided by the MCU manufacturer (Nuvoton).<br>
The required libraries (standard drivers, CMSIS) have been extracted unchanged from the following BSP version:<br>
Nuvoton M2351_BSP_v3.00.001


## System configuration
### Pin Configuration for Chili 2 USB Module (double-sided population)
 Pin    | Default Function  | Chili 2 Function | Usage
 -----: | :------------- | :----------------- | :-----------------------------
   1    | PB.5     IN    | PB.5        IN     | Connector
   2    | PB.4     IN    | PB.4        IN     | Connector
   3    | PB.3     IN    | PB.3        IN     | Connector
   4    | PB.2     IN    | PB.2        IN     | Connector
   5    | PB.1     IN    | EADC0_CH1   IN     | VOLTS; Battery Voltage Measurement ADC Input
   6    | PB.0     IN    | PB.0        IN     | CHARGE_STAT; Battery Charge Status
   7    | PF.5     IN    | X32_IN      IN     | 32 kHz Xtal
   8    | PF.4     IN    | X32_OUT     I/O    | 32 kHz Xtal
   9    | PF.3     IN    | XT1_IN      IN     | System Clock from CA-821x to M2351
  10    | PF.2     IN    | XT1_OUT     I/O    | NC
  11    | PA.3     IN    | PA.3        OUT    | SPI_CS      (CA821x SSB,    P22)
  12    | PA.2     IN    | SPI0_SCLK   OUT    | SPI_CLK     (CA821x SCLK,   P21)
  13    | PA.1     IN    | SPI0_MISO   IN     | SPI_MISO    (CA821x MISO,   P24)
  14    | PA.0     IN    | SPI0_MOSI   OUT    | SPI_MOSI    (CA821x MOSI,   P23)
  15    | VDD33    SUP   | VDD33       SUP    | -
  16    | nRESET   IN    | nRESET      IN     | -
  17    | PF.0     IN    | ICE_DAT     I/O    | TMS
  18    | PF.1     IN    | ICE_CLK     IN     | TCK
  19    | PC.1     IN    | PC.1        OUT    | ZIG_RESET   (CA821x NRESET, P27)
  20    | PC.0     IN    | PC.0        IN     | ZIG_IRQB    (CA821x NIRQ,   P20)
  21    | PA.12    IN    | USB_5V      SUP    | USB VBUS
  22    | PA.13    IN    | USB_D-      I/O    | USB D-
  23    | PA.14    IN    | USB_D+      I/O    | USB D+
  24    | PA.15    IN    | PA.15       IN     | Connector
  25    | VSS      SUP   | VSS         SUP    | -
  26    | VSW      SUP   | VSW         SUP    | -
  27    | VDD      SUP   | VDD         SUP    | -
  28    | LDO_CAP  SUP   | LDO_CAP     SUP    | -
  29    | PB.14    IN    | PB.14       IN     | ZIG_APP     (CA821x DIG3,   P26, GPIO9)
  30    | PB.13    IN    | PB.13       OUT    | VOLTS_TEST; Battery Voltage Measurement Control
  31    | PB.12    IN    | PB.12       IN     | USB_PRESENT; USB Supply Status
  32    | AVDD     SUP   | AVDD        SUP    | -
### Pin Configuration for Chili 2 solder-on module (one-sided population)
 Pin    | Default Function  | Chili 2 Function | Usage
 -----: | :------------- | :----------------- | :-----------------------------
   1    | PB.5     IN    | PB.5        IN     | Connector
   2    | PB.4     IN    | PB.4        IN     | Connector
   3    | PB.3     IN    | PB.3        IN     | Connector
   4    | PB.2     IN    | PB.2        IN     | Connector
   5    | PB.1     IN    | PB.1        IN     | Connector
   6    | PB.0     IN    | PB.0        IN     | Connector
   7    | PF.5     IN    | X32_IN      IN     | 32 kHz Xtal
   8    | PF.4     IN    | X32_OUT     I/O    | 32 kHz Xtal
   9    | PF.3     IN    | XT1_IN      IN     | System Clock from CA-821x to M2351
  10    | PF.2     IN    | XT1_OUT     I/O    | NC
  11    | PA.3     IN    | PA.3        I/O    | SPI_CS      (CA821x SSB,    P22)
  12    | PA.2     IN    | SPI0_SCLK   OUT    | SPI_CLK     (CA821x SCLK,   P21)
  13    | PA.1     IN    | SPI0_MISO   IN     | SPI_MISO    (CA821x MISO,   P24)
  14    | PA.0     IN    | SPI0_MOSI   OUT    | SPI_MOSI    (CA821x MOSI,   P23)
  15    | VDD33    SUP   | VDD33       SUP    | -
  16    | nRESET   IN    | nRESET      IN     | -
  17    | PF.0     IN    | ICE_DAT     I/O    | TMS
  18    | PF.1     IN    | ICE_CLK     IN     | TCK
  19    | PC.1     IN    | PC.1        OUT    | ZIG_RESET   (CA821x NRESET, P27)
  20    | PC.0     IN    | PC.0        IN     | ZIG_IRQB    (CA821x NIRQ,   P20)
  21    | PA.12    IN    | PA.12       IN     | -
  22    | PA.13    IN    | PA.13       I/O    | Connector
  23    | PA.14    IN    | PA.14       I/O    | Connector
  24    | PA.15    IN    | PA.15       IN     | Connector
  25    | VSS      SUP   | VSS         SUP    | -
  26    | VSW      SUP   | VSW         SUP    | -
  27    | VDD      SUP   | VDD         SUP    | -
  28    | LDO_CAP  SUP   | LDO_CAP     SUP    | -
  29    | PB.14    IN    | PB.14       IN     | ZIG_APP     (CA821x DIG3,   P26, GPIO9)
  30    | PB.13    IN    | UART0_TXD   OUT    | UART TxD; Only when USE_UART defined
  31    | PB.12    IN    | UART0_RXD   IN     | UART RxD; Only when USE_UART defined
  32    | AVDD     SUP   | AVDD        SUP    | -
### Pin Configuration for NuMaker-PFM-M2351 development board (port usage only)
 Default Function  | Interface Function | Usage
 :------------- | :----------------- | :-----------------------------
 PH.9     IN    | PH.9        OUT    | SPI_CS      (CA821x SSB,    P22)
 PH.8     IN    | SPI1_SCLK   OUT    | SPI_CLK     (CA821x SCLK,   P21)
 PE.1     IN    | SPI1_MISO   IN     | SPI_MISO    (CA821x MISO,   P24)
 PE.0     IN    | SPI1_MOSI   OUT    | SPI_MOSI    (CA821x MOSI,   P23)
 PC.10    IN    | PC.10       OUT    | ZIG_RESET   (CA821x NRESET, P27)
 PC.9     IN    | PC.9        IN     | ZIG_IRQB    (CA821x NIRQ,   P20)

### Interrupts
Several interrupts are used in this project with different priorities. Interrupts with a higher priority (smaller value) will interrupt lower priority ISRs. The interrupts and their priorities are as follows:

| Interrupt  | Priority |
| ---------- | -------- |
| TMR0_IRQn  | 0        |
| TMR1_IRQn  | 0        |
| TMR2_IRQn  | 0        |
| TMR3_IRQn  | 0        |
| USBD_IRQn  | 2        |
| GPB_IRQn   | 1        |
| GPC_IRQn   | 1        |
| UART0_IRQn | 2        |
