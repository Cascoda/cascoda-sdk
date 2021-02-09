/*
// Copyright 2018 Oleksandr Grytsov
// Modifications copyright (c) 2020, Cascoda Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include <cbor.h>
#include <openthread/instance.h>
#include <openthread/platform/settings.h>
#include "port/oc_log.h"
#include "port/oc_storage.h"
#include "cascoda_chili_config.h"
#include "oc_config.h"
#include "platform.h"

#ifdef OC_SECURITY

extern otInstance *OT_INSTANCE;

int oc_storage_config(const char *store)
{
	// In the POSIX example, `store` is the name of the folder where the
	// credentials are stored as files. Since the Chili does not have a
	// filesystem and can only run one application at a time, this function
	// does nothing.
	(void)store;
	return 0;
}

long oc_storage_read(const char *store, uint8_t *buf, size_t size)
{
	OC_DBG("Getting stored data with the key %s...\n", store);
	void *   read_buffer;
	uint16_t read_buffer_size;

	// Data structure we are looking for: null-terminated string containing
	// the name of the "file" we are reading, followed by the data contained
	// in said file up until read_buffer_size

	for (int i = 0;; ++i)
	{
		int error;
		error = otPlatSettingsGetAddress(OC_SETTINGS_KEY, i, &read_buffer, &read_buffer_size);

		if (error)
		{
			OC_DBG("Could not find settings! Final otPlatSettingsGet returns %d\n", error);
			return -error;
		}

		if (strcmp(store, (char *)read_buffer) == 0)
		{
			// we have found the key we are looking for, copy and return
			uint16_t offset   = strlen(store) + 1;
			uint16_t data_len = read_buffer_size - offset;

			OC_DBG("Data of size %d found!\n", data_len);
			memcpy(buf, read_buffer + offset, data_len);
			return data_len;
		}
	}
	return -OT_ERROR_NOT_FOUND;
}

long oc_storage_write(const char *store, uint8_t *buf, size_t size)
{
	// Write store to appropriate setting (including null terminator), followed by data
	OC_DBG("Writing data at key %s\n", store);
	OC_DBG("Name Length: %d", strlen(store));
	OC_DBG("Data Length: %d", size);
	otError  error;
	void *   read_buffer;
	uint16_t read_buffer_size;

	for (int i = 0;; ++i)
	{
		// Check whether the file already exists
		int error;
		error = otPlatSettingsGetAddress(OC_SETTINGS_KEY, i, &read_buffer, &read_buffer_size);

		if (error)
		{
			OC_DBG("Key was not found! Error code %d. Creating new entry...\n", error);
			break;
		}

		if (strcmp(store, (char *)read_buffer) == 0)
		{
			// We found the key - delete it and add the new item.
			OC_DBG("Key already exists! Deleting...\n");
			otPlatSettingsDelete(OT_INSTANCE, OC_SETTINGS_KEY, i);
			break;
		}
	}

	// Writing new entry to flash
	struct settingBuffer buffers[2] = {{store, strlen(store) + 1}, {buf, size}};

	error = otPlatSettingsAddVector(OT_INSTANCE, OC_SETTINGS_KEY, buffers, 2);

	OC_DBG("Encoded Length: %d", size);

	if (error)
		OC_DBG("Could not add new setting! otPlatSettingsAdd returned %d", error);
	return error ? -error : size;
}
#endif /* OC_SECURITY */
