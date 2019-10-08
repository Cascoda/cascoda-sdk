
#include <stdio.h>
/* Platform */
#include "M2351.h"
#include "i2c.h"
#include "spi.h"
#include "sys.h"
/* Cascoda */
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_sensorif.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda_chili.h"

/****************************************************************************/
/* These values must also be updated in cascoda_sensorif_m2351.c            */
/****************************************************************************/

/* set I2C interface number (0/1/2) */
#define SENSORIF_I2CNUM 1

/* I2C module */
#if (SENSORIF_I2CNUM == 0)
#define SENSORIF_I2CIF I2C0
#elif (SENSORIF_I2CNUM == 1)
#define SENSORIF_I2CIF I2C1
#elif (SENSORIF_I2CNUM == 2)
#define SENSORIF_I2CIF I2C2
#else
#error "sensorif I2C module not valid"
#endif

/* set SPI interface number (1/2)*/
#define SENSORIF_SPINUM 1

/* SPI module */
#if (SENSORIF_SPINUM == 1)
#define SENSORIF_SPIIF SPI1
#elif (SENSORIF_SPINUM == 2)
#define SENSORIF_SPIIF SPI2
#else
#error "sensorif SPI module not valid"
#endif

__NONSECURE_ENTRY void SENSORIF_I2C_Init(void)
{
	/* enable I2C peripheral clock */
#if (SENSORIF_I2CNUM == 0)
	CLK_EnableModuleClock(I2C0_MODULE);
#elif (SENSORIF_I2CNUM == 1)
	CLK_EnableModuleClock(I2C1_MODULE);
#else
	CLK_EnableModuleClock(I2C2_MODULE);
#endif

	/* SDA/SCL port configurations */
#if (SENSORIF_I2CNUM == 0)
	/* re-config PB.5 and PB.4 */
	GPIO_ENABLE_DIGITAL_PATH(PB, BIT5);
	GPIO_ENABLE_DIGITAL_PATH(PB, BIT4);
	GPIO_DISABLE_DEBOUNCE(PB, BIT5);
	GPIO_DISABLE_DEBOUNCE(PB, BIT4);
#if (SENSORIF_INT_PULLUPS)
	GPIO_SetPullCtl(PB, BIT5, GPIO_PUSEL_PULL_UP);
	GPIO_SetPullCtl(PB, BIT4, GPIO_PUSEL_PULL_UP);
#else
	GPIO_SetPullCtl(PB, BIT5, GPIO_PUSEL_DISABLE);
	GPIO_SetPullCtl(PB, BIT4, GPIO_PUSEL_DISABLE);
#endif
	/* initialise PB MFP for I2C0 SDA and SCL */
	/* PB.5 = I2C0 SCL */
	/* PB.4 = I2C0 SDA */
	SYS->GPB_MFPL &= ~(SYS_GPB_MFPL_PB4MFP_Msk | SYS_GPB_MFPL_PB5MFP_Msk);
	SYS->GPB_MFPL |= (SYS_GPB_MFPL_PB4MFP_I2C0_SDA | SYS_GPB_MFPL_PB5MFP_I2C0_SCL);
#elif (SENSORIF_I2CNUM == 1)
	/* re-config PB.1 and PB.0 */
	GPIO_ENABLE_DIGITAL_PATH(PB, BIT1);
	GPIO_ENABLE_DIGITAL_PATH(PB, BIT0);
	GPIO_DISABLE_DEBOUNCE(PB, BIT1);
	GPIO_DISABLE_DEBOUNCE(PB, BIT0);
#if (SENSORIF_INT_PULLUPS)
	GPIO_SetPullCtl(PB, BIT1, GPIO_PUSEL_PULL_UP);
	GPIO_SetPullCtl(PB, BIT0, GPIO_PUSEL_PULL_UP);
#else
	GPIO_SetPullCtl(PB, BIT1, GPIO_PUSEL_DISABLE);
	GPIO_SetPullCtl(PB, BIT0, GPIO_PUSEL_DISABLE);
#endif
	/* initialise PB MFP for I2C1 SDA and SCL */
	/* PB.1 = I2C1 SCL */
	/* PB.0 = I2C1 SDA */
	SYS->GPB_MFPL &= ~(SYS_GPB_MFPL_PB0MFP_Msk | SYS_GPB_MFPL_PB1MFP_Msk);
	SYS->GPB_MFPL |= (SYS_GPB_MFPL_PB0MFP_I2C1_SDA | SYS_GPB_MFPL_PB1MFP_I2C1_SCL);
#else
	/* re-config PB.13 and PB.12 */
	GPIO_ENABLE_DIGITAL_PATH(PB, BIT13);
	GPIO_ENABLE_DIGITAL_PATH(PB, BIT12);
	GPIO_DISABLE_DEBOUNCE(PB, BIT13);
	GPIO_DISABLE_DEBOUNCE(PB, BIT12);
#if (SENSORIF_INT_PULLUPS)
	GPIO_SetPullCtl(PB, BIT13, GPIO_PUSEL_PULL_UP);
	GPIO_SetPullCtl(PB, BIT12, GPIO_PUSEL_PULL_UP);
#else
	GPIO_SetPullCtl(PB, BIT13, GPIO_PUSEL_DISABLE);
	GPIO_SetPullCtl(PB, BIT12, GPIO_PUSEL_DISABLE);
#endif
	/* initialise PB MFP for I2C2 SDA and SCL */
	/* PB.13 = I2C2 SCL */
	/* PB.12 = I2C2 SDA */
	SYS->GPB_MFPL &= ~(SYS_GPB_MFPH_PB12MFP_Msk | SYS_GPB_MFPH_PB13MFP_Msk);
	SYS->GPB_MFPL |= (SYS_GPB_MFPH_PB12MFP_I2C2_SDA | SYS_GPB_MFPH_PB13MFP_I2C2_SCL);
#endif

	/* reset I2C module */
#if (SENSORIF_I2CNUM == 0)
	SYS_ResetModule(I2C0_RST);
#elif (SENSORIF_I2CNUM == 1)
	SYS_ResetModule(I2C1_RST);
#else
	SYS_ResetModule(I2C2_RST);
#endif

	/* enable I2C */
	I2C_Open(SENSORIF_I2CIF, SENSORIF_I2C_CLK_FREQUENCY);
}

__NONSECURE_ENTRY void SENSORIF_I2C_Deinit(void)
{
	I2C_DisableInt(SENSORIF_I2CIF);
	I2C_Close(SENSORIF_I2CIF);
#if (SENSORIF_I2CNUM == 0)
	CLK_DisableModuleClock(I2C0_MODULE);
#elif (SENSORIF_I2CNUM == 1)
	CLK_DisableModuleClock(I2C1_MODULE);
#else
	CLK_DisableModuleClock(I2C2_MODULE);
#endif
}

__NONSECURE_ENTRY void SENSORIF_SPI_Init(void)
{
	/* enable SPI peripheral clock */
#if (SENSORIF_SPINUM == 1)
	CLK_EnableModuleClock(SPI1_MODULE);
#elif (SENSORIF_SPINUM == 2)
	CLK_EnableModuleClock(SPI2_MODULE);
#endif

	/* MOSI/CLK/SS port configurations (Half-duplex mode so no MISO)*/
#if (SENSORIF_SPINUM == 1)
	/* re-config PB.4, PB.3 and PB.2 */
	GPIO_ENABLE_DIGITAL_PATH(PB, BIT4);
	GPIO_ENABLE_DIGITAL_PATH(PB, BIT3);
	//GPIO_ENABLE_DIGITAL_PATH(PB, BIT2); /* Uncomment if using SS */
	GPIO_DISABLE_DEBOUNCE(PB, BIT4);
	GPIO_DISABLE_DEBOUNCE(PB, BIT3);
	//GPIO_DISABLE_DEBOUNCE(PB, BIT2); /* Uncomment if using SS */
	GPIO_SetPullCtl(PB, BIT4, GPIO_PUSEL_DISABLE);
	GPIO_SetPullCtl(PB, BIT3, GPIO_PUSEL_DISABLE);
	//GPIO_SetPullCtl(PB, BIT2, GPIO_PUSEL_DISABLE);
	/* initialise PB MFP for SPI1 MOSI, CLK, and SS */
	/* PB.4 = SPI1 MOSI */
	/* PB.3 = SPI1 CLK */
	SYS->GPB_MFPL = (SYS->GPB_MFPL & (~SYS_GPB_MFPL_PB4MFP_Msk)) | SYS_GPB_MFPL_PB4MFP_SPI1_MOSI;
	SYS->GPB_MFPL = (SYS->GPB_MFPL & (~SYS_GPB_MFPL_PB3MFP_Msk)) | SYS_GPB_MFPL_PB3MFP_SPI1_CLK;
	/* PB.2 = SPI1 SS */
	//SYS->GPB_MFPL = (SYS->GPB_MFPL & (~SYS_GPB_MFPL_PB2MFP_Msk)) | SYS_GPB_MFPL_PB2MFP_GPIO;
#elif (SENSORIF_SPINUM == 2)
	/* re-config PA.15, PA.13 and PA.12 */
	GPIO_ENABLE_DIGITAL_PATH(PA, BIT15);
	GPIO_ENABLE_DIGITAL_PATH(PA, BIT13);
	// GPIO_ENABLE_DIGITAL_PATH(PA, BIT12); /* Uncomment if using SS */
	GPIO_DISABLE_DEBOUNCE(PA, BIT15);
	GPIO_DISABLE_DEBOUNCE(PA, BIT13);
	// GPIO_DISABLE_DEBOUNCE(PA, BIT12); /* Uncomment if using SS */
	GPIO_SetPullCtl(PA, BIT15, GPIO_PUSEL_DISABLE);
	GPIO_SetPullCtl(PA, BIT13, GPIO_PUSEL_DISABLE);
	// GPIO_SetPullCtl(PA, BIT12, GPIO_PUSEL_DISABLE); /* Uncomment if using SS */
	/* initialise PA MFP for SPI2 MOSI, CLK, and SS */
	/* PA.15 = SPI2 MOSI */
	/* PA.13 = SPI2 CLK */
	SYS->GPA_MFPH = (SYS->GPA_MFPH & (~SYS_GPA_MFPH_PA15MFP_Msk)) | SYS_GPA_MFPH_PA15MFP_SPI2_MOSI;
	SYS->GPA_MFPH = (SYS->GPA_MFPH & (~SYS_GPA_MFPH_PA13MFP_Msk)) | SYS_GPA_MFPH_PA13MFP_SPI2_CLK;
	/* PA.12 = SPI2 SS */
	//SYS->GPA_MFPH = (SYS->GPA_MFPH & (~SYS_GPA_MFPH_PA12MFP_Msk)) | SYS_GPA_MFPH_PA12MFP_SPI2_SS;
#endif

	/* reset SPI module */
#if (SENSORIF_SPINUM == 1)
	SYS_ResetModule(SPI1_RST);
#elif (SENSORIF_SPINUM == 2)
	SYS_ResetModule(SPI2_RST);
#endif

	/* clear transmit fifo*/
	SPI_ClearTxFIFO(SENSORIF_SPIIF);

	/* enable SPI */
	SPI_Open(SENSORIF_SPIIF, SPI_MASTER, SPI_MODE_0, SENSORIF_SPI_DATA_WIDTH, SENSORIF_SPI_CLK_FREQUENCY);

	/* Set SPI commmunication to half-duplex mode with output data direction */
	SENSORIF_SPIIF->CTL |= (SPI_CTL_HALFDPX_Msk | SPI_CTL_DATDIR_Msk);
}

__NONSECURE_ENTRY void SENSORIF_SPI_Deinit(void)
{
	SPI_DisableInt(SENSORIF_SPIIF, 0x3FF);
	SPI_Close(SENSORIF_SPIIF);
#if (SENSORIF_SPINUM == 1)
	CLK_DisableModuleClock(SPI1_MODULE);
#elif (SENSORIF_SPINUM == 2)
	CLK_DisableModuleClock(SPI2_MODULE);
#endif
}
