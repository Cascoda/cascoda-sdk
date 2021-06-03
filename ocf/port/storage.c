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
#include "cascoda-util/cascoda_rand.h"
#include "port/oc_log.h"
#include "port/oc_storage.h"
#include "cascoda_chili_config.h"
#include "oc_config.h"
#include "platform.h"

#include "mbedtls/ccm.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"

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
	void *              read_buffer;
	uint16_t            read_buffer_size;
	mbedtls_ccm_context ccm;
	int                 error;

	mbedtls_ccm_init(&ccm);

	// Data structure we are looking for: null-terminated string containing
	// the name of the "file" we are reading, followed by the data contained
	// in said file up until read_buffer_size

	for (int i = 0;; ++i)
	{
		error = otPlatSettingsGetAddress(OC_SETTINGS_KEY, i, &read_buffer, &read_buffer_size);

		if (error || read_buffer_size != 16)
		{
			OC_DBG("Could not find key! Final otPlatSettingsGet returns %d\n", error);
			goto exit;
		}

		if (strcmp(store, (char *)read_buffer) == 0)
		{
			// we have found the key we are looking for, copy and return
			uint16_t offset = strlen(store) + 1;

			uint8_t  key[16];
			uint16_t key_size = sizeof(key);
			error             = otPlatSettingsGet(OT_INSTANCE, OC_ENCRYPTION_KEY_KEY, 0, key, &key_size);

			mbedtls_ccm_setkey(&ccm, MBEDTLS_CIPHER_ID_AES, key, 128);

			if (error)
				goto exit;

			uint8_t iv[13];
			memcpy(iv, read_buffer + offset, sizeof(iv));
			offset += sizeof(iv);

			uint8_t tag[8];
			memcpy(tag, read_buffer + offset, sizeof(tag));
			offset += sizeof(tag);

			uint16_t data_len = read_buffer_size - offset;

			mbedtls_ccm_auth_decrypt(
			    &ccm, data_len, iv, sizeof(iv), store, strlen(store) + 1, read_buffer + offset, buf, tag, sizeof(tag));

			OC_DBG("Data of size %d found!\n", data_len);
			//memcpy(buf, read_buffer + offset, data_len);
			mbedtls_ccm_free(&ccm);
			return data_len;
		}
	}
exit:
	mbedtls_ccm_free(&ccm);
	return -error;
}

long oc_storage_write(const char *store, uint8_t *buf, size_t size)
{
	void *              read_buffer;
	uint16_t            read_buffer_size;
	mbedtls_ccm_context ccm;
	otError             error;
	uint8_t             key[16];
	uint16_t            key_size = sizeof(key);

	OC_DBG("Writing data at key %s\n", store);
	OC_DBG("Name Length: %d", strlen(store));
	OC_DBG("Data Length: %d", size);

	mbedtls_ccm_init(&ccm);

	error = otPlatSettingsGet(OT_INSTANCE, OC_ENCRYPTION_KEY_KEY, 0, key, &key_size);

	if (error == OT_ERROR_NOT_FOUND || key_size != sizeof(key))
	{
		// generate random key
		error = RAND_GetCryptoBytes(sizeof(key), key);

		if (error != CA_ERROR_SUCCESS)
			goto exit;

		// store the key, which will be used to decrypt all future OCF settings
		otPlatSettingsAdd(OT_INSTANCE, OC_ENCRYPTION_KEY_KEY, key, sizeof(key));
	}

	// We either have the key from storage, or we have just generated one
	mbedtls_ccm_setkey(&ccm, MBEDTLS_CIPHER_ID_AES, key, 128);

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

	uint8_t iv[13];
	uint8_t tag[8];
	// seed the nonce with random data
	error = RAND_GetCryptoBytes(sizeof(iv), iv);
	if (error != CA_ERROR_SUCCESS)
		goto exit;

	mbedtls_ccm_encrypt_and_tag(&ccm, size, iv, sizeof(iv), store, strlen(store) + 1, buf, buf, tag, sizeof(tag));

	// Writing new entry to flash
	struct settingBuffer buffers[4] = {{store, strlen(store) + 1}, {iv, sizeof(iv)}, {tag, sizeof(tag)}, {buf, size}};

	error = otPlatSettingsAddVector(OT_INSTANCE, OC_SETTINGS_KEY, buffers, 4);

	OC_DBG("Encoded Length: %d", size);

	if (error)
		OC_DBG("Could not add new setting! otPlatSettingsAdd returned %d", error);

	mbedtls_ccm_auth_decrypt(&ccm, size, iv, sizeof(iv), store, strlen(store) + 1, buf, buf, tag, sizeof(tag));
exit:
	mbedtls_ccm_free(&ccm);
	return error ? -error : size;
}
#endif /* OC_SECURITY */
