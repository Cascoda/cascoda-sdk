#ifndef CASCODA_INTERFACE_H
#define CASCODA_INTERFACE_H

#include <stdbool.h>
#include <stdint.h>

#include "cascoda-bm/cascoda_bm.h"
#include "cascoda-bm/cascoda_types.h"
#include "ca821x_api.h"

/****** Global Variables defined in cascoda_bsp_*.c                      ******/

/** LED Mode Definitions */
enum LEDMode
{
	LED_M_CLRERROR             = 0, //!< Clear System Error
	LED_M_SETERROR             = 1, //!< Set System Error
	LED_M_CONNECTED_BAT_FULL   = 2, //!< Connected, Battery full or not present
	LED_M_CONNECTED_BAT_CHRG   = 3, //!< Connected, Battery charging
	LED_M_DEVICE_NOTASSOCIATED = 4, //!< Sensing Device, not conected to network
	LED_M_DEVICE_ASSOCIATED    = 5, //!< Sensing Device, associated to network
	LED_M_CLRALL               = 6, //!< Clear all LEDs
	LED_M_TEST_REF             = 7, //!< Test Mode, Reference Device
	LED_M_TEST_PASS            = 8, //!< Test Mode, Pass
	LED_M_ALLON                = 9  //!< both LEDs on
};

/** Description of the internal flash */
struct FlashInfo
{
	u32_t pageSize; //!< Size of each flash page (in bytes)
	u8_t  numPages; //!< Number of flash pages that make up the user flash region
};

//BSP should fill this in with correct values
extern const struct FlashInfo BSP_FlashInfo;

//BSP should call these upon certain events if they are populated
extern int (*button1_pressed)(void);     //!< Called when the  user button is pressed
extern int (*usb_present_changed)(void); //!< Called when the USB power is connected/disconnected

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
 *
 */
void BSP_PowerDown(u32_t sleeptime_ms, u8_t use_timer0);

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

/*******************************************************************************/
/** **OPTIONAL Function Declarations for cascoda_bsp_*.c                       */
/** (make stubs & return 0 if unused)                                          */
/*******************************************************************************/

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
 * \brief LED Signaling Mode
 *
 * \param mode - Enumerated mode
 *
 */
void BSP_LEDSigMode(u8_t mode);

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
 * \brief Sense Switch (Port PA.9) for Test Mode
 *
 * \return State of PA.9
 *
 */
u8_t BSP_GPIOSenseSwitch(void);

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

#if defined(USE_DEBUG)
// Debug globals
extern u8_t Debug_IRQ_State; //!<  IRQ State (wakeup, occasional or invalid)
extern u8_t Debug_BSP_Error; //!<  BSP Error Status
void        BSP_Debug_Reset(void);
void        BSP_Debug_Error(u8_t code);
#endif

#endif // CASCODA_BSP_H
