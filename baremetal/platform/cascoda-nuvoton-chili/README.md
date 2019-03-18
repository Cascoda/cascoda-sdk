# cascoda-nuvoton-chili
Platform abstraction for the Chili 1 module.<br>
Module: Chili 1 (Versions 1.2 and 1.3)<br>
MCU:    Nuvoton Nano120 (NANO120LE3BN)<br>

## Directory structure
#### port
Platform-specific code linking cascoda-bm-driver to vendor libraries.

| File | Description |
| :--- | :--- |
|cascoda_bsp_chili.c | contains interface functions declared in cascoda-bm/cascoda_interface.h |
|cascoda_isr_chili.c | contains Interrupt Service Routines (ISRs) |
|cascoda_chili.c | functions internal to port |
|cascoda_dataflash_nano120.c | non-volatile memory access functions for data storage |
|cascoda_usb_nano120.c | Device USB HID driver functions for communications with host (if USE_USB is defined) |

#### vendor
Libraries provided by the MCU manufacturer (Nuvoton).<br>
The required libraries (standard drivers, CMSIS) have been extracted unchanged from the following BSP version:<br>
Nuvoton Nano100B_Series_BSP_CMSIS_v3.03.000

## System configuration
### Pin Configuration
 Pin    | Default Function  | Chili 1 Function | Usage
 -----: | :------------- | :----------------- | :-----------------------------
   1    | PB.12    IN    | PB.12       OUT    | VOLTS_TEST; Battery Voltage Measurement Control
   4    | PA.11    IN    | PA.11       OUT    | ZIG_RESET (CA-821x NRESET, P27)
   5    | PA.10    IN    | PA.10       IN     | ZIG_IRQB (CA-821x NIRQ, P20)
   6    | PA.9     IN    | PA.9        IN     | SWITCH SW2
   7    | PA.8     IN    | PA.8        OUT    | BATT_ON (Version 1.2 or lower only)
   8    | PB.4     IN    | UART1_RX    IN     | UART RxD; Only when USE_UART defined
   9    | PB.5     IN    | UART1_TX    OUT    | UART TxD; Only when USE_UART defined
  17    | PB.0     IN    | SPI1_MOSI0  OUT    | SPI_MOSI (CA-821x MOSI, P23)
  18    | PB.1     IN    | SPI1_MISO0  IN     | SPI_MISO (CA-821x MISO, P24)
  19    | PB.2     IN    | SPI1_SCLK   OUT    | SPI_CLK (CA-821x SCLK, P21)
  20    | PB.3     IN    | PB.3        OUT    | SPI_CS (CA-821x SSB, P22)
  26    | PA.14    IN    | PA.14       OUT    | LED_R; Red LED Control
  27    | PA.13    IN    | PA.13       OUT    | LED_G; Green LED Control
  28    | PA.12    IN    | PA.12       IN     | CHARGE_STAT; Battery Charge Status
  29    | ICE_DAT  IN    | ICE_DAT     I/O    | TMS
  30    | ICE_CLK  IN    | ICE_CLK     IN     | TCK
  32    | PA.0     IN    | AD0         IN     | VOLTS; Battery Voltage Measurement ADC Input
  43    | PB.15    IN    | PB.15       IN     | USB_PRESENT; USB Supply Status (Version 1.2 or lower)
  41    | PC.7     IN    | PC.7        IN     | USB_PRESENT; USB Supply Status (Version 1.3 or higher)
  44    | XT1_IN   I/O   | XT1_IN      IN     | System Clock from CA-821x to Nano120
  45    | XT1_OUT  I/O   | XT1_OUT     I/O    | -

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
