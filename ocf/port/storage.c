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

#define OC_STORAGE_CBOR_OVERHEAD (5)

extern otInstance *OT_INSTANCE;

static uint8_t write_buffer[8192];

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
	// Algorithm overview: iterate through the settings @ 0xe107 (magic number,
	// looks like IoT and is high so as not to conflict with OpenThread) using
	// otPlatSettingsGet. At every index you will find an object that looks like
	// {"super-secure-password": "fas,opc0q-2=91,51"}. Compare `store` with the key
	// and copy the value to buf.

	// Assumption: the items stored in the settings are always of the format
	// {"super-secure-password": "fas,opc0q-2=91,51"}.

	OC_DBG("Getting stored data with the key %s...\n", store);
	void *   read_buffer;
	uint16_t read_buffer_size;

	for (int i = 0;; ++i)
	{
		int        error;
		CborParser parser;
		CborValue  key_found;
		CborValue  value;

		// The initial value of `read_buffer_size` is used by OpenThread as the maximum
		// size of the buffer to write into.

		error = otPlatSettingsGetAddress(OC_SETTINGS_KEY, i, &read_buffer, &read_buffer_size);

		// No more settings
		if (error)
		{
			OC_DBG("Could not find settings! Final otPlatSettingsGet returns %d\n", error);
			return -error;
		}

		// You have the i-th setting - parse it as a CBOR object.
		error = cbor_parser_init(read_buffer, read_buffer_size, 0, &parser, &key_found);

		// The data is corrupt, or some other part of the code inserted a non-CBOR value
		// into this key.
		if (error)
		{
			OC_DBG("Corrupt data in OCF Storage!");
			continue;
		}
		// Check whether the key matches what the user supplied.
		error = cbor_value_map_find_value(&key_found, store, &value);

		// The key does not match - try the next setting
		if (error || !cbor_value_is_valid(&value))
			continue;

		// Copy the contents to the output buffer
		OC_DBG("Data of size %d found!\n", read_buffer_size);
		cbor_value_copy_byte_string(&value, buf, &size, NULL);
		return size;
	}

	return -OT_ERROR_NOT_FOUND;
}

long oc_storage_write(const char *store, uint8_t *buf, size_t size)
{
	// Algorithm overview: iterate through the settings @ 0xe107 until you
	// either find the key or otPlatSettingsGet returns OT_ERROR_NOT_FOUND.
	// If you found the key, delete that entry and insert the new one.
	// If you haven't found the key, add it anyways!

	OC_DBG("Writing data at key %s\n", store);
	OC_DBG("Name Length: %d", strlen(store));
	OC_DBG("Data Length: %d", size);
	otError     error;
	CborEncoder encoder, mapEncoder;
	uint16_t    length;
	void *      read_buffer;
	size_t      read_buffer_size;

	size_t write_buffer_max_size = strlen(store) + size + OC_STORAGE_CBOR_OVERHEAD;

	if (!read_buffer)
	{
		OC_ERR("Could not allocate read buffer!");
		return -1;
	}

	for (int i = 0;; ++i)
	{
		CborParser parser;
		CborValue  key_found;
		CborValue  value;
		uint16_t   read_buffer_size;

		// The CBOR code is essentialy identical to oc_storage_read, but we act differently
		// depending on its results.

		error = otPlatSettingsGetAddress(OC_SETTINGS_KEY, i, &read_buffer, &read_buffer_size);

		// We iterated through all the keys and the key to be added does not match.
		// Break and add the new setting.
		if (error)
		{
			OC_DBG("Key was not found! Error code %d. Creating new entry...\n", error);
			break;
		}

		error = cbor_parser_init(read_buffer, read_buffer_size, 0, &parser, &key_found);

		if (error)
		{
			continue;
		}
		error = cbor_value_map_find_value(&key_found, store, &value);

		// We found the key - delete it and add the new item.
		if (cbor_value_is_valid(&value))
		{
			OC_DBG("Key already exists! Deleting...\n");
			otPlatSettingsDelete(OT_INSTANCE, OC_SETTINGS_KEY, i);
			break;
		}
	}

	if (write_buffer_max_size > sizeof(write_buffer))
	{
		OC_ERR("Stored data too large for write buffer!");
		return -1;
	}
	cbor_encoder_init(&encoder, write_buffer, write_buffer_max_size, 0);

	error = cbor_encoder_create_map(&encoder, &mapEncoder, 1);
	if (error)
		goto CBOR_ERROR;
	error = cbor_encode_text_stringz(&mapEncoder, store);
	if (error)
		goto CBOR_ERROR;
	error = cbor_encode_byte_string(&mapEncoder, buf, size);
	if (error)
		goto CBOR_ERROR;
	error = cbor_encoder_close_container(&encoder, &mapEncoder);
	if (error)
		goto CBOR_ERROR;

	length = cbor_encoder_get_buffer_size(&encoder, write_buffer);
	error  = otPlatSettingsAdd(OT_INSTANCE, OC_SETTINGS_KEY, write_buffer, length);

	OC_DBG("Encoded Length: %d", length);

	if (error)
		OC_DBG("Could not add new setting! otPlatSettingsAdd returned %d", error);
	return error ? -error : length;

CBOR_ERROR:
	OC_DBG("CBOR encoding error! Error code %d", error);
	return -error;
}

#endif /* OC_SECURITY */
