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

#ifndef POSIX_APP_CHILICTL_COMMON_DEVICELIST_HPP_
#define POSIX_APP_CHILICTL_COMMON_DEVICELIST_HPP_

#include <string>
#include <vector>

#include "common/DeviceInfo.hpp"

namespace ca {

class DeviceListFilter
{
	friend class DeviceList;

public:
	/**
	 * Default constructor
	 */
	DeviceListFilter()
	    : mAvailableFilter()
	    , mSerialNoFilter()
	    , mAvailableFilterEnabled()
	{
	}

	/**
	 * Enable the 'available' filter and set the parameters
	 * @param aTarget The desired value of 'Available' in the results.
	 */
	void SetAvailable(bool aTarget)
	{
		mAvailableFilter        = aTarget;
		mAvailableFilterEnabled = true;
	}

	/**
	 * Disable the available filter.
	 */
	void ClearAvailable() { mAvailableFilterEnabled = false; }

	/**
	 * Set the serial number filter. An empty string means no filter.
	 * @param aSerialNo The desired serial number
	 */
	void SetSerialNo(const char *aSerialNo) { mSerialNoFilter = aSerialNo; }

	/**
	 * Set the app name filter. An empty string means no filter.
	 * @param aAppName The desired app name
	 */
	void SetAppName(const char *aAppName) { mAppNameFilter = aAppName; }

	/**
	 * Run the filter on the given DeviceInfo
	 * @param aDeviceInfo The DeviceInfo to filter
	 * @return True if aDeviceInfo passes the filter, False if it should be filtered out
	 */
	bool IsFilterPass(const DeviceInfo &aDeviceInfo) const;

private:
	bool        mAvailableFilter;
	std::string mSerialNoFilter;
	std::string mAppNameFilter;

	bool mAvailableFilterEnabled;

	static const DeviceListFilter sEmpty;
};

/**
 * Class to assist in obtaining a list of connected ca821x devices, filtering as required.
 */
class DeviceList
{
public:
	/**
	 * Default constructor.
	 */
	DeviceList()
	    : mDevices()
	{
	}

	/**
	 * Refresh the internal list of devices. Must be called at least once.
	 */
	void Refresh(const DeviceListFilter &aFilter = DeviceListFilter::sEmpty);

	/**
	 * Get the list of discovered devices as a vector.
	 * @return Vector of discovered devices.
	 */
	const std::vector<DeviceInfo> &Get() { return mDevices; }

private:
	std::vector<DeviceInfo> mDevices;
};

} /* namespace ca */

#endif /* POSIX_APP_CHILICTL_COMMON_DEVICELIST_HPP_ */
