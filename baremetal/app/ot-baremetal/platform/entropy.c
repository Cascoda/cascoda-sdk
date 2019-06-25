/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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

#include "openthread/platform/entropy.h"
#include "cascoda-bm/cascoda_types.h"
#include "ca821x_api.h"
#include "code_utils.h"
#include "hwme_tdme.h"
#include "platform.h"

otError otPlatEntropyGet(uint8_t *aOutput, uint16_t aOutputLength)
{
	struct ca821x_dev *pDeviceRef = PlatformGetDeviceRef();
	otError            error      = OT_ERROR_NONE;
	uint8_t            result[2];
	uint8_t            len;
	uint16_t           curLen = 0;

	otEXPECT_ACTION(aOutput != NULL, error = OT_ERROR_INVALID_ARGS);

	while (curLen < aOutputLength)
	{
		HWME_GET_request_sync(HWME_RANDOMNUM, &len, result, pDeviceRef);
		otEXPECT_ACTION(len == 2, error = OT_ERROR_ABORT);

		if (curLen == aOutputLength - 1)
		{
			aOutput[curLen++] = result[0];
		}
		else
		{
			aOutput[curLen++] = result[0];
			aOutput[curLen++] = result[1];
		}
	}

exit:
	return error;
}
