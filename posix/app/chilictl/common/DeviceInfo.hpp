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

#ifndef POSIX_APP_CHILICTL_COMMON_DEVICEINFO_HPP_
#define POSIX_APP_CHILICTL_COMMON_DEVICEINFO_HPP_

#include "ca821x-posix/ca821x-types.h"

namespace ca {

/**
 * C++ wrapper for the C ca_device_info struct, which owns its own memory.
 */
class DeviceInfo : private ca_device_info
{
public:
	/**
	 * Default constructor for empty instance
	 */
	DeviceInfo()
	    : mBuffer(nullptr)
	{
	}

	/**
	 * Constructor to copy from the c-style struct passed by lower levels
	 * @param aDeviceInfo Pointer to ca_device_info struct.
	 */
	DeviceInfo(const ca_device_info *aDeviceInfo);

	/**
	 * Copy constructor
	 * @param aDeviceInfo instance to copy from
	 */
	DeviceInfo(const DeviceInfo &aDeviceInfo);

	/**
	 * Move constructor
	 * @param aDeviceInfo instance to move from
	 */
	DeviceInfo(DeviceInfo &&aDeviceInfo);

	/**
	 * Destructor to free owned memory
	 */
	~DeviceInfo() { delete[] mBuffer; }

	/**
	 * Copy assignment
	 * @param other instance to copy from
	 * @return reference to this
	 */
	DeviceInfo &operator=(const DeviceInfo &other);

	/**
	 * Move assignment
	 * @param other instance to move from
	 * @return reference to this
	 */
	DeviceInfo &operator=(DeviceInfo &&other);

	/**
	 * Get the exchange type in a string format.
	 * @return Exchange type in string format
	 */
	const char *GetExchangeTypeAsString();

	/**
	 * @return @copydoc ca_device_info::exchange_type
	 */
	ca821x_exchange_type GetExchangeType() const { return exchange_type; }

	/**
	 * @return @copydoc ca_device_info::path
	 */
	const char *GetPath() const { return path; }

	/**
	 * @return @copydoc ca_device_info::device_name
	 */
	const char *GetDeviceName() const { return device_name; }

	/**
	 * @return @copydoc ca_device_info::app_name
	 */
	const char *GetAppName() const { return app_name; }

	/**
	 * @return @copydoc ca_device_info::version
	 */
	const char *GetVersion() const { return version; }

	/**
	 * @return @copydoc ca_device_info::serialno
	 */
	const char *GetSerialNo() const { return serialno; }

	/**
	 * @return @copydoc ca_device_info::available
	 */
	bool IsAvailable() const { return available; }

	/**
	 * @return @copydoc ca_device_info::external_flash_available
	 */
	bool IsExternalFlashChipAvailable() const { return external_flash_available; }

private:
	char *mBuffer;

	void copy_from(const ca_device_info *aDeviceInfo);
	void move_from(DeviceInfo &aDeviceInfo);
};

} /* namespace ca */

#endif /* POSIX_APP_CHILICTL_COMMON_DEVICEINFO_HPP_ */
