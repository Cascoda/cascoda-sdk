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
 * @retval zero on success, nonzero on failure
 */
int PlatformRadioInitWithDev(struct ca821x_dev *apDeviceRef);

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
 * Process updates for the alarm subsystem.
 *
 * @param aInstance A pointer to the ot instance.
 */
void PlatformAlarmProcess(otInstance *aInstance);

/**
 * Get the remaining time, in ms, before the alarm is due
 * to trigger an event.
 *
 * @retval The remaining time, in ms, before the alarm expires.
 */
uint32_t PlatformGetAlarmMilliTimeout(void);

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

#endif /* CA821X_OPENTHREAD_PLATFORM_PLATFORM_H_ */
