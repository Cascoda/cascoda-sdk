/*
 *  Copyright (c) 2019, Cascoda Ltd.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */
/**
 * @file
 * Declaration of platform abstraction functions
 */
/**
 * @ingroup bm-driver
 * @defgroup bm-interface Baremetal platform abstraction
 * @brief  Platform abstraction layer definition for baremetal, expands upon \ref bm-interface-core
 *
 * @{
 */

#ifndef CASCODA_INTERFACE_H
#define CASCODA_INTERFACE_H

#include <stdbool.h>
#include <stdint.h>

#include "cascoda-bm/cascoda_bm.h"
#include "cascoda-bm/cascoda_interface_core.h"
#include "ca821x_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/****** Enum system core clock frequency [MHz] ******/
typedef enum fsys_mhz
{
	FSYS_4MHZ  = 4,
	FSYS_12MHZ = 12,
	FSYS_16MHZ = 16,
	FSYS_24MHZ = 24,
	FSYS_32MHZ = 32,
	FSYS_48MHZ = 48,
	FSYS_64MHZ = 64
} fsys_mhz;

/****** Enum for wakeup condition after reset ******/
typedef enum wakeup_reason
{
	WAKEUP_POWERON        = 0, ///< power-on
	WAKEUP_DEEP_POWERDOWN = 1, ///< deep-power-down (DPD) with no data retention
	WAKEUP_WATCHDOG       = 2, ///< Watchdog Timeout
	WAKEUP_HARDFAULT      = 3, ///< Hardfault
	WAKEUP_SYSRESET       = 4, ///< System Reset
	WAKEUP_RTCALARM       = 5, ///< RTC Alarm
} wakeup_reason;

/****** Enum for system reset mode ******/
typedef enum sysreset_mode
{
	SYSRESET_APROM = 0, //!< Systemreset normally, into application flash
	SYSRESET_DFU   = 1, //!< System reset into Device Firmware Update mode
} sysreset_mode;

/****** Enums for module pin handling                                 ******/
/** Pin Type */
typedef enum module_pin_dir
{
	MODULE_PIN_DIR_IN  = 0, //!< input
	MODULE_PIN_DIR_OUT = 1  //!< output
} module_pin_dir;

/** Pin Pull-Up */
typedef enum module_pin_pullup
{
	MODULE_PIN_PULLUP_OFF = 0, //!< pull-up off
	MODULE_PIN_PULLUP_ON  = 1  //!< pull-up on
} module_pin_pullup;

/** Pin Debounce */
typedef enum module_pin_debounce
{
	MODULE_PIN_DEBOUNCE_OFF = 0, //!< debounce off
	MODULE_PIN_DEBOUNCE_ON  = 1  //!< debounce on
} module_pin_debounce;

/** Pin is LED? */
typedef enum module_pin_type
{
	MODULE_PIN_TYPE_GENERIC = 0, //!< pin not attached to LED
	MODULE_PIN_TYPE_LED     = 1  //!< pin is attached to LED
} module_pin_type;

/** Pin Interrupt */
typedef enum module_pin_irq
{
	MODULE_PIN_IRQ_OFF  = 0, //!< irq off
	MODULE_PIN_IRQ_FALL = 1, //!< irq falling edge
	MODULE_PIN_IRQ_RISE = 2, //!< irq rising  edge
	MODULE_PIN_IRQ_BOTH = 3  //!< irq both edges
} module_pin_irq;

enum module_pin_not_available
{
	P_NA = 255 //!< Pin/Port functionality not available
};

/**
 * @brief Arguments for the BSP_ModuleRegisterGPIOInput function.
 *
 * Passing the arguments through a structure is necessary because GCC TrustZone
 * support is very primitive. Because there are too many arguments to pass them
 * through registers, GCC refuses to compile the sensible way of doing this.
 */
struct gpio_input_args
{
	u8_t                mpin;     ///< Number of the pin
	module_pin_pullup   pullup;   ///< Whether to use a pullup or not
	module_pin_debounce debounce; ///< Whether to use debouncing
	module_pin_irq      irq;      ///< Which edge, if any, to trigger an interrupt on
	int (*callback)(void);        ///< Callback called when an interrupt is triggered by this pin
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
	u32_t dataFlashBaseAddr; //!< Base address of the dataflash
	u16_t pageSize;          //!< Size of each flash page (in bytes)
	u8_t  numPages;          //!< Number of flash pages that make up the user flash region
};

// Type for a function pointer that points to DISPATCH_ReadCA821x
typedef void (*dispatch_read_t)(struct ca821x_dev *pDeviceRef);

/** Interface Structure for RTC Date and Time */
struct RTCDateAndTime
{
	u32_t year;  //!< Year (i.e. 2019)
	u32_t month; //!< Month (1-12)
	u32_t day;   //!< Day (1-31)
	u32_t hour;  //!< Hour (0-23)
	u32_t min;   //!< Minutes (0-59)
	u32_t sec;   //!< Seconds (0-59)
};

#if CASCODA_EXTERNAL_FLASHCHIP_PRESENT
/****************************************************/
/****** Variables for the external flash chip. ******/
/****************************************************/

/** Description of the external flash */
struct ExternalFlashInfo
{
	u32_t baseAddress;    //!< Base address of the dataflash
	u32_t readWriteLimit; //!< Maximum number of bytes that can be read/written in a single instruction
	u16_t pageSize;       //!< Size of each flash page (in bytes)
};

/** Function pointer used to schedule callbacks for the next higher layer */
typedef ca_error (*ExternalFlashCallback)(void *aContext);

/** Structure for generic status information of the external flash chip */
struct ExternalFlashStatus
{
	// Add more members to this structure in the future, as needed.
	bool writeEnabled : 1; //!< Write enabled status
};

/** Types of partial erase */
typedef enum ExternalFlashPartialEraseType
{
	SECTOR_4KB = 0,
	BLOCK_32KB,
	BLOCK_64KB
} ExternalFlashPartialEraseType;
#endif

/*******************************************************************************/
/****** REQUIRED Function Declarations for cascoda_bsp_*.c                ******/
/*******************************************************************************/

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
 * \param BufferSize - Max. Characters to Read, 0 to cancel read.
 *
 * \return Number of Characters placed in Buffer
 *
 */
u32_t BSP_SerialRead(u8_t *pBuffer, u32_t BufferSize);
#endif // USE_UART

/**
 * \brief Set up wake on timer/IRQ, Power down MCU and return on wakeup
 *
 * \param sleeptime_ms - sleep time [milliseconds]
 * \param use_timer0 - flag if to use timer0 (1) or not (0), 0 to wake on IRQ
 * \param dpd - flag if to enter deep-power-down without data retention
 * \param pDeviceRef - Pointer to initialised ca821x_device_ref struct
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
 * \param useExternalClock - (0: Use internal clock) (1: Use the clock from CA821x)
 *
 */
void BSP_UseExternalClock(u8_t useExternalClock);

/**
 * Get the struct of special pins for the platform
 * @return Struct of special function pins for use with the Module system
 */
struct ModuleSpecialPins BSP_GetModuleSpecialPins(void);

/**
 * \brief Registers GPIO Functionality for Module Pin
 * \param args Arguments, see gpio_input_args
 * \return status
 *
 */
ca_error BSP_ModuleRegisterGPIOInput(struct gpio_input_args *args);

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
 * 
 * Output pins must be registered before they can be used. See BSP_ModuleRegisterGPIOOutput().
 *
 * \param mpin  - module pin
 * \param val - output value
 * \return status
 *
 */
ca_error BSP_ModuleSetGPIOPin(u8_t mpin, u8_t val);

/**
 * \brief Senses GPIO Input Value of Module Pin
 *
 * Input pins must be registered before they can be used. See BSP_ModuleRegisterGPIOInput().
 *
 * \param mpin  - module pin
 * \param val - value read
 * \return status
 *
 */
ca_error BSP_ModuleSenseGPIOPin(u8_t mpin, u8_t *val);

/**
 * \brief Reads ADC Conversion Value on Module Pin
 *
 * Pins must NOT be registered if used with this function.
 *
 * \param mpin  - module pin
 * \param val - 32-bit conversion value (12-bit resolution ADC: 0-4095)
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
 * \param resetMode The mode of reset to use
 */
void BSP_SystemReset(sysreset_mode resetMode);

/**
 * \brief Set the default boot mode for the device
 * @param bootMode The default mode to boot into
 * @return ca_error code, CA_ERROR_SUCCESS if successful
 */
ca_error BSP_SetBootMode(sysreset_mode bootMode);

/**
 * \brief Get a 64-bit ID that is unique to this device
 */
u64_t BSP_GetUniqueId(void);

/**
 * \brief Get a string that describes this device
 * \return A string that describes this device (e.g. "Chili2")
 */
const char *BSP_GetPlatString(void);

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
 * \return Temperature in tenths of a degree (celsius)
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
 */
void BSP_WatchdogReset(void);

/**
 * \brief Watchdog Disable
 *
 */
void BSP_WatchdogDisable(void);

/**
 * \brief Checks if the watchdog has been triggered, clears the warning if so
 *
  * \retval boolean, 1: triggered, 0: Not triggered
 *
 */
u8_t BSP_IsWatchdogTriggered(void);

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
 * Get the flashinfo struct for the current platform.
 * @param aFlashInfoOut Pointer to struct to fill with flashinfo
 */
void BSP_GetFlashInfo(struct FlashInfo *aFlashInfoOut);

/**
 * \brief Writes Dataflash Memory, relies on Memory erased
 *
 * startaddr is absolute, see the dataFlashBaseAddr of the FlashInfo struct for correct offset.
 *
 * \param startaddr - byte address (divisible by 4 (word))
 * \param data -      pointer to data
 * \param datasize -  size of data (in bytes)
 * @retval CA_ERROR_SUCCESS       Flash erased successfully
 * @retval CA_ERROR_INVALID_ARGS  Invalid arguments/flash range
 */
ca_error BSP_FlashWriteInitial(u32_t startaddr, void *data, u32_t datasize);

/**
 * \brief Erases a Dataflash Memory page (All words in page set to 0xFFFFFFFF)
 *
 * startaddr is absolute, see the dataFlashBaseAddr of the FlashInfo struct for correct offset.
 * Using any address in a given page will erase that entire page.
 *
 * \param startaddr - byte address (divisible by 4 (word))
 * @retval CA_ERROR_SUCCESS       Flash erased successfully
 * @retval CA_ERROR_INVALID_ARGS  Invalid arguments/flash range
 */
ca_error BSP_FlashErase(u32_t startaddr);

/**
 * \brief Reads Dataflash Memory
 *
 * startaddr is absolute, see the dataFlashBaseAddr of the FlashInfo struct for correct offset.
 *
 * \param startaddr - byte address (divisible by 4 (word))
 * \param data -      pointer to data (words)
 * \param datasize -  size of data (in words)
 * @retval CA_ERROR_SUCCESS       Flash read into buffer successfully
 * @retval CA_ERROR_INVALID_ARGS  Invalid arguments/flash range
 */
ca_error BSP_FlashRead(u32_t startaddr, u32_t *data, u32_t datasize);

/**
 * Check that a given range of flash matches the provided CRC32 checksum.
 *
 * @param startaddr The start address to check (must be page-aligned)
 * @param checklen  The number of bytes to check (must be page-aligned)
 * @param crc32     The CRC32 checksum to verify against
 * @return cascoda status
 * @retval CA_ERROR_SUCCESS       CRC32 matches the data in flash
 * @retval CA_ERROR_INVALID_ARGS  Invalid arguments/flash range
 * @retval CA_ERROR_FAIL          CRC32 value does not match the data in flash range
 */
ca_error BSP_FlashCheck(u32_t startaddr, u32_t checklen, u32_t crc32);

/**
 * \brief Initialises RTC
 *
 */
void BSP_RTCInitialise(void);

/**
 * \brief Sets RTC Alarm in seconds from current time
 * \param seconds - seconds to be added to current time
 * \return status
 *
 */
ca_error BSP_RTCSetAlarmSeconds(u32_t seconds);

/**
 * \brief Disables RTC Alarm
 *
 */
void BSP_RTCDisableAlarm(void);

/**
 * \brief Sets RTC Date+Time
 * \param dateandtime - RTCDateAndTime structure
 * \return status
 *
 */
ca_error BSP_RTCSetDateAndTime(struct RTCDateAndTime dateandtime);

/**
 * \brief Gets RTC Date+Time
 * \param dateandtime - pointer to RTCDateAndTime structure
 * \return status
 *
 */
void BSP_RTCGetDateAndTime(struct RTCDateAndTime *dateandtime);

/**
 * \brief Converts Unix Time seconds to RTC Date+Time
 * \param seconds - Unix time seconds
 * \param dateandtime - pointer to RTCDateAndTime structure
 *
 */
void BSP_RTCConvertSecondsToDateAndTime(i64_t seconds, struct RTCDateAndTime *dateandtime);

/**
 * \brief Converts RTC Date+Time to Unix Time seconds
 * \param dateandtime - RTCDateAndTime structure
 * \return Unix time seconds
 *
 */
i64_t BSP_RTCConvertDateAndTimeToSeconds(const struct RTCDateAndTime *dateandtime);

/**
 * \brief Registers RTC IRQ function callback
 * \param callback - pointer to ISR for input
 *
 */
void BSP_RTCRegisterCallback(int (*callback)(void));

#if CASCODA_EXTERNAL_FLASHCHIP_PRESENT
/**
 * Initialise the external flash.
 * Make sure to call this function directly (not through BSP_ExternalFlashScheduleCallback)
 * at the initialisation stage of any application that uses the external flash chip.
 *
 * See "baremetal/app/external-flash-bm/flash_test_main.c" for an example on how
 * this is used.
 */
void BSP_ExternalFlashInit(void);

/**
 * Program the external flash.
 *
 * This function should only be called within a higher layer application function
 * which is scheduled by BSP_ExternalFlashScheduleCallback(). Otherwise,
 * it will fail with CA_ERROR_NO_ACCESS.
 *
 * See "baremetal/app/external-flash-bm/flash_test_main.c" for an example on how
 * this is used.
 *
 * @param aStartAddress The start address of the page program instruction.
 *                      Should be between 0x00000 and 0xFFFFF
 * @param aNumOfBytes   The number of bytes to be sequentially programmed, starting from aStartAddress.
 *                      Should be between 1 and 128 bytes.
 * @param aTxData       The data to program
 *
 * @return Status of the command
 * @retval CA_ERROR_SUCCESS      Success
 * @retval CA_ERROR_NO_ACCESS    This function wasn't called from a scheduled higher layer callback.
 * @retval CA_ERROR_INVALID_ARGS aStartAddress and/or aNumOfBytes are invalid.
 */
ca_error BSP_ExternalFlashProgram(uint32_t aStartAddress, uint8_t aNumOfBytes, uint8_t *aTxData);

/**
 * Read up to 128 bytes into aRxData sequentially from the external flash, starting at aStartAddress.
 *
 * This function should only be called within a higher layer application function
 * which is scheduled by BSP_ExternalFlashScheduleCallback(). Otherwise,
 * it will fail with CA_ERROR_NO_ACCESS.
 *
 * See "baremetal/app/external-flash-bm/flash_test_main.c" for an example on how
 * this is used.
 *
 * @param aStartAddress The start address of the read data instruction
 * @param aNumOfBytes   The number of bytes to be sequentially read, starting from aStartAddress.
 * 						This value should be between 1 and 128.
 * @param aRxData       Stores the data read from the external flash.
 *
 * @return Status of the command
 * @retval CA_ERROR_SUCCESS      Success
 * @retval CA_ERROR_NO_ACCESS    This function wasn't called from a scheduled higher layer callback.
 * @retval CA_ERROR_INVALID_ARGS aNumOfBytes isn't between 1 and 128, or is incompatible with aStartAddress.
 */
ca_error BSP_ExternalFlashReadData(uint32_t aStartAddress, uint8_t aNumOfBytes, uint8_t *aRxData);

/**
 * Erase (i.e. set to all 1s) all memory within a 4KB sector, 32KB block, or 64KB block.
 *
 * This function should only be called within a higher layer application function
 * which is scheduled by BSP_ExternalFlashScheduleCallback(). Otherwise,
 * it will fail with CA_ERROR_NO_ACCESS.
 *
 * See "baremetal/app/external-flash-bm/flash_test_main.c" for an example on how
 * this is used.
 *
 * @param aEraseType The type of partial erase that will be performed (4KB, 32KB, or 64KB)
 * @param aAddress   The address contained in the sector or block that will be erased
 *
 * @return Status of the command
 * @retval CA_ERROR_SUCCESS      Success
 * @retval CA_ERROR_NO_ACCESS    This function wasn't called from a scheduled higher layer callback.
 * @retval CA_ERROR_INVALID_ARGS aAddress is greater than the highest existing memory address in the flash.
 */
ca_error BSP_ExternalFlashPartialErase(ExternalFlashPartialEraseType aEraseType, uint32_t aAddress);

/**
 * Erase (i.e. set to all 1s) all the memory of the external flash chip.
 *
 * This function should only be called within a higher layer application function
 * which is scheduled by BSP_ExternalFlashScheduleCallback(). Otherwise,
 * it will fail with CA_ERROR_NO_ACCESS.
 *
 * See "baremetal/app/external-flash-bm/flash_test_main.c" for an example on how
 * this is used.
 *
 * @return Status of the command
 * @retval CA_ERROR_SUCCESS   Success
 * @retval CA_ERROR_NO_ACCESS This function wasn't called from a scheduled higher layer callback.
 */
ca_error BSP_ExternalFlashChipErase(void);

/**
 * Get the ID of the external flash chip.
 *
 * This function should only be called within a higher layer application function
 * which is scheduled by BSP_ExternalFlashScheduleCallback(). Otherwise,
 * it will fail with CA_ERROR_NO_ACCESS.
 *
 * See "baremetal/app/external-flash-bm/flash_test_main.c" for an example on how
 * this is used.
 *
 * @param aId Stores the ID of the external flash chip
 *
 * @return Status of the command
 * @retval CA_ERROR_SUCCESS   Success
 * @retval CA_ERROR_NO_ACCESS This function wasn't called from a scheduled higher layer callback.
 */
ca_error BSP_ExternalFlashGetDeviceId(uint8_t *aId);

/**
 * Get status information about the external flash chip.
 *
 * This function should only be called within a higher layer application function
 * which is scheduled by BSP_ExternalFlashScheduleCallback(). Otherwise,
 * it will fail with CA_ERROR_NO_ACCESS.
 *
 * See "baremetal/app/external-flash-bm/flash_test_main.c" for an example on how
 * this is used.
 *
 * @param status Stores the status information.
 *
 * @return Status of the command
 * @retval CA_ERROR_SUCCESS   Success
 * @retval CA_ERROR_NO_ACCESS This function wasn't called from a scheduled higher layer callback.
 */
ca_error BSP_ExternalFlashGetStatus(struct ExternalFlashStatus *status);

/**
 * Schedule a callback for the higher layer, which will be automatically be executed as soon as possible.
 * The callback should be a pointer to a function of the higher layer, and not a pointer to a BSP_ExternalFlash*() function.
 *
 * @param aCallback The callback to be scheduled
 * @param aContext  Context pointer to be used by the scheduled callback
 *
 * @return Status of the command
 * @retval CA_ERROR_SUCCESS Success
 * @retval CA_ERROR_ALREADY An instruction has already been scheduled (and hasn't been handled yet)
 */
ca_error BSP_ExternalFlashScheduleCallback(ExternalFlashCallback aCallback, void *aContext);

/**
 * Stores some useful information about the external flash chip in aFlashInfoOut.
 *
 * @param aFlashInfoOut Output parameter into which information about the external flash chip will be stored.
 */
void BSP_ExternalFlashGetInfo(struct ExternalFlashInfo *aFlashInfoOut);
#endif

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif // CASCODA_BSP_H
