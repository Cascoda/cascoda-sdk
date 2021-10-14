/*
 *  Copyright (c) 2020, Cascoda Ltd.
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

#include <cstring>

#include "DeviceInfo.hpp"

namespace ca {

DeviceInfo::DeviceInfo(const ca_device_info *aDeviceInfo)
{
	copy_from(aDeviceInfo);
}

DeviceInfo::DeviceInfo(const DeviceInfo &aDeviceInfo) //NOLINT(bugprone-copy-constructor-init)
    : ca_device_info()
{
	copy_from(static_cast<const ca_device_info *>(&aDeviceInfo));
}

DeviceInfo::DeviceInfo(DeviceInfo &&aDeviceInfo)
{
	move_from(aDeviceInfo);
}

DeviceInfo &DeviceInfo::operator=(const DeviceInfo &other)
{
	if (this != &other)
	{
		delete[] mBuffer;
		copy_from(static_cast<const ca_device_info *>(&other));
	}
	return *this;
}

DeviceInfo &DeviceInfo::operator=(DeviceInfo &&other)
{
	if (this != &other)
	{
		delete[] mBuffer;
		move_from(other);
	}
	return *this;
}

void DeviceInfo::copy_from(const ca_device_info *aDeviceInfo)
{
	size_t pathlen    = aDeviceInfo->path ? 1 + strlen(aDeviceInfo->path) : 0;
	size_t devnamelen = aDeviceInfo->device_name ? 1 + strlen(aDeviceInfo->device_name) : 0;
	size_t appnamelen = aDeviceInfo->app_name ? 1 + strlen(aDeviceInfo->app_name) : 0;
	size_t verlen     = aDeviceInfo->version ? 1 + strlen(aDeviceInfo->version) : 0;
	size_t serlen     = aDeviceInfo->serialno ? 1 + strlen(aDeviceInfo->serialno) : 0;
	size_t buflen     = pathlen + devnamelen + appnamelen + verlen + serlen + 1;
	char * fillptr    = nullptr;

	mBuffer    = new char[buflen];
	mBuffer[0] = '\0';
	fillptr    = mBuffer + 1;

	if (pathlen)
	{
		memcpy(fillptr, aDeviceInfo->path, pathlen);
		path = fillptr;
		fillptr += pathlen;
	}
	else
	{
		path = mBuffer;
	}

	if (devnamelen)
	{
		memcpy(fillptr, aDeviceInfo->device_name, devnamelen);
		device_name = fillptr;
		fillptr += devnamelen;
	}
	else
	{
		device_name = mBuffer;
	}

	if (appnamelen)
	{
		memcpy(fillptr, aDeviceInfo->app_name, appnamelen);
		app_name = fillptr;
		fillptr += appnamelen;
	}
	else
	{
		app_name = mBuffer;
	}

	if (verlen)
	{
		memcpy(fillptr, aDeviceInfo->version, verlen);
		version = fillptr;
		fillptr += verlen;
	}
	else
	{
		version = mBuffer;
	}

	if (serlen)
	{
		memcpy(fillptr, aDeviceInfo->serialno, serlen);
		serialno = fillptr;
		fillptr += serlen;
	}
	else
	{
		serialno = mBuffer;
	}

	(void)fillptr;
	exchange_type            = aDeviceInfo->exchange_type;
	available                = aDeviceInfo->available;
	external_flash_available = aDeviceInfo->external_flash_available;
}

const char *DeviceInfo::GetExchangeTypeAsString()
{
	switch (exchange_type)
	{
	case ca821x_exchange_usb:
		return "USB";
	case ca821x_exchange_uart:
		return "UART";
	case ca821x_exchange_kernel:
		return "kernel";
	default:
		return "???";
	}
}

void DeviceInfo::move_from(DeviceInfo &aDeviceInfo)
{
	mBuffer                  = aDeviceInfo.mBuffer;
	path                     = aDeviceInfo.path;
	device_name              = aDeviceInfo.device_name;
	app_name                 = aDeviceInfo.app_name;
	version                  = aDeviceInfo.version;
	serialno                 = aDeviceInfo.serialno;
	exchange_type            = aDeviceInfo.exchange_type;
	available                = aDeviceInfo.available;
	external_flash_available = aDeviceInfo.external_flash_available;

	//Put the moved-from instance in a valid state that won't free the memory we stole
	aDeviceInfo.mBuffer = nullptr;
}

} /* namespace ca */
