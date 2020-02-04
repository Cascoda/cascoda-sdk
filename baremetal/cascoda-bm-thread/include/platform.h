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

/** Flash settings key for joiner credential */
static const uint16_t joiner_credential_key = 0xCA50;
/** Flash settings key for autostart */
static const uint16_t autostart_key = 0xCA51;
/** Flash settings key for sensordemo mode */
static const uint16_t sensordemo_key = 0xCA5C;
/** Flash settings key for actuatordemo mode */
static const uint16_t actuatordemo_key = 0xCA5D;

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
 * Handle received serial data
 *
 * @param aBuf Pointer to the received data payload
 * @param aBufLength The length of the received data payload
 *
 * @retval OT_ERROR_NONE upon success
 */
otError PlatformUartReceive(const uint8_t *aBuf, uint16_t aBufLength);

/**
 * Helper function to attempt the Thread joining process.
 *
 * @param pDeviceRef Pointer to initialised ca821x_device_ref struct
 * @param aInstance The openthread instance
 *
 * @return Status of the join
 * @retval OT_ERROR_NONE Successfully joined Thread Network
 * @retval OT_ERROR_ALREADY Device was already joined to Thread network (can leave with otInstanceFactoryReset)
 * @retval (other) Failed to join a Thread Network for the given reason.
 */
otError PlatformTryJoin(struct ca821x_dev *pDeviceRef, otInstance *aInstance);

/**
 * Helper function to get the joiner credential of this device.
 *
 * @param aInstance The openthread instance
 * @return The joiner credential as a null-terminated string
 */
const char *PlatformGetJoinerCredential(otInstance *aInstance);

#endif /* CA821X_OPENTHREAD_PLATFORM_PLATFORM_H_ */
