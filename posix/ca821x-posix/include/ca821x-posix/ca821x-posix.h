#ifndef CA821X_POSIX_H
#define CA821X_POSIX_H 1

#include "ca821x-posix/ca821x-types.h"
#include "ca821x_api.h"

/**
 * Generic function to initialise an available ca821x device. This includes
 * initialisation of the api and an exchange. Use of these generic functions
 * over using a specific exchange allows more flexibility.
 *
 * Calling twice on the same pDeviceRef without a deinit produces undefined
 * behaviour.
 *
 * @param[in]   pDeviceRef   Device reference to be initialised. Must point to
 *                           allocated memory, but does not have to be
 *                           initialised. The memory is cleared and initialised
 *                           internally.
 *
 * @param[in]   errorHandler A function pointer to an error handling function.
 *                           This callback will be triggered in the event of an
 *                           unrecoverable error. The driver will make a best
 *                           effort to recover the ca821x, and call this callback
 *                           to reset the PiB to the correct state.
 *
 *                           This will be spawned from a seperate recovery
 *                           thread and can be used to reset the PiB to the
 *                           correct state. The recovery thread has special
 *                           properties and should only be used with sync
 *                           commands (eg. to reset pib). Any messages that
 *                           had been requested but not actually sent will be
 *                           sent after recovery. If a sync command was in
 *                           progress during the crash, it will be locked until
 *                           the recovery is complete, then completed afterwards.
 *
 * @returns 0 for success, -1 for error
 *
 */
int ca821x_util_init(struct ca821x_dev *pDeviceRef, ca821x_errorhandler errorHandler);

/**
 * Generic function to deinitialise an initialised ca821x device. This will
 * free any resources that were allocated by ca821x_util_init.
 *
 * Calling on an uninitialised pDeviceRef produces undefined behaviour.
 *
 * @param[in]   pDeviceRef   Device reference to be deinitialised.
 *
 * @returns 0 for success, -1 for error
 *
 */
void ca821x_util_deinit(struct ca821x_dev *pDeviceRef);

/**
 * Generic function to attempt a hard reset of the ca821x chip.
 *
 * Calling on an uninitialised pDeviceRef produces undefined behaviour.
 *
 * @param[in]   pDeviceRef   Device reference for device to be reset.
 *
 * @returns 0 for success, -1 for error
 *
 */
int ca821x_util_reset(struct ca821x_dev *pDeviceRef);

/**
 * Generic function to poll the receive queue and call callbacks for received
 * commands. This function should only be used if POSIX_ASYNC_DISPATCH has been
 * set to 0. If POSIX_ASYNC_DISPATCH is set to 1, the callbacks will be called
 * immediately and asynchronously from another thread.
 *
 * It is recommended that if the return value is nonzero, the function should be
 * called again.
 *
 * Calling on an uninitialised pDeviceRef produces undefined behaviour.
 *
 * @param[in]   pDeviceRef   Device reference for device to be reset.
 *
 * @returns Length of processed command, or 0 if the queue was empty.
 *
 */
#if !CA821X_ASYNC_CALLBACK
int ca821x_util_dispatch_poll(struct ca821x_dev *pDeviceRef);
#endif

/**
 * Registers the callback to call for any non-ca821x commands that are sent over
 * the interface. Commands are still limited to the ca821x format, and must
 * use a command ID that is not currently used by the ca821x-spi protocol.
 * Currently, 0xA8 is used for openthread commands.
 *
 * @param[in]  callback   Function pointer to an user-command-handling callback
 *
 * @returns 0 for success, -1 for error
 *
 */
int exchange_register_user_callback(exchange_user_callback callback, struct ca821x_dev *pDeviceRef);

/**
 * Sends a user-defined command over the connected interface. This is not useful for direct CA-821x
 * communication, as the api commands handle this. This can be used to implement custom commands
 * on the chili modules, when communicating over USB or UART, for example.
 *
 * Synchronous commands are not supported using this mechanism.
 *
 * @param[in]  cmdid   Command ID to be used by command
 * @param[in]  cmdlen  Length of the payload
 * @param[in]  payload  Pointer to a buffer containing the payload data with length cmdlen
 * @param[in]  pDeviceRef  The device reference to communicate with
 *
 * @returns ca_error
 *
 */
ca_error exchange_user_command(uint8_t cmdid, uint8_t cmdlen, uint8_t *payload, struct ca821x_dev *pDeviceRef);

#endif //CA821X_POSIX_H
