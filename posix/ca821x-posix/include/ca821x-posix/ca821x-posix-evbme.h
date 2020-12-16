/*
 * Copyright (c) 2020, Cascoda
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/**
 * @file
 * @brief EVBME host API commands
 */
/**
 * @ingroup posix
 * @defgroup posix-evbme Posix EVBME
 * @brief  Functions for communicating with the EVBME of a connected Chili platform
 *
 * @{
 */

#ifndef POSIX_CA821X_POSIX_INCLUDE_CA821X_POSIX_CA821X_POSIX_EVBME_H_
#define POSIX_CA821X_POSIX_INCLUDE_CA821X_POSIX_CA821X_POSIX_EVBME_H_

#include "ca821x_api.h"

#ifdef __cplusplus
extern "C" {
#endif

//Callback definitions ------------------------------------------------------------------------

typedef ca_error (*EVBME_Message_callback)(struct EVBME_Message *params, struct ca821x_dev *pDeviceRef);

struct EVBME_callbacks
{
	EVBME_Message_callback EVBME_MESSAGE_indication;
	EVBME_Message_callback EVBME_COMM_indication;
	EVBME_Message_callback EVBME_DFU_cmd;
};

/**
 * Get the mutable callback structure for the given device. Modify this to add EVBME callback handlers.
 * Do not modify the EVBME callback handlers while the downstream dispatch worker is running asynchronously
 * after being started with ca821x_util_start_downstream_dispatch_worker.
 *
 * @param pDeviceRef The device struct for the device that is to have callbacks modified
 * @return A mutable pointer to the device's EVBME callback structure
 */
struct EVBME_callbacks *EVBME_GetCallbackStruct(struct ca821x_dev *pDeviceRef);

//Asynchronous commands -----------------------------------------------------------------------

/**
 * Send an asynchronous EVBME COMM CHECK request to the given device. This is used to stress the comms bus.
 *
 * @param aHandle     The handle used to associate indications to this request
 * @param aDelay      The delay the receiving device should wait before sending COMM CHECK indications
 * @param aIndCount   The number of indications that the receiving device should send back
 * @param aIndSize    The size of the indications that the receiving device should send back
 * @param aPayloadLen The additional size of the payload to be included in this request
 * @param pDeviceRef  The device struct for the device this message is to be sent to
 *
 * @return Status of the command
 * @retval CA_ERROR_SUCCESS      Success
 * @retval CA_ERROR_INVALID_ARGS aPayloadLen or aIndSize is too large
 */
ca_error EVBME_COMM_CHECK_request(uint8_t            aHandle,
                                  uint8_t            aDelay,
                                  uint8_t            aIndCount,
                                  uint8_t            aIndSize,
                                  uint8_t            aPayloadLen,
                                  struct ca821x_dev *pDeviceRef);

/**
 * Send a notification to the given device to reset and take control of it. Causes an RF reset and print of device info.
 *
 * @warning This command causes a device reset, so should only be used with simple stateless applications like mac-dongle
 *
 * @param pDeviceRef The device struct for the device this message is to be sent to
 *
 * @return Status of the command
 * @retval CA_ERROR_SUCCESS Success
 */
ca_error EVBME_HOST_CONNECTED_notify(struct ca821x_dev *pDeviceRef);

/**
 * Send a notification to the given device to reset and release control of it. Causes an RF reset.
 *
 * @warning This command causes a device reset, so should only be used with simple stateless applications like mac-dongle
 *
 * @param pDeviceRef The device struct for the device this message is to be sent to
 *
 * @return Status of the command
 * @retval CA_ERROR_SUCCESS Success
 */
ca_error EVBME_HOST_DISCONNECTED_notify(struct ca821x_dev *pDeviceRef);

/**
 * Send a DFU request for reboot to the given device. Causes a full device reset into the requested mode.
 *
 * @param aRebootMode The mode to boot into
 * @param pDeviceRef  The device struct for the device this message is to be sent to
 *
 * @return Status of the command
 * @retval CA_ERROR_SUCCESS Success
 */
ca_error EVBME_DFU_REBOOT_request(enum evbme_dfu_rebootmode aRebootMode, struct ca821x_dev *pDeviceRef);

/**
 * Send a DFU request for Erase to a given device. Causes the given flash pages to be erased.
 * This command is processed asynchronously and the EVBME_DFU_STATUS_indication will indicate completion.
 *
 * EVBME_DFU_STATUS       | Status code meaning
 * ---------------------- | -------------------
 * CA_ERROR_SUCCESS       | Command was successful
 * CA_ERROR_INVALID_ARGS  | aStartAddr or aEraseLen is not aligned
 * CA_ERROR_INVALID_STATE | Device is not in DFU mode
 *
 * @param aStartAddr The start address of the erase, must be page-aligned
 * @param aEraseLen  The number of bytes to erase, must be a multiple of the page size
 * @param pDeviceRef The device struct for the device this message is to be sent to
 *
 * @return Status of the command
 * @retval CA_ERROR_SUCCESS       Success
 */
ca_error EVBME_DFU_ERASE_request(uint32_t aStartAddr, uint32_t aEraseLen, struct ca821x_dev *pDeviceRef);

/**
 * Send a DFU request for Write to a given device. Causes the given flash to be written.
 * This command is processed asynchronously and the EVBME_DFU_STATUS_indication will indicate completion.
 *
 * EVBME_DFU_STATUS       | Status code meaning
 * ---------------------- | -------------------
 * CA_ERROR_SUCCESS       | Command was successful
 * CA_ERROR_INVALID_ARGS  | aStartAddr or aWriteLen is not aligned
 * CA_ERROR_INVALID_STATE | Device is not in DFU mode
 *
 * @param aStartAddr The start address of the write, must be word-aligned
 * @param aWriteLen  The length of data to write in bytes, must be word-aligned and max 244 bytes
 * @param aWriteData The data to write, of length aWriteLen
 * @param pDeviceRef The device struct for the device this message is to be sent to
 *
 * @return Status of the command
 * @retval CA_ERROR_SUCCESS       Success
 * @retval CA_ERROR_INVALID_ARGS  aWriteLen is too long
 */
ca_error EVBME_DFU_WRITE_request(uint32_t           aStartAddr,
                                 size_t             aWriteLen,
                                 void *             aWriteData,
                                 struct ca821x_dev *pDeviceRef);

/**
 * Send a DFU request to verify a flash range of a given device.
 * This command is processed asynchronously and the EVBME_DFU_STATUS_indication will indicate completion.
 *
 * EVBME_DFU_STATUS       | Status code meaning
 * ---------------------- | -------------------
 * CA_ERROR_SUCCESS       | Command was successful, checksum matches
 * CA_ERROR_INVALID_ARGS  | aStartAddr or aCheckLen is not aligned
 * CA_ERROR_INVALID_STATE | Device is not in DFU mode
 * CA_ERROR_FAIL          | Command was successful, **CHECKSUM DOES NOT MATCH**
 *
 * @param aStartAddr The start address of the check, must be page-aligned
 * @param aCheckLen  The number of bytes to check, must be a multiple of the page size
 * @param aChecksum  The CRC32 Checksum expected of the flash range
 * @param pDeviceRef The device struct for the device this message is to be sent to
 *
 * @return Status of the command
 * @retval CA_ERROR_SUCCESS       Success - _check EVBME_DFU_STATUS_indication for checksum status_
 */
ca_error EVBME_DFU_CHECK_request(uint32_t           aStartAddr,
                                 uint32_t           aCheckLen,
                                 uint32_t           aChecksum,
                                 struct ca821x_dev *pDeviceRef);

/**
 * Send a DFU request to set the default boot mode of the given device.
 * This command is processed asynchronously and the EVBME_DFU_STATUS_indication will indicate completion.
 *
 * @param aBootMode The mode to boot into by default
 * @param pDeviceRef  The device struct for the device this message is to be sent to
 *
 * @return Status of the command
 * @retval CA_ERROR_SUCCESS Success
 */
ca_error EVBME_DFU_BOOTMODE_request(enum evbme_dfu_rebootmode aBootMode, struct ca821x_dev *pDeviceRef);

//Synchronous commands -----------------------------------------------------------------------

/**
 * Send an EVBME SET request to set the value of an EVBME attribute
 *
 * @param aAttrId    The ID of the target EVBME attribute
 * @param aAttrLen   The length of the data to send
 * @param aAttrData  The Data to set in the EVBME attribute
 * @param pDeviceRef The device struct for the device this message is to be sent to
 *
 * @return Status of the command
 * @retval CA_ERROR_SUCCESS      Success
 * @retval CA_ERROR_INVALID_ARGS Length too long
 * @retval CA_ERROR_INVALID      Attribute rejected by device for invalid len/value
 * @retval CA_ERROR_UNKNOWN      Attribute rejected by device for unknown Attr ID
 */
ca_error EVBME_SET_request_sync(enum evbme_attribute aAttrId,
                                size_t               aAttrLen,
                                uint8_t *            aAttrData,
                                struct ca821x_dev *  pDeviceRef);

/**
 * Send an EVBME GET request to get the value of an EVBME attribute
 *
 * @param aAttrId        The ID of the target EVBME attribute
 * @param aMaxAttrLen    The size of the aAttrData buffer
 * @param[out] aAttrData The buffer to store the got attribute value
 * @param[out] aAttrLen  The length of the attribute stored in the buffer
 * @param pDeviceRef     The device struct for the device this message is to be sent to
 *
 * @return Status of the command
 * @retval CA_ERROR_SUCCESS    Success - attribute value in aAttrData
 * @retval CA_ERROR_NO_BUFFER  aAttrData buffer too small to hold received data
 * @retval CA_ERROR_UNKNOWN    Attribute rejected by device for unknown Attr ID
 */
ca_error EVBME_GET_request_sync(enum evbme_attribute aAttrId,
                                size_t               aMaxAttrLen,
                                uint8_t *            aAttrData,
                                uint8_t *            aAttrLen,
                                struct ca821x_dev *  pDeviceRef);

#ifdef __cplusplus
}
#endif

#endif /* POSIX_CA821X_POSIX_INCLUDE_CA821X_POSIX_CA821X_POSIX_EVBME_H_ */

/**
 * @}
 */
