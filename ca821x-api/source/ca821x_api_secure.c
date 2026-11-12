
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "ca821x_api.h"
#include "ca821x_blacklist.h"
#include "ca821x_log.h"
#include "mac_messages.h"

#if defined(__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3L)
#include <arm_cmse.h>
typedef ca_error (*ca821x_generic_callback_nsc)(void *params, struct ca821x_dev *pDeviceRef)
    __attribute((cmse_nonsecure_call));
#if defined(__ICCARM__)
#define __NONSECURE_ENTRY __cmse_nonsecure_entry
#define __NONSECURE_ENTRY_WEAK __cmse_nonsecure_entry //__weak
#define __NONSECURE_CALL __cmse_nonsecure_call
#else
#define __NONSECURE_ENTRY __attribute__((cmse_nonsecure_entry))
#define __NONSECURE_ENTRY_WEAK __attribute__((cmse_nonsecure_entry, weak))
#define __NONSECURE_CALL __attribute__((cmse_nonsecure_call))
#endif
#else
#define __NONSECURE_ENTRY
#define __NONSECURE_ENTRY_WEAK __attribute__((weak))
#define __NONSECURE_CALL
#endif

__NONSECURE_ENTRY ca_error ca821x_upstream_dispatch(struct MAC_Message *msg, struct ca821x_dev *pDeviceRef)
{
	ca_error ret = CA_ERROR_NOT_HANDLED;

	union ca821x_api_callback *rval = ca821x_get_callback(msg->CommandId, pDeviceRef);

	if (!rval) //Unrecognised command ID
	{
		ret = CA_ERROR_INVALID_ARGS;
#if CASCODA_CA_VER >= 8212
		//TODO: Temporary for CA-8212 development
		if (pDeviceRef->callbacks.generic_dispatch)
			ret = pDeviceRef->callbacks.generic_dispatch(msg, pDeviceRef);
#endif // CASCODA_CA_VER >= 8212
		goto exit;
	}

	/* call appropriate checks/updates/workarounds */
#if CASCODA_CA_VER == 8210
	switch (msg->CommandId)
	{
	case SPI_MCPS_DATA_INDICATION:
		ret = check_data_ind_destaddr(&msg->PData.DataInd, pDeviceRef);
		if (ret)
		{
			goto exit;
		}
		break;
	case SPI_MLME_SCAN_CONFIRM:
		verify_scancnf_results(msg, pDeviceRef);
		break;
	case SPI_MLME_ASSOCIATE_CONFIRM:
		get_assoccnf_shortaddr(&msg->PData.AssocCnf, pDeviceRef);
		break;
	}
#endif // CASCODA_CA_VER == 8210

#if CASCODA_MAC_BLACKLIST != 0
	if (blacklist_must_filter(msg, pDeviceRef))
	{
		ret = CA_ERROR_SUCCESS;
		goto exit;
	}
#endif // CASCODA_MAC_BLACKLIST

	//If there is a callback registered, call it. Otherwise (or if not handled), call the generic dispatch.
	ret = CA_ERROR_NOT_HANDLED;
	if (rval->generic_callback)
	{
#if defined(__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3L)
		ret = ((ca821x_generic_callback_nsc)rval->generic_callback)(msg->PData.Payload, pDeviceRef);
#else
		ret = rval->generic_callback(msg->PData.Payload, pDeviceRef);
#endif
	}
	if (ret == CA_ERROR_NOT_HANDLED && pDeviceRef->callbacks.generic_dispatch)
	{
		ret = pDeviceRef->callbacks.generic_dispatch(msg, pDeviceRef);
	}

exit:
	return ret;
}

#if CASCODA_MAC_BLACKLIST != 0

static uint8_t blacklist_must_filter(struct MAC_Message *msg, struct ca821x_dev *pDeviceRef)
{
	struct FullAddr src;
	size_t          address_len;
	src.AddressMode = MAC_MODE_NO_ADDR;

	switch (msg->CommandId)
	{
	case SPI_MCPS_DATA_INDICATION:
		src = msg->PData.DataInd.Src;
		break;
	case SPI_MLME_ASSOCIATE_INDICATION:
		src.AddressMode = MAC_MODE_LONG_ADDR;
		memcpy(src.Address, msg->PData.AssocInd.DeviceAddress, sizeof(src.Address));
		break;
	case SPI_MLME_BEACON_NOTIFY_INDICATION:
		src = msg->PData.BeaconInd.PanDescriptor.Coord;
		break;
	case SPI_MLME_COMM_STATUS_INDICATION:
		src.AddressMode = msg->PData.CommStatusInd.SrcAddrMode;
		memcpy(src.Address, msg->PData.CommStatusInd.SrcAddr, sizeof(src.Address));
		break;
	case SPI_MLME_DISASSOCIATE_INDICATION:
		src.AddressMode = MAC_MODE_LONG_ADDR;
		memcpy(src.Address, msg->PData.DisassocInd.DevAddr, sizeof(src.Address));
		break;
	case SPI_MLME_ORPHAN_INDICATION:
		src.AddressMode = MAC_MODE_LONG_ADDR;
		memcpy(src.Address, msg->PData.OrphanInd.OrphanAddr, sizeof(src.Address));
		break;
#if CASCODA_CA_VER >= 8211
	case SPI_MLME_POLL_INDICATION:
		src = msg->PData.PollInd.Src;
		break;
#endif // CASCODA_CA_VER >= 8211
#if CASCODA_CA_VER >= 8212
	case SPI_MLME_IE_NOTIFY_INDICATION:
		src = msg->Pdata.NotifyInd.Src;
		break;
#endif // CASCODA_CA_VER >= 8212
	}

	if (src.AddressMode == MAC_MODE_SHORT_ADDR)
		address_len = 2;
	else if (src.AddressMode == MAC_MODE_LONG_ADDR)
		address_len = 8;
	else
		address_len = 0;

	if (address_len)
	{
		for (uint8_t i = 0; i < CASCODA_MAC_BLACKLIST; ++i)
		{
			if ((pDeviceRef->blacklist[i].AddressMode == src.AddressMode &&
			     (memcmp(src.Address, pDeviceRef->blacklist[i].Address, address_len) == 0)))
			{
				return 1;
			}
		}
	}
	return 0;
}
#endif

__NONSECURE_ENTRY ca_error BLACKLIST_Add(struct MacAddr *pAddress, struct ca821x_dev *pDeviceRef)
{
#if CASCODA_MAC_BLACKLIST != 0
	ca_error ret;
	if (pAddress->AddressMode != MAC_MODE_LONG_ADDR && pAddress->AddressMode != MAC_MODE_SHORT_ADDR)
	{
		ret = CA_ERROR_INVALID_ARGS;
		goto exit;
	}

	ret = CA_ERROR_NO_BUFFER;
	for (uint8_t i = 0; i < CASCODA_MAC_BLACKLIST; ++i)
	{
		if (pDeviceRef->blacklist[i].AddressMode == MAC_MODE_NO_ADDR)
		{
			pDeviceRef->blacklist[i].AddressMode = pAddress->AddressMode;
			memcpy(pDeviceRef->blacklist[i].Address, pAddress->Address, 8);
			ret = CA_ERROR_SUCCESS;
			goto exit;
		}
	}

exit:
	return ret;
#else
	(void)pAddress;
	(void)pDeviceRef;
	return CA_ERROR_FAIL;
#endif
}

__NONSECURE_ENTRY void BLACKLIST_Clear(struct ca821x_dev *pDeviceRef)
{
#if CASCODA_MAC_BLACKLIST != 0
	for (uint8_t i = 0; i < CASCODA_MAC_BLACKLIST; ++i)
	{
		pDeviceRef->blacklist[i].AddressMode = MAC_MODE_NO_ADDR;
	}
#else
	(void)pDeviceRef;
#endif
}