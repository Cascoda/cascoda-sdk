/*
 * Copyright (c) 2021, Cascoda
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
 * Stubs for OTA Upgrade procedure. Contact us for more information.
 */

#ifndef CASCODA_OTA_STUBS_H
#define CASCODA_OTA_STUBS_H

#include "cascoda-bm/cascoda_interface.h"
#include "ca821x_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#if CASCODA_EXTERNAL_FLASHCHIP_PRESENT
/***** Callback type to allow flexibility in using the external flash API ******/
typedef ca_error (*ExtFlashAPICallback)(void *aContext);

/**
 * Stub: Does nothing, returns CA_ERROR_NOT_IMPLEMENTED
 *
 * @return CA_ERROR_NOT_IMPLEMENTED
 */
ca_error ota_upgrade_init();

/**
 * Stub: Does nothing, returns CA_ERROR_INVALID_ARGS
 *
 * @param aStartAddr         Start address of the external flash to write to.
 * @param aWriteLen          Number of bytes to write to the external flash.
 * @param aData              Data to write to the external flash.
 * @param aExternalFlashInfo External flash information.
 * @param aUpstreamCallback	 Upstream callback that is executed at the completion of the procedure initiated by this function.
 *
 * @return CA_ERROR_INVALID_ARGS
 */
ca_error ota_handle_write(uint32_t            aStartAddr,
                          uint32_t            aWriteLen,
                          uint8_t *           aData,
                          ExternalFlashInfo   aExternalFlashInfo,
                          ExtFlashAPICallback aUpstreamCallback);

/**
 * Stub: Does nothing, returns CA_ERROR_INVALID_ARGS
 *
 * @param aStartAddr        Start address of the external flash to be erased.
 * @param aEraseLen         Number of bytes to erase from the external flash.
 * @param aUpstreamCallback Upstream callback that is executed at the completion of the procedure initiated by this function.
 *
 * @return CA_ERROR_INVALID_ARGS
 */
ca_error ota_handle_erase(uint32_t aStartAddr, uint32_t aEraseLen, ExtFlashAPICallback aUpstreamCallback);

/**
 * Stub: Does nothing, returns CA_ERROR_INVALID_ARGS
 *
 * @param aStartAddr         Start address of the external flash where the data checked is stored.
 * @param aCheckLen          Number of bytes to be checked.
 * @param aCheckSum          Checksum for the given number of bytes.
 * @param aExternalFlashInfo External flash information.
 * @param aUpstreamCallback	 Upstream callback that is executed at the completion of the procedure initiated by this function.
 *
 * @return CA_ERROR_INVALID_ARGS
 */
ca_error ota_handle_check(uint32_t            aStartAddr,
                          uint32_t            aCheckLen,
                          uint32_t            aCheckSum,
                          ExternalFlashInfo   aExternalFlashInfo,
                          ExtFlashAPICallback aUpstreamCallback);
#endif

#ifdef __cplusplus
}
#endif

#endif /* CASCODA_OTA_STUBS_H */
