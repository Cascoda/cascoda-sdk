/*
 *  Copyright (c) 2023, Cascoda Ltd.
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

#include <stdio.h>
#include <string.h>

#include "M2351.h"

#include "cascoda-util/cascoda_hash.h"
#include "ca821x_endian.h"
#include "cascoda_chili_config.h"
#include "ldrom_uart.h"

void ProcessHardFault(void)
{
}
void SH_Return(void)
{
}

void UART_SetClockSource(void)
{
#if (UART_CHANNEL == 0)
	CLK->CLKSEL1 = (CLK->CLKSEL1 & (~CLK_CLKSEL1_UART0SEL_Msk)) | CLK_CLKSEL1_UART0SEL_HIRC;
#elif (UART_CHANNEL == 1)
	CLK->CLKSEL1  = (CLK->CLKSEL1 & (~CLK_CLKSEL1_UART1SEL_Msk)) | CLK_CLKSEL1_UART1SEL_HIRC;
#elif (UART_CHANNEL == 2)
	CLK->CLKSEL3  = (CLK->CLKSEL3 & (~CLK_CLKSEL3_UART2SEL_Msk)) | CLK_CLKSEL3_UART2SEL_HIRC;
#elif (UART_CHANNEL == 4)
	CLK->CLKSEL3  = (CLK->CLKSEL3 & (~CLK_CLKSEL3_UART4SEL_Msk)) | CLK_CLKSEL3_UART4SEL_HIRC;
#elif (UART_CHANNEL == 5)
	CLK->CLKSEL3  = (CLK->CLKSEL3 & (~CLK_CLKSEL3_UART5SEL_Msk)) | CLK_CLKSEL3_UART5SEL_HIRC;
#endif
}

void UART_SetMFP(void)
{
#if (UART_CHANNEL == 0)
	SYS->GPB_MFPH = (SYS->GPB_MFPH & (~(UART0_RXD_PB12_Msk | UART0_TXD_PB13_Msk))) | UART0_RXD_PB12 | UART0_TXD_PB13;
#elif (UART_CHANNEL == 1)
	SYS->GPB_MFPL = (SYS->GPB_MFPL & (~(UART1_RXD_PB2_Msk | UART1_TXD_PB3_Msk))) | UART1_RXD_PB2 | UART1_TXD_PB3;
#elif (UART_CHANNEL == 2)
	SYS->GPB_MFPL = (SYS->GPB_MFPL & (~(UART2_RXD_PB0_Msk | UART2_TXD_PB1_Msk))) | UART2_RXD_PB0 | UART2_TXD_PB1;
#elif (UART_CHANNEL == 4)
	SYS->GPA_MFPH = (SYS->GPA_MFPH & (~(UART4_RXD_PA13_Msk | UART4_TXD_PA12_Msk))) | UART4_RXD_PA13 | UART4_TXD_PA12;
#elif (UART_CHANNEL == 5)
	SYS->GPB_MFPL = (SYS->GPB_MFPL & (~(UART5_RXD_PB4_Msk | UART5_TXD_PB5_Msk))) | UART5_RXD_PB4 | UART5_TXD_PB5;
#endif
}

void SYS_Init(void)
{
	/*---------------------------------------------------------------------------------------------------------*/
	/* Init System Clock                                                                                       */
	/*---------------------------------------------------------------------------------------------------------*/

	/* Enable HIRC clock */
	CLK->PWRCTL |= CLK_PWRCTL_HIRCEN_Msk;

	/* Wait for HIRC clock ready */
	while (!(CLK->STATUS & CLK_STATUS_HIRCSTB_Msk))
		;

	/* Set HCLK source to HIRC first */
	CLK->CLKSEL0 = (CLK->CLKSEL0 & (~CLK_CLKSEL0_HCLKSEL_Msk)) | CLK_CLKSEL0_HCLKSEL_HIRC;

	/* Enable PLL */
	CLK->PLLCTL = CLK_PLLCTL_128MHz_HIRC;

	/* Wait for PLL stable */
	while (!(CLK->STATUS & CLK_STATUS_PLLSTB_Msk))
		;

	/* Select HCLK clock source as PLL and HCLK source divider as 2 */
	CLK->CLKDIV0 = (CLK->CLKDIV0 & (~CLK_CLKDIV0_HCLKDIV_Msk)) | CLK_CLKDIV0_HCLK(2);
	CLK->CLKSEL0 = (CLK->CLKSEL0 & (~CLK_CLKSEL0_HCLKSEL_Msk)) | CLK_CLKSEL0_HCLKSEL_PLL;

	/* Update System Core Clock */
	PllClock        = 128000000;
	SystemCoreClock = 128000000 / 2;
	CyclesPerUs     = SystemCoreClock / 1000000; /* For CLK_SysTickDelay() */

	/* Enable UART module clock */
	CLK->APBCLK0 |= CLK_APBCLK_MASK;

	/* Select UART module clock source */
	UART_SetClockSource();

	/*---------------------------------------------------------------------------------------------------------*/
	/* Init I/O Multi-function                                                                                 */
	/*---------------------------------------------------------------------------------------------------------*/

	/* Set multi-function pins for UART RXD and TXD */
	UART_SetMFP();
}

void ParseReboot(void)
{
	FMC_Open();

	/**
	 * Here we set the vector mapping, so that upon the
	 * software triggered System Reset, the system boots
	 * into either LDROM or APROM depending on the setting.
	 * This setting is volatile, unlike CONFIG0 BS.
	 */

	FMC->ISPCTL |= FMC_ISPCTL_BS_Msk; //We set to LDROM boot mode so vector mapping takes effect.
	if (gRxBuffer.data.dfu.dfu_cmd.reboot_cmd.rebootMode)
	{
		FMC_SetVectorPageAddr(FMC_LDROM_BASE);
	}
	else
	{
		FMC_SetVectorPageAddr(FMC_APROM_BASE);
	}

	FMC_Close();

	NVIC_SystemReset();
	while (1)
		;
}

void ParseErase(void)
{
	uint32_t startAddr = GETLE32(gRxBuffer.data.dfu.dfu_cmd.erase_cmd.startAddr);
	uint32_t eraseLen  = GETLE32(gRxBuffer.data.dfu.dfu_cmd.erase_cmd.eraseLen);

	//Check that startAddr and eraseLen are page-aligned
	if ((startAddr % FMC_FLASH_PAGE_SIZE) || (eraseLen % FMC_FLASH_PAGE_SIZE))
	{
		gTxBuffer.data.dfu.dfu_cmd.status_cmd.status = CA_ERROR_INVALID_ARGS;
		TxReady();
		return;
	}

	FMC_Open();
	FMC_ENABLE_AP_UPDATE();

	/* Erase page by page */
	while (eraseLen)
	{
		FMC_Erase(startAddr);
		eraseLen -= FMC_FLASH_PAGE_SIZE;
		startAddr += FMC_FLASH_PAGE_SIZE;
	}

	FMC_DISABLE_AP_UPDATE();
	FMC_Close();

	//Success
	gTxBuffer.data.dfu.dfu_cmd.status_cmd.status = CA_ERROR_SUCCESS;
	TxReady();
}

void ParseWrite(void)
{
	uint32_t startAddr = GETLE32(gRxBuffer.data.dfu.dfu_cmd.write_cmd.startAddr);
	uint8_t  writeLen  = gRxBuffer.len - 5; //1 for dfu_cmdid, 4 for startAddr

	//Check that startAddr is word aligned and writeLen is word-aligned
	if ((startAddr % sizeof(uint32_t)) || (writeLen % sizeof(uint32_t)))
	{
		gTxBuffer.data.dfu.dfu_cmd.status_cmd.status = CA_ERROR_INVALID_ARGS;
		TxReady();
		return;
	}

	writeLen /= sizeof(uint32_t);

	FMC_Open();
	FMC_ENABLE_AP_UPDATE();

	/* Write Flash */
	for (uint32_t i = 0; i < writeLen; ++i)
	{
		FMC_Write(startAddr, gRxBuffer.data.dfu.dfu_cmd.write_cmd.data[i]);
		startAddr += 4;
	}

	FMC_DISABLE_AP_UPDATE();
	FMC_Close();

	//Success
	gTxBuffer.data.dfu.dfu_cmd.status_cmd.status = CA_ERROR_SUCCESS;
	TxReady();
}

void ParseCheck(void)
{
	uint32_t startAddr = GETLE32(gRxBuffer.data.dfu.dfu_cmd.check_cmd.startAddr);
	uint32_t checkLen  = GETLE32(gRxBuffer.data.dfu.dfu_cmd.check_cmd.checkLen);
	uint32_t checksum  = GETLE32(gRxBuffer.data.dfu.dfu_cmd.check_cmd.checksum);
	uint32_t rval;

	//Check that startAddr and checkLen are page-aligned
	if ((startAddr % FMC_FLASH_PAGE_SIZE) || (checkLen % FMC_FLASH_PAGE_SIZE))
	{
		gTxBuffer.data.dfu.dfu_cmd.status_cmd.status = CA_ERROR_INVALID_ARGS;
		TxReady();
		return;
	}

	FMC_Open();
	rval = FMC_GetChkSum(startAddr, checkLen);
	FMC_Close();

	if (checksum == rval)
	{
		//Success
		gTxBuffer.data.dfu.dfu_cmd.status_cmd.status = CA_ERROR_SUCCESS;
		TxReady();
	}
	else
	{
		//Fail
		gTxBuffer.data.dfu.dfu_cmd.status_cmd.status = CA_ERROR_FAIL;
		TxReady();
	}
}

void ParseBootMode(void)
{
	uint32_t config[4];

	FMC_Open();
	FMC_ENABLE_AP_UPDATE();
	FMC_ENABLE_CFG_UPDATE();

	FMC_ReadConfig(config, 4);

	/**
	 * Here we set bit 7 (BS) of CONFIG0 to set the default
	 * boot source of the chip. CONFIG0 is nonvolatile, but
	 * BS can be overridden by FMC_ISPCTL_BS when software
	 * reboot takes place.
	 */

	if (gRxBuffer.data.dfu.dfu_cmd.reboot_cmd.rebootMode)
	{
		config[0] &= ~(1 << 7); //Boot into LDROM
	}
	else
	{
		config[0] |= (1 << 7);        //Boot into APROM
		FMC_Erase(FMC_USER_CONFIG_0); //Page-erase, so we have to re-write all configs
		FMC_Write(FMC_USER_CONFIG_1, config[1]);
		FMC_Write(FMC_USER_CONFIG_2, config[2]);
		FMC_Write(FMC_USER_CONFIG_3, config[3]);
	}

	FMC_Write(FMC_USER_CONFIG_0, config[0]);
	FMC_DISABLE_CFG_UPDATE();
	FMC_DISABLE_AP_UPDATE();
	FMC_Close();

	gTxBuffer.data.dfu.dfu_cmd.status_cmd.status = CA_ERROR_SUCCESS;
	TxReady();
}

void ParseCmd(void)
{
	gTxBuffer.cmdid              = EVBME_DFU_CMD;
	gTxBuffer.len                = 2;
	gTxBuffer.data.dfu.dfu_cmdid = DFU_STATUS;
	switch (gRxBuffer.data.dfu.dfu_cmdid)
	{
	case DFU_REBOOT:
		ParseReboot();
		break;
	case DFU_ERASE:
		ParseErase();
		break;
	case DFU_WRITE:
		ParseWrite();
		break;
	case DFU_CHECK:
		ParseCheck();
		break;
	case DFU_BOOTMODE:
		ParseBootMode();
		break;
	default:
		break;
	}
}

uint64_t HASH_fnv1a_64(const void *data_in, size_t num_bytes)
{
	uint64_t       hash = basis64;
	const uint8_t *data = data_in;

	for (size_t i = 0; i < num_bytes; i++)
	{
		hash = (data[i] ^ hash) * prime64;
	}
	return hash;
}

void GetUID(uint32_t *uid_out)
{
#if defined(__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3L)
	// Check that the output memory is actually nonsecure.
	uid_out = cmse_check_address_range(uid_out, 3 * sizeof(uint32_t), CMSE_NONSECURE);
#endif
	if (uid_out == NULL)
		return;

	FMC_Open();
	uid_out[0] = FMC_ReadUID(0);
	uid_out[1] = FMC_ReadUID(1);
	uid_out[2] = FMC_ReadUID(2);
	FMC_Close();
}

uint64_t GetUniqueId(void)
{
	uint64_t rval = 0;
	uint32_t UID[3];

	GetUID(UID);
	rval = HASH_fnv1a_64(&UID, sizeof(UID));

	return rval;
}

void ParseEvbmeSerialNo(void)
{
	uint8_t                   internalbuf[30]; //Only used to size the flexible array member at end of below struct.
	struct evbme_get_confirm *cnf = (struct evbme_get_confirm *)internalbuf;

	uint64_t serialNo                     = GetUniqueId();
	gTxBuffer.data.evbme.cnf.attributeLen = sizeof(serialNo);
	PUTLE64(serialNo, cnf->attribute);
	memcpy(gTxBuffer.data.evbme.cnf.attribute, cnf->attribute, gTxBuffer.data.evbme.cnf.attributeLen);

	gTxBuffer.len =
	    gTxBuffer.data.evbme.cnf.attributeLen + 3; // + 3 to account for status, attributeId, and attributeLen
	gTxBuffer.data.evbme.cnf.status      = CA_ERROR_SUCCESS;
	gTxBuffer.data.evbme.cnf.attributeId = gRxBuffer.data.evbme.req.attributeId;
}

void ParseEvbmeString(const char *str)
{
	uint8_t                   internalbuf[30]; //Only used to size the flexible array member at end of below struct.
	struct evbme_get_confirm *cnf = (struct evbme_get_confirm *)internalbuf;

	gTxBuffer.data.evbme.cnf.attributeLen = strlen(str) + 1;

	memcpy(cnf->attribute, str, gTxBuffer.data.evbme.cnf.attributeLen);
	cnf->attribute[gTxBuffer.data.evbme.cnf.attributeLen - 1] = '\0';
	memcpy(gTxBuffer.data.evbme.cnf.attribute, cnf->attribute, gTxBuffer.data.evbme.cnf.attributeLen);

	gTxBuffer.len =
	    gTxBuffer.data.evbme.cnf.attributeLen + 3; // + 3 to account for status, attributeId, and attributeLen
	gTxBuffer.data.evbme.cnf.status      = CA_ERROR_SUCCESS;
	gTxBuffer.data.evbme.cnf.attributeId = gRxBuffer.data.evbme.req.attributeId;
}

void ParseEvbmeAppString(void)
{
	ParseEvbmeString("DFU");
}

void ParseEvbmePlatString(void)
{
	ParseEvbmeString("Chili2");
}

void ParseEvbmeGet(void)
{
	gTxBuffer.SofPkt = 0xDE;
	gTxBuffer.cmdid  = EVBME_GET_CONFIRM;

	switch (gRxBuffer.data.evbme.req.attributeId)
	{
	case EVBME_SERIALNO:
		ParseEvbmeSerialNo();
		break;
	case EVBME_APPSTRING:
		ParseEvbmeAppString();
		break;
	case EVBME_PLATSTRING:
		ParseEvbmePlatString();
		break;
	default:
		gTxBuffer.len                   = 1;
		gTxBuffer.data.evbme.cnf.status = CA_ERROR_INVALID_STATE;
		break;
	}
	TxReady();
}

/*---------------------------------------------------------------------------------------------------------*/
/*  Main Function                                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
int32_t main(void)
{
	/* Unlock protected registers */
	SYS_UnlockReg();

	/* Init System, peripheral clock and multi-function I/O */
	SYS_Init();

	/* Init UART */
	UART_Init();

	/* Enable ISP */
	CLK->AHBCLK |= CLK_AHBCLK_ISPCKEN_Msk;
	FMC->ISPCTL |= FMC_ISPCTL_ISPEN_Msk;

	while (1)
	{
		//If we have received something
		if (gRxBuffer.isReady)
		{
			if (gRxBuffer.cmdid == EVBME_DFU_CMD)
			{
				ParseCmd();
			}
			else if (gRxBuffer.cmdid == EVBME_GET_REQUEST)
			{
				ParseEvbmeGet();
			}
			RxHandled();
		}
	}
}