#ifndef CASCODA_INTERFACE_H
#define CASCODA_INTERFACE_H

#include <stdbool.h>
#include <stdint.h>

#include "cascoda-bm/cascoda_bm.h"
#include "ca821x_api.h"

/****** Enums system core clock frequency [MHz] ******/
typedef enum fsys_mhz
{
	FSYS_4MHZ  = 0,
	FSYS_12MHZ = 1,
	FSYS_16MHZ = 2,
	FSYS_24MHZ = 3,
	FSYS_32MHZ = 4,
	FSYS_48MHZ = 5,
	FSYS_64MHZ = 6
} fsys_mhz;

/****** Enums for wakeup condition after reset ******/
typedef enum wakeup_reason
{
	WAKEUP_POWERON        = 0, ///< power-on
	WAKEUP_DEEP_POWERDOWN = 1, ///< deep-power-down (DPD) with no data retention
	WAKEUP_WATCHDOG       = 2, ///< Watchdog Timeout
	WAKEUP_HARDFAULT      = 3, ///< Hardfault
	WAKEUP_SYSRESET       = 4, ///< System Reset
} wakeup_reason;

/****** Enums for module pin handling                                 ******/
typedef enum module_pin_dir /* Pin Type */
{
	MODULE_PIN_DIR_IN  = 0, /* input */
	MODULE_PIN_DIR_OUT = 1  /* output */
} module_pin_dir;

typedef enum module_pin_pullup /* Pin Pull-Up */
{
	MODULE_PIN_PULLUP_OFF = 0, /* pull-up off */
	MODULE_PIN_PULLUP_ON  = 1  /* pull-up on */
} module_pin_pullup;

typedef enum module_pin_debounce /* Pin Debounce */
{
	MODULE_PIN_DEBOUNCE_OFF = 0, /* debounce off */
	MODULE_PIN_DEBOUNCE_ON  = 1  /* debounce on */
} module_pin_debounce;

typedef enum module_pin_type /* Pin is LED? */
{
	MODULE_PIN_TYPE_GENERIC = 0, /* pin not attached to LED */
	MODULE_PIN_TYPE_LED     = 1  /* pin is attached to LED */
} module_pin_type;

typedef enum module_pin_irq /* Pin Interrupt */
{
	MODULE_PIN_IRQ_OFF  = 0, /* irq off */
	MODULE_PIN_IRQ_FALL = 1, /* irq falling edge */
	MODULE_PIN_IRQ_RISE = 2, /* irq rising  edge */
	MODULE_PIN_IRQ_BOTH = 3  /* irq both edges */
} module_pin_irq;

enum module_pin_not_available
{
	P_NA = 255 //!< Pin/Port functionality not available
};

/**********************************************************************/
/****** If LEDs are used they should be tied to VDD and pulled   ******/
/****** to Ground with a Resistor when on to avoid Dark Current. ******/
/**********************************************************************/
enum module_pin_led_set
{
	LED_ON  = 0,
	LED_OFF = 1,
};

struct ModuleSpecialPins
{
	u8_t SWITCH;
	u8_t LED_GREEN;
	u8_t LED_RED;
	u8_t USB_PRESENT;
};
//For forward compatibility, add a P_NA for every additional field in the above struct
#define MSP_DEFAULT P_NA, P_NA, P_NA, P_NA

/** Description of the internal flash */
struct FlashInfo
{
	u32_t pageSize; //!< Size of each flash page (in bytes)
	u8_t  numPages; //!< Number of flash pages that make up the user flash region
};

//BSP should fill these in with correct values
extern const struct FlashInfo         BSP_FlashInfo;
extern const struct ModuleSpecialPins BSP_ModuleSpecialPins;

/*******************************************************************************/
/****** REQUIRED Function Declarations for cascoda_bsp_*.c               ******/
/*******************************************************************************/

/**
 * \brief Wait for specified Time in Microseconds (max. 1000)
 *
 * \param us - Time in microseconds
 *
 */
void BSP_WaitUs(u32_t us);

/**
 * \brief Reset CAX RF Chip
 *
 * \param ms - Reset Low Time in [ms]
 *
 */
void BSP_ResetRF(u8_t ms);

/**
 * \brief Sense whether SPI IRQ is high or low
 *
 * \return State of IRQ Pin  PA.10
 *
 */
u8_t BSP_SenseRFIRQ(void);

/**
 * \brief Inhibit SPI IRQ, supresses interrupt but still latches it
 *
 */
void BSP_DisableRFIRQ(void);

/**
 * \brief Allow SPI IRQ, re-enabling interrupt after BSP_DisableRFIRQ()
 *
 */
void BSP_EnableRFIRQ(void);

/**
 * \brief Put SPI Select (SSB) Pin high
 *
 */
void BSP_SetRFSSBHigh(void);

/**
 * \brief Put SPI Select (SSB) Pin low
 *
 */
void BSP_SetRFSSBLow(void);

/**
 * \brief Enable the serial (usb/uart) irq
 *
 */
void BSP_EnableSerialIRQ(void);

/**
 * \brief Disable the serial (usb/uart) irq
 *
 */
void BSP_DisableSerialIRQ(void);

#if defined(USE_USB)

/**
 * \brief Write Buffer Fragment of Data to USB
 *
 * \param pBuffer - Pointer to Data Buffer
 *
 */
void BSP_USBSerialWrite(u8_t *pBuffer);

/**
 * \brief Peek a received USB buffer
 *
 * \return Pointer to Data Buffer or NULL if none available
 */
u8_t *BSP_USBSerialRxPeek(void);

/**
 * \brief Release a received USB buffer following processing
 */
void BSP_USBSerialRxDequeue(void);

#endif // USE_USB
#if defined(USE_UART)

/**
 * \brief Write Buffer of Data to Serial
 *
 * \param pBuffer - Pointer to Data Buffer
 * \param BufferSize - Size of Data Buffer
 *
 */
void BSP_SerialWriteAll(u8_t *pBuffer, u32_t BufferSize);

/**
 * \brief Read Character(s) from Serial
 *
 * \param pBuffer - Pointer to Data Buffer
 * \param BufferSize - Max. Characters to Read
 *
 * \return Number of Characters placed in Buffer
 *
 */
u32_t BSP_SerialRead(u8_t *pBuffer, u32_t BufferSize);
#endif // USE_UART

/**
 * \brief Initialise GPIO and SPI Pins for Comms with CA-821X
 *
 */
void BSP_SPIInit(void);

/**
 * \brief Transmit Byte to SPI and Receive Byte at same Time
 *
 * \param OutByte - Character to transmit
 *
 * \return Character received
 *
 * It is required that the Exchange & push/pop methods can work together,
 * so it is recommended that one is implemented with the other. The FIFO
 * behaviour is optimal, as it allows maximal usage of the SPI bus. Then
 * SPIExchangeByte can be implemented like:
 *
 * u8_t BSP_SPIExchangeByte( u8_t OutByte )
 * {
 *    u8_t InByte;
 *    while(!BSP_SPIPushByte(OutByte));
 *    while(!BSP_SPIPopByte(&InByte));
 *    return InByte;
 * }
 *
 * If the given board doesn't support FIFO behaviour, then the push/pop methods
 * can be implemented by using a virtual receive FIFO of length 1, and actually
 * doing the SPI byte exchange when PushByte is called.
 *
 */
u8_t BSP_SPIExchangeByte(u8_t OutByte);

/**
 * \brief Push byte to FIFO for transmitting over SPI
 *
 * \param OutByte - Character to transmit
 *
 * \return 1 if successful, 0 if the FIFO is full
 *
 */
u8_t BSP_SPIPushByte(u8_t OutByte);

/**
 * \brief Get Byte from SPI receive FIFO
 *
 * \param InByte - output, filled with the received byte if retval is 1
 *
 * \return 1 if successful, 0 if the FIFO is full
 *
 */
u8_t BSP_SPIPopByte(u8_t *InByte);

/**
 * \brief Set up wake on timer/IRQ, Power down MCU and return on wakeup
 *
 * \param sleeptime_ms - sleep time [milliseconds]
 * \param use_timer0 - flag if to use timer0 (1) or not (0), 0 to wake on IRQ
 * \param dpd - flag if to enter deep-power-down without data retention
 *
 */
void BSP_PowerDown(u32_t sleeptime_ms, u8_t use_timer0, u8_t dpd, struct ca821x_dev *pDeviceRef);

/**
 * \brief Initialise the system for a given ca821x_dev
 *
 */
void BSP_Initialise(struct ca821x_dev *pDeviceRef);

/**
 * \brief Enable or disable the usage of the external clock from the CA821x
 * \params useExternalClock - (0: Use internal clock) (1: Use the clock from CA821x)
 *
 */
void BSP_UseExternalClock(u8_t useExternalClock);

/**
 * \brief Registers GPIO Functionality for Module Pin
 * \param mpin  - module pin number
 * \param pullup - none/pullup/pulldown for input
 * \param debounce - enable debounce for input
 * \param irq - enable interupt for input
 * \param callback - pointer to ISR for input
 * \return status
 *
 */
ca_error BSP_ModuleRegisterGPIOInput(u8_t                mpin,
                                     module_pin_pullup   pullup,
                                     module_pin_debounce debounce,
                                     module_pin_irq      irq,
                                     int (*callback)(void));

/**
 * \brief Registers GPIO Functionality for Module Pin
 * \param mpin  - module pin number
 * \param isled - pin is attached to led (Used to prevent power down leakage)
 * \return status
 *
 */
ca_error BSP_ModuleRegisterGPIOOutput(u8_t mpin, module_pin_type isled);

/**
 * \brief Unregisters GPIO Functionality for Module Pin to Default Settings
 * \param mpin  - module pin number
 * \return status
 *
 */
ca_error BSP_ModuleDeregisterGPIOPin(u8_t mpin);

/**
 * \brief Checks if a Module Pin is already registered / used
 * \param mpin  - module pin number
 * \return status (0: no, 1: yes)
 *
 */
u8_t BSP_ModuleIsGPIOPinRegistered(u8_t mpin);

/**
 * \brief Sets Module Pin GPIO Output Value
 * \param mpin  - module pin
 * \param val - output value
 * \return status
 *
 */
ca_error BSP_ModuleSetGPIOPin(u8_t mpin, u8_t val);

/**
 * \brief Senses GPIO Input Value of Module Pin
 * \param mpin  - module pin
 * \param val - value read
 * \return status
 *
 */
ca_error BSP_ModuleSenseGPIOPin(u8_t mpin, u8_t *val);

/**
 * \brief Reads ADC Conversion Value on Module Pin
 * \param mpin  - module pin
 * \param val - 32-bit conversion value
 * \return status
 *
 */
ca_error BSP_ModuleReadVoltsPin(u8_t mpin, u32_t *val);

/*******************************************************************************/
/** **OPTIONAL Function Declarations for cascoda_bsp_*.c                       */
/** (make stubs & return 0 if unused)                                          */
/*******************************************************************************/

/**
 * \brief Reset the CPU using a soft-reset.
 *
 */
void BSP_SystemReset();

/**
 * \brief This function will be called repeatedly when the Baremetal drivers
 * are blocking & waiting (eg. TIME_WaitTicks or WAIT_Callback), in case the
 * BSP needs to do any system maintenance or wants to reduce power consumption.
 *
 */
void BSP_Waiting(void);

/**
 * \brief Get a 64-bit ID that is unique to this device
 */
u64_t BSP_GetUniqueId(void);

/**
 * \brief Get Microchip MCP73831 Charge Status
 *
 * \return Charging Status
 *
 */
u8_t BSP_GetChargeStat(void);

/**
 * \brief Measure and Read Temperature Value
 *
 * \return Temperature in tenths of a degree (celcius)
 *
 */
i32_t BSP_GetTemperature(void);

/**
 * \brief Measure and Read Battery Volts Value from ADC
 *
 * \return ADC output
 *
 */
u32_t BSP_ADCGetVolts(void);

/**
 * \brief Reason MCU has woken up after reset
 *
 * \return wakeup reason
 *
 */
wakeup_reason BSP_GetWakeupReason(void);

/**
 * \brief Watchdog Enable
 *
  * \param timeout_ms - timeout in milliseconds
 *
 */
void BSP_WatchdogEnable(u32_t timeout_ms);

/**
 * \brief Watchdog Reset and Restart
 *
  * \param timeout_ms - timeout in milliseconds
 *
 */
void BSP_WatchdogReset(u32_t timeout_ms);

/**
 * \brief Checks if the watchdog has been triggered, clears the warning if so
 *
  * \retval boolean, 1: triggered, 0: Not triggered
 *
 */
u8_t BSP_IsWatchdogTriggered(void);

/**
 * \brief Watchdog Disable
 *
 */
void BSP_WatchdogDisable(void);

/**
 * \brief Enable the USB if connected
 * \retval - 0 if successfully enabled, -1 if enable not possible (eg. not connected)
 *
 */
void BSP_EnableUSB(void);

/**
 * \brief Disable the USB
 *
 */
void BSP_DisableUSB(void);

/**
 * \brief Is the USB connected?
 * \retval returns 1 if connected, 0 if disconnected
 *
 */
u8_t BSP_IsUSBPresent(void);

/**
 * \brief re-configures system clock frequency and comms interface
 *
 * \param fsys -         system frequency [MHz], enum type fsys_mhz
 * \param enable_comms - boolean, 1: comms interface enabled 0: disabled
 *
 */
void BSP_SystemConfig(fsys_mhz fsys, u8_t enable_comms);

/**
 * \brief Writes Dataflash Memory, relies on Memory erased
 *
 * startaddr is relative to the start of userflash, so first address is 0.
 *
 * \param startaddr - byte address (divisible by 4 (word))
 * \param data -      pointer to data (words)
 * \param datasize -  size of data (in words)
 *
 */
void BSP_WriteDataFlashInitial(u32_t startaddr, u32_t *data, u32_t datasize);

/**
 * \brief Erases a Dataflash Memory page (All words in page set to 0xFFFFFFFF)
 *
 * startaddr is relative to the start of userflash, so first address is 0.
 * Using any address in a given page will erase that entire page.
 *
 * \param startaddr - byte address (divisible by 4 (word))
 *
 */
void BSP_EraseDataFlashPage(u32_t startaddr);

/**
 * \brief Reads Dataflash Memory
 *
 * startaddr is relative to the start of userflash, so first address is 0.
 *
 * \param startaddr - byte address (divisible by 4 (word))
 * \param data -      pointer to data (words)
 * \param datasize -  size of data (in words)
 *
 */
void BSP_ReadDataFlash(u32_t startaddr, u32_t *data, u32_t datasize);

/**
 * \brief Erases all Dataflash Memory
 *
 */
void BSP_ClearDataFlash(void);

#endif // CASCODA_BSP_H
