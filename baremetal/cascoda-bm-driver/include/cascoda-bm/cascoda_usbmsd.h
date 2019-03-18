/**
 * @file   cascoda_usbmsd.h
 * @brief  USB MSD definitions
 * @author Peter Burnett
 * @date   27/08/14
 *//*
 * Copyright (C) 2016  Cascoda, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CASCODA_USBMSD_H
#define CASCODA_USBMSD_H

/******************************************************************************/
/****** Endpoints to be used                                             ******/
/******************************************************************************/
#define MSD_IN_EP_NO (2)
#define MSD_OUT_EP_NO (3)
#define MSD_CTRL_MAX_PKT_SIZE (64)
#define MSD_BULK_MAX_PKT_SIZE (64)

/******************************************************************************/
/****** Mass Storage Class-Specific Request Codes                        ******/
/******************************************************************************/

#define BULK_ONLY_MASS_STORAGE_RESET 0xFF
#define GET_MAX_LUN 0xFE

/******************************************************************************/
/****** Mass Storage Class Information                                   ******/
/******************************************************************************/

#define MASS_BUFFER_SIZE 48     /**!< Mass Storage command buffer size */
#define STORAGE_BUFFER_SIZE 512 /**!< Data transfer buffer size in 512 bytes alignment */
#define UDC_SECTOR_SIZE 512     /**!< logic sector size */

/******************************************************************************/
/****** Mass Storage State defines                                       ******/
/******************************************************************************/

#define BULK_CBW 0x00
#define BULK_IN 0x01
#define BULK_OUT 0x02
#define BULK_CSW 0x04
#define BULK_NORMAL 0xFF

/******************************************************************************/
/****** Mass Storage Signatures                                          ******/
/******************************************************************************/

#define CBW_SIGNATURE 0x43425355
#define CSW_SIGNATURE 0x53425355

/******************************************************************************/
/****** Mass Storage UFI command defines                                 ******/
/******************************************************************************/

#define UFI_TEST_UNIT_READY 0x00
#define UFI_REQUEST_SENSE 0x03
#define UFI_INQUIRY 0x12
#define UFI_MODE_SELECT_6 0x15
#define UFI_MODE_SENSE_6 0x1A
#define UFI_START_STOP 0x1B
#define UFI_PREVENT_ALLOW_MEDIUM_REMOVAL 0x1E
#define UFI_READ_FORMAT_CAPACITY 0x23
#define UFI_READ_CAPACITY 0x25
#define UFI_READ_10 0x28
#define UFI_WRITE_10 0x2A
#define UFI_VERIFY_10 0x2F
#define UFI_MODE_SELECT_10 0x55
#define UFI_MODE_SENSE_10 0x5A

/******************************************************************************/
/****** Mass Storage Wrappers                                            ******/
/******************************************************************************/

/** USB Mass Storage Class - Command Block Wrapper Structure */
struct CBW // Set from Host to Device
{
	u32_t dCBWSignature;          //!< 0x43425355
	u32_t dCBWTag;                //!< Associates this CBW with CSW sent in reply
	u32_t dCBWDataTransferLength; //!< Data length in data transfer phase
	u8_t  bmCBWFlags;             //!< b7 = 0 Host->Device; 1 Device->Host
	u8_t  bCBWLUN;                //!< logical unit (0)
	u8_t  bCBWCBLength;           //!< Length of following command field
	u8_t  u8OPCode;               //!< command field op code
	u8_t  u8LUN;                  //!< command field lun
	u8_t  au8Data[14];            //!< rest of command
};

/** USB Mass Storage Class - Command Status Wrapper Structure */
struct CSW // Sent from Device to Host
{
	u32_t dCSWSignature;   //!< 0x53425355
	u32_t dCSWTag;         //!< From dCBWTag in CBW
	u32_t dCSWDataResidue; //!< Length of data left unprocessed
	u8_t  bCSWStatus;      //!< 0 - passed; 1 - failed; 2 - phase error
};

// For the above CBW and CSW the sequence is as follows:
// 1. The host sends CBW
// 2. Optionally data is transferred in either direction as indicated by bmCBWFlags
// 3. The device sends CSW

/******************************************************************************/
/****** USB Mass Storage Class Information Structure                     ******/
/******************************************************************************/

typedef struct DRVUSBD_MASS_STRUCT
{
	u8_t BulkState;
	u8_t SenseKey[4];
	u8_t preventflag;
	u8_t Size;
	u8_t F_DATA_FLASH_LUN;

	u32_t DataFlashStartAddr;
	u32_t Address;
	u32_t Length;
	u32_t LbaAddress;
	u32_t BytesInStorageBuf;
	u32_t BulkBuf0;
	u32_t BulkBuf1;

	i32_t dataFlashTotalSectors;

	/* CBW/CSW variables */
	struct CBW sCBW;
	struct CSW sCSW;

} STR_USBD_MASS_T;

/** Configuration descriptor for MSD device */
typedef struct MsdConfigurationDescriptor
{
	UsbConfigurationDescriptor_t Config;
	UsbInterfaceDescriptor_t     Interface;
	//    UsbMSDDescriptor_t                  Msd;
	UsbEndpointDescriptor_t EP0;
	UsbEndpointDescriptor_t EP1;
} MsdConfigurationDescriptor_t;

/******************************************************************************/
/****** MSD Request codes                                                ******/
/******************************************************************************/
#define MSD_GET_REPORT 0x01
#define MSD_GET_IDLE 0x02
#define MSD_GET_PROTOCOL 0x03
#define MSD_SET_REPORT 0x09
#define MSD_SET_IDLE 0x0A
#define MSD_SET_PROTOCOL 0x0B

extern const UsbDeviceDescriptor_t        MsdDeviceDescriptor;
extern const MsdConfigurationDescriptor_t MsdConfDesc;
extern u8_t                               USB_StringDescs[];
extern const u8_t                         MsdInquiryID[];
extern const u8_t                         MsdMS6[];
extern u8_t                               MsdModePage_01[];
extern u8_t                               MsdModePage_05[];
extern u8_t                               MsdModePage_1B[];
extern u8_t                               MsdModePage_1C[];

#endif // CASCODA_USBMSD_H
