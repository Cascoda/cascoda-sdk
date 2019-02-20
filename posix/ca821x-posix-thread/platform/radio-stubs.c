#include "openthread/platform/radio-mac.h"

void otPlatRadioEnableSrcMatch(otInstance *aInstance, bool aEnable)
{
	(void)aInstance;
	(void)aEnable;
}

otError otPlatRadioAddSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
	(void)aInstance;
	(void)aShortAddress;
	return OT_ERROR_NONE;
}

otError otPlatRadioAddSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
	(void)aInstance;
	(void)aExtAddress;
	return OT_ERROR_NONE;
}

otError otPlatRadioClearSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
	(void)aInstance;
	(void)aShortAddress;
	return OT_ERROR_NONE;
}

otError otPlatRadioClearSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
	(void)aInstance;
	(void)aExtAddress;
	return OT_ERROR_NONE;
}

void otPlatRadioClearSrcMatchShortEntries(otInstance *aInstance)
{
	(void)aInstance;
}

void otPlatRadioClearSrcMatchExtEntries(otInstance *aInstance)
{
	(void)aInstance;
}
