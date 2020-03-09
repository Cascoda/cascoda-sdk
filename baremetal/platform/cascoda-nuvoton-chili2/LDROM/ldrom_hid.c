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

#include <stdio.h>
#include <string.h>

#include "M2351.h"

#include "ca821x_endian.h"
#include "ldrom_hid.h"

// For M2351 Ver. C API
volatile ISP_INFO_T     g_ISPInfo  = {0};
volatile BL_USBD_INFO_T g_USBDInfo = {0};

#define TRIM_INIT (SYS_BASE + 0x10C)

void ProcessHardFault(void)
{
}
void SH_Return(void)
{
}

uint32_t CLK_GetPLLClockFreq(void)
{
	return 48000000;
}

uint32_t CLK_GetCPUFreq(void)
{
	return 48000000;
}

void SYS_Init(void)
{
	/*---------------------------------------------------------------------------------------------------------*/
	/* Init System Clock                                                                                       */
	/*---------------------------------------------------------------------------------------------------------*/
	/* Enable Internal RC 48MHz clock */
	CLK->PWRCTL |= CLK_PWRCTL_HIRC48EN_Msk;

	/* Waiting for Internal RC clock ready */
	while (!(CLK->STATUS & CLK_STATUS_HIRC48STB_Msk))
		;

	/* Switch HCLK clock source to Internal RC and HCLK source divide 1 */
	CLK->CLKSEL0 = (CLK->CLKSEL0 & (~CLK_CLKSEL0_HCLKSEL_Msk)) | CLK_CLKSEL0_HCLKSEL_HIRC48;
	CLK->CLKDIV0 = (CLK->CLKDIV0 & (~CLK_CLKDIV0_HCLKDIV_Msk)) | CLK_CLKDIV0_HCLK(1);
	/* Use HIRC48 as USB clock source */
	CLK->CLKSEL0 = (CLK->CLKSEL0 & (~CLK_CLKSEL0_USBSEL_Msk)) | CLK_CLKSEL0_USBSEL_HIRC48;
	CLK->CLKDIV0 = (CLK->CLKDIV0 & (~CLK_CLKDIV0_USBDIV_Msk)) | CLK_CLKDIV0_USB(1);
	/* Select USBD */
	SYS->USBPHY = (SYS->USBPHY & ~SYS_USBPHY_USBROLE_Msk) | SYS_USBPHY_OTGPHYEN_Msk | SYS_USBPHY_SBO_Msk;
	/* Enable IP clock */
	CLK->APBCLK0 |= CLK_APBCLK0_USBDCKEN_Msk;

	/*---------------------------------------------------------------------------------------------------------*/
	/* Init I/O Multi-function                                                                                 */
	/*---------------------------------------------------------------------------------------------------------*/
	/* USBD multi-function pins for VBUS, D+, D-, and ID pins */
	SYS->GPA_MFPH &=
	    ~(SYS_GPA_MFPH_PA12MFP_Msk | SYS_GPA_MFPH_PA13MFP_Msk | SYS_GPA_MFPH_PA14MFP_Msk | SYS_GPA_MFPH_PA15MFP_Msk);
	SYS->GPA_MFPH |= (SYS_GPA_MFPH_PA12MFP_USB_VBUS | SYS_GPA_MFPH_PA13MFP_USB_D_N | SYS_GPA_MFPH_PA14MFP_USB_D_P |
	                  SYS_GPA_MFPH_PA15MFP_USB_OTG_ID);
}

void ParseReboot(void)
{
	FMC_Open();
	FMC_ENABLE_AP_UPDATE();

	if (gRxBuffer.dfu_cmd.reboot_cmd.rebootMode)
		FMC_SetVectorPageAddr(FMC_LDROM_BASE);
	else
		FMC_SetVectorPageAddr(FMC_APROM_BASE);

	FMC_DISABLE_AP_UPDATE();
	FMC_Close();

	NVIC_SystemReset();
	while (1)
		;
}

void ParseErase(void)
{
	uint32_t startAddr = GETLE32(gRxBuffer.dfu_cmd.erase_cmd.startAddr);
	uint32_t eraseLen  = GETLE32(gRxBuffer.dfu_cmd.erase_cmd.eraseLen);

	//Check that startAddr and eraseLen are page-aligned
	if ((startAddr % FMC_FLASH_PAGE_SIZE) || (eraseLen % FMC_FLASH_PAGE_SIZE))
	{
		gTxBuffer.dfu_cmd.status_cmd.status = CA_ERROR_INVALID_ARGS;
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
	gTxBuffer.dfu_cmd.status_cmd.status = CA_ERROR_SUCCESS;
	TxReady();
}

void ParseWrite(void)
{
	uint32_t startAddr = GETLE32(gRxBuffer.dfu_cmd.write_cmd.startAddr);
	uint8_t  writeLen  = gRxBuffer.len - 5; //1 for dfu_cmdid, 4 for startAddr

	//Check that startAddr is page aligned and writeLen is word-aligned
	if ((startAddr % FMC_FLASH_PAGE_SIZE) || (writeLen % sizeof(uint32_t)))
	{
		gTxBuffer.dfu_cmd.status_cmd.status = CA_ERROR_INVALID_ARGS;
		TxReady();
		return;
	}

	writeLen /= 4;

	FMC_Open();
	FMC_ENABLE_AP_UPDATE();

	/* Write Flash */
	for (uint32_t i = 0; i < writeLen; ++i)
	{
		FMC_Write(startAddr, gRxBuffer.dfu_cmd.write_cmd.data[i]);
		startAddr += 4;
	}

	FMC_DISABLE_AP_UPDATE();
	FMC_Close();

	//Success
	gTxBuffer.dfu_cmd.status_cmd.status = CA_ERROR_SUCCESS;
	TxReady();
}

void ParseCheck(void)
{
	uint32_t startAddr = GETLE32(gRxBuffer.dfu_cmd.check_cmd.startAddr);
	uint32_t checkLen  = GETLE32(gRxBuffer.dfu_cmd.check_cmd.checkLen);
	uint32_t checksum  = GETLE32(gRxBuffer.dfu_cmd.check_cmd.checksum);
	uint32_t rval;

	//Check that startAddr and checkLen are page-aligned
	if ((startAddr % FMC_FLASH_PAGE_SIZE) || (checkLen % FMC_FLASH_PAGE_SIZE))
	{
		gTxBuffer.dfu_cmd.status_cmd.status = CA_ERROR_INVALID_ARGS;
		TxReady();
		return;
	}

	FMC_Open();
	FMC_ENABLE_AP_UPDATE();
	rval = FMC_GetChkSum(startAddr, checkLen);
	FMC_DISABLE_AP_UPDATE();
	FMC_Close();

	if (checksum == rval)
	{
		//Success
		gTxBuffer.dfu_cmd.status_cmd.status = CA_ERROR_SUCCESS;
		TxReady();
	}
	else
	{
		//Fail
		gTxBuffer.dfu_cmd.status_cmd.status = CA_ERROR_FAIL;
		TxReady();
	}
}

void ParseCmd(void)
{
	switch (gRxBuffer.dfu_cmdid)
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
	default:
		break;
	}
}

/*---------------------------------------------------------------------------------------------------------*/
/*  Main Function                                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
int32_t main(void)
{
	uint32_t                 u32TrimInit;
	volatile ISP_INFO_T *    pISPInfo;
	volatile BL_USBD_INFO_T *pUSBDInfo;

	pISPInfo  = &g_ISPInfo;
	pUSBDInfo = &g_USBDInfo;

	/* Unlock protected registers */
	SYS_UnlockReg();
	/* Init System, peripheral clock and multi-function I/O */
	SYS_Init();

	BL_USBDOpen(&gsInfo, NULL, NULL, (uint32_t *)pUSBDInfo);
	/* Endpoint configuration */
	HID_Init();
	NVIC_EnableIRQ(USBD_IRQn);
	BL_USBDStart();

	/* Backup default trim */
	u32TrimInit = M32(TRIM_INIT);
	/* Clear SOF */
	USBD->INTSTS = USBD_INTSTS_SOFIF_Msk;

	while (1)
	{
		/* Start USB trim if it is not enabled. */
		if ((SYS->TCTL48M & SYS_TCTL48M_FREQSEL_Msk) != 1)
		{
			/* Start USB trim only when SOF */
			if (USBD->INTSTS & USBD_INTSTS_SOFIF_Msk)
			{
				/* Clear SOF */
				USBD->INTSTS = USBD_INTSTS_SOFIF_Msk;
				/* Re-enable crystal-less */
				SYS->TCTL48M = 0x01;
				SYS->TCTL48M |= SYS_TCTL48M_REFCKSEL_Msk;
			}
		}

		/* Disable USB Trim when error */
		if (SYS->TISTS48M & (SYS_TISTS48M_CLKERRIF_Msk | SYS_TISTS48M_TFAILIF_Msk))
		{
			/* Init TRIM */
			M32(TRIM_INIT) = u32TrimInit;
			/* Disable crystal-less */
			SYS->TCTL48M = 0;
			/* Clear error flags */
			SYS->TISTS48M = SYS_TISTS48M_CLKERRIF_Msk | SYS_TISTS48M_TFAILIF_Msk;
			/* Clear SOF */
			USBD->INTSTS = USBD_INTSTS_SOFIF_Msk;
		}

		//If we have received something and the previous Tx has been fully sent.
		if (gRxBuffer.isReady && gTxBuffer.isReady)
		{
			if (gRxBuffer.cmdid == EVBME_DFU_CMD)
			{
				ParseCmd();
			}
			RxHandled();
		}
	}
}

void USBD_IRQHandler(void)
{
	uint32_t u32INTSTS = USBD_GET_INT_FLAG();

	//Process our endpoints first
	if ((u32INTSTS & USBD_INTSTS_USB))
	{
		if (u32INTSTS & USBD_INTSTS_EP3)
		{
			USBD_CLR_INT_FLAG(USBD_INTSTS_EP3);
			EP3_Handler();
		}
		if (u32INTSTS & USBD_INTSTS_EP2)
		{
			USBD_CLR_INT_FLAG(USBD_INTSTS_EP2);
			EP2_Handler();
		}
	}

	//Pass any unhandled endpoints (such as setup endpoint) to the MKROM implementation
	BL_ProcessUSBDInterrupt((uint32_t *)g_ISPInfo.pfnUSBDEP, (uint32_t *)&g_ISPInfo, (uint32_t *)&g_USBDInfo);
}
