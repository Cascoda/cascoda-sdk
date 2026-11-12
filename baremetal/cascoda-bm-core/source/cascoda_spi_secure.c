#include <arm_cmse.h>

#include <stdio.h>
#include <string.h>

#include "cascoda-bm/cascoda_interface_core.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-bm/cascoda_wait.h"
#include "cascoda-util/cascoda_time.h"
#include "ca821x_api.h"
#include "mac_messages.h"

#if defined(__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3L)
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

#if CASCODA_EXTERNAL_FLASHCHIP_PRESENT
static volatile bool SpiExternallyInUse = false; //!< SPI is currently in use by an external device
static void (*SPI_External_Complete_callback)(void);

__NONSECURE_ENTRY void SetSpiExternallyInUse(bool in_use)
{
	SpiExternallyInUse = in_use;
}

__NONSECURE_ENTRY bool GetSpiExternallyInUse()
{
	return SpiExternallyInUse;
}

__NONSECURE_ENTRY void SetSpiExternalCompleteCallback(void (*new_cb)(void))
{
	SPI_External_Complete_callback = new_cb;
}

#if defined(__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3L)
typedef void (*spi_callback_ns_t)(void) __attribute((cmse_nonsecure_call));
#endif

__NONSECURE_ENTRY void CallSpiExternalCompleteCallback()
{
#if defined(__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3L)
	if ((int)SPI_External_Complete_callback & 0x10000000)
		((spi_callback_ns_t)cmse_nsfptr_create(SPI_External_Complete_callback))();
	else
#endif
		SPI_External_Complete_callback();
}

#endif

static volatile bool ExchangeInProgress = false; //!< SPI Exchange currently in progress (possibly dma)
static volatile bool FinalExchange      = false; //!< If true, the RFSS pin will be set to high upon exchange completion

__NONSECURE_ENTRY void SetExchangeInProgress(bool val)
{
	ExchangeInProgress = val;
}

__NONSECURE_ENTRY bool IsExchangeInProgress()
{
	return ExchangeInProgress;
}

__NONSECURE_ENTRY void SetFinalExchange(bool val)
{
	FinalExchange = val;
}
__NONSECURE_ENTRY bool IsFinalExchange()
{
	return FinalExchange;
}
