#include "M2351.h"
#include "cascoda_secure.h"

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_ENTROPY_HARDWARE_ALT)
int mbedtls_hardware_poll( void *aData,
                           unsigned char *aOutput, size_t aInLen, size_t *aOutLen )
{
    (void) aData;

    CHILI_EnableTRNGClk();
    // Select the highest frequency clock prescaler, so that it works
    // regardless of peripheral clock frequency
    TRNG->CTL &= ~(0xf << TRNG_CTL_CLKP_Pos);
    // Activate TRNG
    TRNG->ACT |= TRNG_ACT_ACT_Msk;
    // Wait for TRNG to finish initialising - READY bit goes high
    while ( !(TRNG->CTL & TRNG_CTL_READY_Msk) )
        ;

    // Start generating random numbers
    TRNG->CTL |= TRNG_CTL_TRNGEN_Msk;
    for (size_t i=0; i < aInLen; ++i)
    {
        // Wait for the random number to be generated
        while ( !(TRNG->CTL & TRNG_CTL_DVIF_Msk) )
            ;
        aOutput[i] = TRNG->DATA & TRNG_DATA_DATA_Msk;
    }
    *aOutLen = aInLen;

    // Disable the random number generator
    TRNG->CTL &= ~TRNG_CTL_TRNGEN_Msk;
    TRNG->ACT &= ~TRNG_ACT_ACT_Msk;
    CHILI_DisableTRNGClk();

    return 0;
}
#endif

void targetm2351_hwpoll_register(void)
{
}
