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
#include <algorithm>

#include "ca821x-posix/ca821x-posix.h"

#include "DeviceList.hpp"

namespace ca {

bool DeviceListFilter::IsFilterPass(const DeviceInfo &aDeviceInfo) const
{
	if (mAvailableFilterEnabled)
	{
		if (aDeviceInfo.IsAvailable() != mAvailableFilter)
			return false;
	}
	if (mExtFlashAvailableFilterEnabled)
	{
		if (aDeviceInfo.IsExternalFlashChipAvailable() != mExtFlashAvailableFilter)
			return false;
	}
	if (!mSerialNoFilter.empty())
	{
		if (std::find(mSerialNoFilter.begin(), mSerialNoFilter.end(), aDeviceInfo.GetSerialNo()) ==
		    mSerialNoFilter.end())
		{
			return false;
		}
	}
	if (!mAppNameFilter.empty())
	{
		if (aDeviceInfo.GetAppName() != mAppNameFilter)
			return false;
	}
	if (!mMinVersionFilter.empty())
	{
		if (!aDeviceInfo.GetVersion())
			return false;
		if (EVBME_CompareVersions(aDeviceInfo.GetVersion(), mMinVersionFilter.c_str(), NULL, NULL) < 0)
			return false;
	}
	return true;
}

void DeviceList::Refresh(const DeviceListFilter &aFilter, bool enumerate_uart)
{
	mDevices.clear();
	ca821x_util_enumerate(
	    [](ca_device_info *aDeviceInfo, void *aContext) {
		    static_cast<DeviceList *>(aContext)->mDevices.emplace_back(aDeviceInfo);
	    },
	    enumerate_uart,
	    this);
	mDevices.erase(std::remove_if(mDevices.begin(),
	                              mDevices.end(),
	                              [&](const DeviceInfo &di) { return !aFilter.IsFilterPass(di); }),
	               mDevices.end());
}

} /* namespace ca */
