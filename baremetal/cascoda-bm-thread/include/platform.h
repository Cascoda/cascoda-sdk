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
 * Declarations of platform helper functions for Thread
 */
/**
 * @ingroup bm-thread
 * @defgroup bm-thread-helper Thread helper functions for baremetal
 * @brief  Platform helper functions for OpenThread
 *
 * @{
 */

#ifndef CA821X_OPENTHREAD_PLATFORM_PLATFORM_H_
#define CA821X_OPENTHREAD_PLATFORM_PLATFORM_H_

#include <stdint.h>
#include "openthread/instance.h"
#include "ca821x_api.h"

extern otInstance *OT_INSTANCE;

#define SuccessOrExit(aCondition) \
	do                            \
	{                             \
		if ((aCondition) != 0)    \
		{                         \
			goto exit;            \
		}                         \
	} while (0)

enum openthread_message_codes
{
	OT_SERIAL_DOWNLINK = 0xB2,
	OT_SERIAL_UPLINK   = 0xB3,
};

/** Data structure for vectored I/O using otPlatSettingsAddVector */
struct settingBuffer
{
	const uint8_t *value;  //!< Pointer to the data to write to settings
	uint16_t       length; //!< Length of the data to write to settings
};

/** Flash settings key for joiner credential */
static const uint16_t joiner_credential_key = 0xCA50;
/** Flash settings key for autostart */
static const uint16_t autostart_key = 0xCA51;
/** Flash settings key for sensordemo mode */
static const uint16_t sensordemo_key = 0xCA5C;
/** Flash settings key for actuatordemo mode */
static const uint16_t actuatordemo_key = 0xCA5D;
/** Flash settings key for the stack profiler */
static const uint16_t stack_profiler_key = 0xCA5E;
/** Flash settings key used for storing OCF data */
static const uint16_t OC_SETTINGS_KEY = 0xe107;
/** Flash settings key used for storing OCF encryption private key */
static const uint16_t OC_ENCRYPTION_KEY_KEY = 0xe108;

/**
 * Following Initialisation, this can be used to obtain the pDeviceRef that
 * openthread is using.
 *
 * @retval The pDeviceRef that openthread is using
 */
struct ca821x_dev *PlatformGetDeviceRef(void);

/**
 * Initialise the openthread platform layer with the internal device struct.
 *
 * Either this or PlatformRadioInitWithDev must be used before openthread will
 * be functional.
 *
 * @retval zero on success, nonzero on failure
 */
int PlatformRadioInit(void);

/**
 * Initialise the openthread platform layer with apDeviceRef as the device.
 *
 * Either this or PlatformRadioInit must be used before openthread will
 * be functional.
 *
 * @param pDeviceRef Pointer to initialised ca821x_device_ref struct
 *
 * @retval zero on success, nonzero on failure
 */
int PlatformRadioInitWithDev(struct ca821x_dev *pDeviceRef);

/**
 * Determines whether or not an MCPS-DATA-INDICATION is currently expected,
 * based on the result of the previous poll.
 *
 * This is used by the platform to determine whether or not sleeping is a good
 * idea.
 *
 * @retval 1 if an indication is expected, 0 if an indication is not expected.
 */
int PlatformIsExpectingIndication(void);

/**
 * Stop and reset the CA-821x. This will prevent the MAC from responding to ACKs
 * and command frames as well as wiping the PIB.
 */
void PlatformRadioStop(void);

/**
 * Initialise the platform alarm subsystem.
 */
void PlatformAlarmInit(void);

/**
 * Sends the platform and cax to sleep for the given number of milliseconds.
 *
 * @param aSleepTime The time in milliseconds to go to sleep for
 *
 * @retval OT_ERROR_NONE upon success
 */
otError PlatformSleep(uint32_t aSleepTime);

/**
 * Check whether the platform is able to sleep.
 *
 * @param aInstance The OpenThread instance
 * @return true if platform can sleep, false if busy.
 */
bool PlatformCanSleep(otInstance *aInstance);

/**
 * Handle received serial data
 *
 * @param aBuf Pointer to the received data payload
 * @param aBufLength The length of the received data payload
 *
 * @retval OT_ERROR_NONE upon success
 */
otError PlatformUartReceive(const uint8_t *aBuf, uint16_t aBufLength);

/**
 * Helper function to attempt the Thread joining process. The state of the Thread IPv6 interface
 * (otIp6SetEnabled) upon returning from this function depends on the return code. Upon a success
 * (OT_ERROR_ALREADY or OT_ERROR_NONE), the interface is left up, anticipating further radio activity.
 * Otherwise, it is brought down, anticipating being retried after a wait period.
 *
 * @param pDeviceRef Pointer to initialised ca821x_device_ref struct
 * @param aInstance The openthread instance
 *
 * @return Status of the join
 * @retval OT_ERROR_NONE Successfully joined Thread Network (Thread IPv6 interface is left up)
 * @retval OT_ERROR_ALREADY Device was already joined to Thread network (can leave with otInstanceFactoryReset) (Thread IPv6 interface is left up)
 * @retval (other) Failed to join a Thread Network for the given reason. (Thread IPv6 interface is brought down)
 */
otError PlatformTryJoin(struct ca821x_dev *pDeviceRef, otInstance *aInstance);

/**
 * Helper function to print the Thread Joiner credentials (for instance, upon boot).
 *
 * This function prints the Thread Joiner credentials using printf. It also handles leaving the
 * interface active in the case of USB, where going to sleep will kill the USB interface and prevent
 * the information from being read. This function will not print the credentials if the device is
 * already commissioned.
 *
 * @param pDeviceRef Pointer to initialised ca821x_device_ref struct
 * @param aInstance The openthread instance
 * @param aMaxWaitMs The maximum time to stay awake to allow credentials to be read (USB only). Only required for devices that sleep.
 */
otError PlatformPrintJoinerCredentials(struct ca821x_dev *pDeviceRef, otInstance *aInstance, uint32_t aMaxWaitMs);

/**
 * Helper function to get the joiner credential of this device.
 *
 * @param aInstance The openthread instance
 * @return The joiner credential as a null-terminated string
 */
const char *PlatformGetJoinerCredential(otInstance *aInstance);

/**
 * @brief Get the address at which a setting is stored, so that it can be read without
 * copying it, as would be necessary when using @ref otPlatSettingsGet().
 *
 *  @param[in]     aKey          The key associated with the requested setting.
 *  @param[in]     aIndex        The index of the specific item to get.
 *  @param[out]    aValue        A pointer to where the address of the setting should be written. May be set to NULL if
 *                               just testing for the presence or length of a setting.
 *  @param[inout]  aValueLength  A pointer to the length of the value. At return, will be overwritten with the length
 * 	of the value.
 *
 *  Note that the pointer written to aValue is not valid after a call to any other otPlatSettings* function, so
 * it must be used immediately.
 *
 *  @retval OT_ERROR_NONE             The given setting was found and fetched successfully.
 *  @retval OT_ERROR_NOT_FOUND        The given setting was not found in the setting store.
 *  @retval OT_ERROR_NOT_IMPLEMENTED  This function is not implemented on this platform.
 */
otError otPlatSettingsGetAddress(uint16_t aKey, int aIndex, void **aValue, uint16_t *aValueLength);

/** This function adds the value to a setting
 *  identified by aKey, without replacing any existing
 *  values.
 *
 *  This function differs from otPlatSettingsAdd in that it
 *  takes a vector of buffers instead of a single one. Use this
 *  function if you must write several non-contiguous buffers into a single
 *  setting without copying them into contiguous memory first.
 *
 *  Note that the underlying implementation is not required
 *  to maintain the order of the items associated with a
 *  specific key. The added value may be added to the end,
 *  the beginning, or even somewhere in the middle. The order
 *  of any pre-existing values may also change.
 *
 *  Calling this function successfully may cause unrelated
 *  settings with multiple values to be reordered.
 *
 * @param[in]  aInstance     The OpenThread instance structure.
 * @param[in]  aKey          The key associated with the setting to change.
 * @param[in]  aVector       The array of buffers that will be written to the setting.
 * @param[in]  aCount        The length of the aVector array.
 *
 * @retval OT_ERROR_NONE             The given setting was added or staged to be added.
 * @retval OT_ERROR_NOT_IMPLEMENTED  This function is not implemented on this platform.
 * @retval OT_ERROR_NO_BUFS          No space remaining to store the given setting.
 */
otError otPlatSettingsAddVector(otInstance *aInstance, uint16_t aKey, struct settingBuffer *aVector, size_t aCount);

#endif /* CA821X_OPENTHREAD_PLATFORM_PLATFORM_H_ */

/**
 * @}
 */
