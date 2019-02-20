/**
 * @file test.c
 * @brief Simple test program for ca821x api
 *//*
 * Copyright (C) 2017  Cascoda, Ltd.
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
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ca821x_api.h"

static int sReturnValue;

/* Test parameters */
#define TEST_DSTADDR 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88
#define TEST_CHANNEL (13)
#define TEST_PANID 0x5C, 0xCA

#define TEST_MSDULENGTH (4)
#define TEST_MSDUHANDLE (0xAA)
#define TEST_MSDU 0xDE, 0xAD, 0xBE, 0xEF

#define TEST_SECURITYLEVEL (7)
#define TEST_KEYIDMODE (3)
#define TEST_KEYSOURCE 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00
#define TEST_KEYINDEX (0x01)

#define TEST_PIBATTRIBUTE (phyCurrentChannel)
#define TEST_PIBATTRIBUTEVALUE (TEST_CHANNEL)
#define TEST_HWATTRIBUTE (HWME_EDTHRESHOLD)
#define TEST_HWATTRIBUTEVALUE (100)
#define TEST_TDMEATTRIBUTE (TDME_CHANNEL)
#define TEST_TDMEATTRIBUTEVALUE (TEST_CHANNEL)

#define TEST_HAESMODE (HAES_MODE_ENCRYPT)
#define TEST_HAESDATA 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff

#define TEST_SFRPAGE (1)
#define TEST_SFRADDRESS (0xBF)
#define TEST_SFRVALUE (0xAE)

#define TEST_TESTMODE (TDME_TEST_IDLE)

#define TEST_SEQUENCENUM (0)

/* Colour codes for printf */
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

/** MCPS-DATA.request reference buffer */
uint8_t ref_mcps_data_req[] = {
    SPI_MCPS_DATA_REQUEST, /* CmdId */
    26 + TEST_MSDULENGTH,  /* Packet Length */
    MAC_MODE_SHORT_ADDR,   /* SrcAddrMode */
    MAC_MODE_LONG_ADDR,    /* DstAddrMode */
    TEST_PANID,            /* DstPANId */
    TEST_DSTADDR,          /* DstAddr */
    TEST_MSDULENGTH,       /* MsduLength */
    TEST_MSDUHANDLE,       /* MsduHandle */
    0x00,                  /* TxOptions */
    TEST_MSDU,             /* Msdu */
    TEST_SECURITYLEVEL,    /* SecurityLevel */
    TEST_KEYIDMODE,        /* KeyIdMode */
    TEST_KEYSOURCE,        /* KeySource */
    TEST_KEYINDEX          /* KeyIndex */
};

/** MCPS-PURGE.request reference buffer */
uint8_t ref_mcps_purge_req[] = {
    SPI_MCPS_PURGE_REQUEST, /* CmdId */
    1,                      /* Packet Length */
    TEST_MSDUHANDLE,        /* MsduHandle */
};

/** MCPS-PURGE.confirm reference buffer */
uint8_t ref_mcps_purge_cnf[] = {
    SPI_MCPS_PURGE_CONFIRM, /* CmdId */
    2,                      /* Packet Length */
    TEST_MSDUHANDLE,        /* MsduHandle */
    MAC_SUCCESS             /* Status */
};

/** MLME-ASSOCIATE.request reference buffer */
uint8_t ref_mlme_associate_req[] = {
    SPI_MLME_ASSOCIATE_REQUEST, /* CmdId */
    24,                         /* Packet Length */
    TEST_CHANNEL,               /* LogicalChannel */
    MAC_MODE_LONG_ADDR,         /* CoordAddrMode */
    TEST_PANID,                 /* CoordPANId */
    TEST_DSTADDR,               /* CoordAddress */
    0x00,                       /* CapabilityInformation */
    TEST_SECURITYLEVEL,         /* SecurityLevel */
    TEST_KEYIDMODE,             /* KeyIdMode */
    TEST_KEYSOURCE,             /* KeySource */
    TEST_KEYINDEX               /* KeyIndex */
};

/** MLME-ASSOCIATE.response reference buffer */
uint8_t ref_mlme_associate_resp[] = {
    SPI_MLME_ASSOCIATE_RESPONSE, /* CmdId */
    22,                          /* Packet Length */
    TEST_DSTADDR,                /* DeviceAddress */
    0x01,
    0xCA,               /* AssocShortAddress */
    0x00,               /* Status */
    TEST_SECURITYLEVEL, /* SecurityLevel */
    TEST_KEYIDMODE,     /* KeyIdMode */
    TEST_KEYSOURCE,     /* KeySource */
    TEST_KEYINDEX       /* KeyIndex */
};

/** MLME-DISASSOCIATE.request reference buffer */
uint8_t ref_mlme_disassociate_req[] = {
    SPI_MLME_DISASSOCIATE_REQUEST, /* CmdId */
    24,                            /* Packet Length */
    MAC_MODE_LONG_ADDR,            /* DeviceAddrMode */
    TEST_PANID,                    /* DevicePANId */
    TEST_DSTADDR,                  /* DeviceAddress */
    DISASSOC_REASON_EVICT,         /* DisassociateReason */
    0,                             /* TxIndirect */
    TEST_SECURITYLEVEL,            /* SecurityLevel */
    TEST_KEYIDMODE,                /* KeyIdMode */
    TEST_KEYSOURCE,                /* KeySource */
    TEST_KEYINDEX                  /* KeyIndex */
};

/** MLME-GET.request reference buffer */
uint8_t ref_mlme_get_req[] = {
    SPI_MLME_GET_REQUEST, /* CmdId */
    2,                    /* Packet Length */
    TEST_PIBATTRIBUTE,    /* PIBAttribute */
    0x00                  /* PIBAttributeIndex */
};

/** MLME-GET.confirm reference buffer */
uint8_t ref_mlme_get_cnf[] = {SPI_MLME_GET_CONFIRM, /* CmdId */
                              5,                    /* Packet Length */
                              MAC_SUCCESS,          /* Status */
                              TEST_PIBATTRIBUTE,    /* PIBAttribute */
                              0x00,                 /* PIBAttributeIndex */
                              0x01,                 /* PIBAttributeLength */
                              TEST_PIBATTRIBUTEVALUE};

/** MLME-ORPHAN.response reference buffer */
uint8_t ref_mlme_orphan_resp[] = {
    SPI_MLME_ORPHAN_RESPONSE, /* CmdId */
    22,                       /* Packet Length */
    TEST_DSTADDR,             /* OrphanAddress */
    0x01,
    0xCA,               /* ShortAddress */
    0,                  /* AssociatedMember */
    TEST_SECURITYLEVEL, /* SecurityLevel */
    TEST_KEYIDMODE,     /* KeyIdMode */
    TEST_KEYSOURCE,     /* KeySource */
    TEST_KEYINDEX       /* KeyIndex */
};

/** MLME-RESET.request reference buffer */
uint8_t ref_mlme_reset_req[] = {
    SPI_MLME_RESET_REQUEST, /* CmdId */
    1,                      /* Packet Length */
    0                       /* SetDefaultPIB */
};

/** MLME-RESET.confirm reference buffer */
uint8_t ref_mlme_reset_cnf[] = {
    SPI_MLME_RESET_CONFIRM, /* CmdId */
    1,                      /* Packet Length */
    MAC_SUCCESS             /* Status */
};

/** MLME-RX-ENABLE.request reference buffer */
uint8_t ref_mlme_rx_enable_req[] = {
    SPI_MLME_RX_ENABLE_REQUEST, /* CmdId */
    9,                          /* Packet Length */
    0x00,                       /* DeferPermit */
    0x0A,
    0x00,
    0x00,
    0xA0, /* RxOnTime */
    0x0B,
    0x00,
    0x00,
    0xB0 /* RxOnDuration */
};

/** MLME-RX-ENABLE.confirm reference buffer */
uint8_t ref_mlme_rx_enable_cnf[] = {
    SPI_MLME_RX_ENABLE_CONFIRM, /* CmdId */
    1,                          /* Packet Length */
    MAC_SUCCESS                 /* Status */
};

/** MLME-SCAN.request reference buffer */
uint8_t ref_mlme_scan_req[] = {
    SPI_MLME_SCAN_REQUEST, /* CmdId */
    17,                    /* Packet Length */
    ACTIVE_SCAN,           /* ScanType */
    0x00,
    0xF8,
    0xFF,
    0x07,               /* ScanChannels */
    0x01,               /* ScanDuration */
    TEST_SECURITYLEVEL, /* SecurityLevel */
    TEST_KEYIDMODE,     /* KeyIdMode */
    TEST_KEYSOURCE,     /* KeySource */
    TEST_KEYINDEX       /* KeyIndex */
};

/** MLME-SET.request reference buffer */
uint8_t ref_mlme_set_req[] = {SPI_MLME_SET_REQUEST, /* CmdId */
                              4,                    /* Packet Length */
                              TEST_PIBATTRIBUTE,    /* PIBAttribute */
                              0x00,                 /* PIBAttributeIndex */
                              0x01,                 /* PIBAttributeLength */
                              TEST_PIBATTRIBUTEVALUE};

/** MLME-SET.confirm reference buffer */
uint8_t ref_mlme_set_cnf[] = {
    SPI_MLME_SET_CONFIRM, /* CmdId */
    3,                    /* Packet Length */
    MAC_SUCCESS,          /* Status */
    TEST_PIBATTRIBUTE,    /* PIBAttribute */
    0x00                  /* PIBAttributeIndex */
};

/** MLME-START.request reference buffer */
uint8_t ref_mlme_start_req[] = {
    SPI_MLME_START_REQUEST, /* CmdId */
    30,                     /* Packet Length */
    TEST_PANID,             /* PANId */
    TEST_CHANNEL,           /* LogicalChannel */
    0x0F,                   /* BeaconOrder */
    0x0F,                   /* SuperframeOrder */
    0,                      /* PANCoordinator */
    0,                      /* BatteryLifeExtension */
    0,                      /* CoordRealignment */
    TEST_SECURITYLEVEL,     /* CoordRealignSecurityLevel */
    TEST_KEYIDMODE,         /* CoordRealignKeyIdMode */
    TEST_KEYSOURCE,         /* CoordRealignKeySource */
    TEST_KEYINDEX,          /* CoordRealignKeyIndex */
    TEST_SECURITYLEVEL,     /* BeaconSecurityLevel */
    TEST_KEYIDMODE,         /* BeaconKeyIdMode */
    TEST_KEYSOURCE,         /* BeaconKeySource */
    TEST_KEYINDEX,          /* BeaconKeyIndex */
};

/** MLME-START.confirm reference buffer */
uint8_t ref_mlme_start_cnf[] = {
    SPI_MLME_START_CONFIRM, /* CmdId */
    1,                      /* Packet Length */
    MAC_SUCCESS             /* Status */
};

/** MLME-POLL.request reference buffer */
uint8_t ref_mlme_poll_req[] = {
    SPI_MLME_POLL_REQUEST, /* CmdId */
#if CASCODA_CA_VER == 8210
    24, /* Packet Length */
#else
    22, /* Packet Length */
#endif
    MAC_MODE_LONG_ADDR, /* CoordAddressMode */
    TEST_PANID,         /* CoordinatorPANId */
    TEST_DSTADDR,       /* CoordAddress */
#if CASCODA_CA_VER == 8210
    00,
    00, /*Interval*/
#endif
    TEST_SECURITYLEVEL, /* SecurityLevel */
    TEST_KEYIDMODE,     /* KeyIdMode */
    TEST_KEYSOURCE,     /* KeySource */
    TEST_KEYINDEX       /* KeyIndex */
};

/** MLME-POLL.confirm reference buffer */
uint8_t ref_mlme_poll_cnf[] = {
    SPI_MLME_POLL_CONFIRM, /* CmdId */
    1,                     /* Packet Length */
    MAC_SUCCESS            /* Status */
};

/** HWME-SET.request reference buffer */
uint8_t ref_hwme_set_req[] = {
    SPI_HWME_SET_REQUEST, /* CmdId */
    3,                    /* Packet Length */
    TEST_HWATTRIBUTE,     /* HWAttribute */
    1,                    /* HWAttributeLength */
    TEST_HWATTRIBUTEVALUE /* HWAttributeValue */
};

/** HWME-SET.confirm reference buffer */
uint8_t ref_hwme_set_cnf[] = {
    SPI_HWME_SET_CONFIRM, /* CmdId */
    2,                    /* Packet Length */
    MAC_SUCCESS,          /* Status */
    TEST_HWATTRIBUTE      /* HWAttribute */
};

/** HWME-GET.request reference buffer */
uint8_t ref_hwme_get_req[] = {
    SPI_HWME_GET_REQUEST, /* CmdId */
    1,                    /* Packet Length */
    TEST_HWATTRIBUTE      /* HWAttribute */
};

/** HWME-GET.confirm reference buffer */
uint8_t ref_hwme_get_cnf[] = {
    SPI_HWME_GET_CONFIRM, /* CmdId */
    4,                    /* Packet Length */
    MAC_SUCCESS,          /* Status */
    TEST_HWATTRIBUTE,     /* HWAttribute */
    1,                    /* HWAttributeLength */
    TEST_HWATTRIBUTEVALUE /* HWAttributeValue */
};

/** HWME-HAES.request reference buffer */
uint8_t ref_hwme_haes_req[] = {
    SPI_HWME_HAES_REQUEST, /* CmdId */
    17,                    /* Packet Length */
    TEST_HAESMODE,         /* HAESMode */
    TEST_HAESDATA          /* HAESData */
};

/** HWME-HAES.confirm reference buffer */
uint8_t ref_hwme_haes_cnf[] = {
    SPI_HWME_HAES_CONFIRM, /* CmdId */
    17,                    /* Packet Length */
    TEST_HAESMODE,         /* HAESMode */
    TEST_HAESDATA          /* HAESData (Encrypted/Descrypted) */
};

/** TDME-SETSFR.request reference buffer */
uint8_t ref_tdme_setsfr_req[] = {
    SPI_TDME_SETSFR_REQUEST, /* CmdId */
    3,                       /* Packet Length */
    TEST_SFRPAGE,            /* SFRPage */
    TEST_SFRADDRESS,         /* SFRAddress */
    TEST_SFRVALUE            /* SFRValue */
};

/** TDME-SETSFR.confirm reference buffer */
uint8_t ref_tdme_setsfr_cnf[] = {
    SPI_TDME_SETSFR_CONFIRM, /* CmdId */
    3,                       /* Packet Length */
    MAC_SUCCESS,             /* Status */
    TEST_SFRPAGE,            /* SFRPage */
    TEST_SFRADDRESS          /* SFRAddress */
};

/** TDME-GETSFR.request reference buffer */
uint8_t ref_tdme_getsfr_req[] = {
    SPI_TDME_GETSFR_REQUEST, /* CmdId */
    2,                       /* Packet Length */
    TEST_SFRPAGE,            /* SFRPage */
    TEST_SFRADDRESS          /* SFRAddress */
};

/** TDME-GETSFR.confirm reference buffer */
uint8_t ref_tdme_getsfr_cnf[] = {
    SPI_TDME_GETSFR_CONFIRM, /* CmdId */
    4,                       /* Packet Length */
    MAC_SUCCESS,             /* Status */
    TEST_SFRPAGE,            /* SFRPage */
    TEST_SFRADDRESS,         /* SFRAddress */
    TEST_SFRVALUE            /* SFRValue */
};

/** TDME-TESTMODE.request reference buffer */
uint8_t ref_tdme_testmode_req[] = {
    SPI_TDME_TESTMODE_REQUEST, /* CmdId */
    1,                         /* Packet Length */
    TEST_TESTMODE              /* TestMode */
};

/** TDME-TESTMODE.confirm reference buffer */
uint8_t ref_tdme_testmode_cnf[] = {
    SPI_TDME_TESTMODE_CONFIRM, /* CmdId */
    1,                         /* Packet Length */
    MAC_SUCCESS,               /* Status */
    TEST_TESTMODE              /* TestMode */
};

/** TDME-SET.request reference buffer */
uint8_t ref_tdme_set_req[] = {
    SPI_TDME_SET_REQUEST,   /* CmdId */
    3,                      /* Packet Length */
    TEST_TDMEATTRIBUTE,     /* TDMEAttribute */
    1,                      /* TDMEAttributeLength */
    TEST_TDMEATTRIBUTEVALUE /* TDMEAttributeValue */
};

/** TDME-SET.confirm reference buffer */
uint8_t ref_tdme_set_cnf[] = {
    SPI_TDME_SET_CONFIRM, /* CmdId */
    2,                    /* Packet Length */
    MAC_SUCCESS,          /* Status */
    TEST_TDMEATTRIBUTE    /* TDMEAttribute */
};

/** TDME-TXPKT.request reference buffer */
uint8_t ref_tdme_txpkt_req[] = {
    SPI_TDME_TXPKT_REQUEST, /* CmdId */
    3 + TEST_MSDULENGTH,    /* Packet Length */
    TDME_TXD_APPENDED,      /* TestPacketDataType */
    TEST_SEQUENCENUM,       /* TestPacketSequenceNumber */
    TEST_MSDULENGTH,        /* TestPacketLength */
    TEST_MSDU               /* TestPacketData */
};

/** TDME-TXPKT.confirm reference buffer */
uint8_t ref_tdme_txpkt_cnf[] = {
    SPI_TDME_TXPKT_CONFIRM, /* CmdId */
    3 + TEST_MSDULENGTH,    /* Packet Length */
    MAC_SUCCESS,            /* Status */
    TEST_SEQUENCENUM,       /* TestPacketSequenceNumber */
    TEST_MSDULENGTH,        /* TestPacketLength */
    TEST_MSDU               /* TestPacketData */
};

/** TDME-LOTLK.request reference buffer */
uint8_t ref_tdme_lotlk_req[] = {
    SPI_TDME_LOTLK_REQUEST, /* CmdId */
    2,                      /* Packet Length */
    TEST_CHANNEL,           /* TestChannel */
    0                       /* TestRxTxb */
};

/** TDME-LOTLK.confirm reference buffer */
uint8_t ref_tdme_lotlk_cnf[] = {
    SPI_TDME_LOTLK_CONFIRM, /* CmdId */
    6,                      /* Packet Length */
    MAC_SUCCESS,            /* Status */
    TEST_CHANNEL,           /* TestChannel */
    0,                      /* TestRxTxb */
    0,                      /* TestLOFDACValue */
    0,                      /* TestLOAMPValue */
    0                       /* TestLOTXCALValue */
};

/** Flags for ensuring correct dispatch has been called for each message */
struct dispatch_flags
{
	int data_ind : 1;          //!< MCPS_DATA_indication
	int data_cnf : 1;          //!< MCPS_DATA_confirm
	int assoc_ind : 1;         //!< MLME_ASSOCIATE_indication
	int assoc_cnf : 1;         //!< MLME_ASSOCIATE_confirm
	int disassoc_ind : 1;      //!< MLME_DISASSOCIATE_indication
	int disassoc_cnf : 1;      //!< MLME_DISASSOCIATE_confirm
	int beacon_notify_ind : 1; //!< MLME_BEACON_NOTIFY_indication
	int orphan_ind : 1;        //!< MLME_ORPHAN_indication
	int scan_cnf : 1;          //!< MLME_SCAN_confirm
	int comm_status_ind : 1;   //!< MLME_COMM_STATUS_indication
	int sync_loss_ind : 1;     //!< MLME_SYNC_LOSS_indication
	int wakeup_ind : 1;        //!< HWME_WAKEUP_indication
	int rxpkt_ind : 1;         //!< TDME_RXPKT_indication
	int eddet_ind : 1;         //!< TDME_EDDET_indication
	int error_ind : 1;         //!< TDME_ERROR_indication
	int generic : 1;           //!< generic dispatch
};

/** Private data for test application */
struct test_context
{
	struct dispatch_flags dflags; //!< Dispatch flags
};

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Fills a synchronous response buffer with the appropriate reference
 *******************************************************************************
 * When a synchronous command is tested, a valid response must be provided for
 * the function to operate correctly.
 *******************************************************************************
 * \param request_id - the command id of the request to respond to
 * \param response - Buffer to populate with synchronous response
 *******************************************************************************
 ******************************************************************************/
void populate_response(uint8_t request_id, uint8_t *response)
{
	uint8_t *reference_buffer = NULL;
	if (!response)
	{
		printf(ANSI_COLOR_RED "NULL response buffer passed to populate_response\n" ANSI_COLOR_RESET);
		return;
	}
	switch (request_id)
	{
	case SPI_MCPS_PURGE_REQUEST:
		reference_buffer = ref_mcps_purge_cnf;
		break;
	case SPI_MLME_GET_REQUEST:
		reference_buffer = ref_mlme_get_cnf;
		break;
	case SPI_MLME_RESET_REQUEST:
		reference_buffer = ref_mlme_reset_cnf;
		break;
	case SPI_MLME_RX_ENABLE_REQUEST:
		reference_buffer = ref_mlme_rx_enable_cnf;
		break;
	case SPI_MLME_SET_REQUEST:
		reference_buffer = ref_mlme_set_cnf;
		break;
	case SPI_MLME_START_REQUEST:
		reference_buffer = ref_mlme_start_cnf;
		break;
	case SPI_MLME_POLL_REQUEST:
		reference_buffer = ref_mlme_poll_cnf;
		break;
	case SPI_HWME_SET_REQUEST:
		reference_buffer = ref_hwme_set_cnf;
		break;
	case SPI_HWME_GET_REQUEST:
		reference_buffer = ref_hwme_get_cnf;
		break;
	case SPI_HWME_HAES_REQUEST:
		reference_buffer = ref_hwme_haes_cnf;
		break;
	case SPI_TDME_SETSFR_REQUEST:
		reference_buffer = ref_tdme_setsfr_cnf;
		break;
	case SPI_TDME_GETSFR_REQUEST:
		reference_buffer = ref_tdme_getsfr_cnf;
		break;
	case SPI_TDME_TESTMODE_REQUEST:
		reference_buffer = ref_tdme_testmode_cnf;
		break;
	case SPI_TDME_SET_REQUEST:
		reference_buffer = ref_tdme_set_cnf;
		break;
	case SPI_TDME_TXPKT_REQUEST:
		reference_buffer = ref_tdme_txpkt_cnf;
		break;
	case SPI_TDME_LOTLK_REQUEST:
		reference_buffer = ref_tdme_lotlk_cnf;
		break;
	default:
		printf("Request id 0x%02x does not have matching confirm buffer\n", request_id);
		break;
	}
	if (reference_buffer)
		memcpy(response, reference_buffer, reference_buffer[1] + 2);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Verifies the command frame produced by an API function
 *******************************************************************************
 * This function is called by the API functions with their constructed frame.
 * This frame is then compared to an expected reference.
 *******************************************************************************
 * \param buf - Constructed command frame
 * \param len - Length of frame
 * \param response - Buffer to populate with synchronous response
 * \param pDeviceRef - Device reference
 *******************************************************************************
 ******************************************************************************/
int verify_command(const uint8_t *buf, size_t len, uint8_t *response, struct ca821x_dev *pDeviceRef)
{
	uint8_t *reference_buffer = NULL;
	int      reference_len    = 0;
	printf("CmdId 0x%02x ", buf[0]);
	/* Set reference buffer */
	switch (buf[0])
	{
	case SPI_MCPS_DATA_REQUEST:
		reference_buffer = ref_mcps_data_req;
		break;
	case SPI_MCPS_PURGE_REQUEST:
		reference_buffer = ref_mcps_purge_req;
		break;
	case SPI_MLME_ASSOCIATE_REQUEST:
		reference_buffer = ref_mlme_associate_req;
		break;
	case SPI_MLME_ASSOCIATE_RESPONSE:
		reference_buffer = ref_mlme_associate_resp;
		break;
	case SPI_MLME_DISASSOCIATE_REQUEST:
		reference_buffer = ref_mlme_disassociate_req;
		break;
	case SPI_MLME_GET_REQUEST:
		reference_buffer = ref_mlme_get_req;
		break;
	case SPI_MLME_ORPHAN_RESPONSE:
		reference_buffer = ref_mlme_orphan_resp;
		break;
	case SPI_MLME_RESET_REQUEST:
		reference_buffer = ref_mlme_reset_req;
		break;
	case SPI_MLME_RX_ENABLE_REQUEST:
		reference_buffer = ref_mlme_rx_enable_req;
		break;
	case SPI_MLME_SCAN_REQUEST:
		reference_buffer = ref_mlme_scan_req;
		break;
	case SPI_MLME_SET_REQUEST:
		reference_buffer = ref_mlme_set_req;
		break;
	case SPI_MLME_START_REQUEST:
		reference_buffer = ref_mlme_start_req;
		break;
	case SPI_MLME_POLL_REQUEST:
		reference_buffer = ref_mlme_poll_req;
		break;
	case SPI_HWME_SET_REQUEST:
		reference_buffer = ref_hwme_set_req;
		break;
	case SPI_HWME_GET_REQUEST:
		reference_buffer = ref_hwme_get_req;
		break;
	case SPI_HWME_HAES_REQUEST:
		reference_buffer = ref_hwme_haes_req;
		break;
	case SPI_TDME_SETSFR_REQUEST:
		reference_buffer = ref_tdme_setsfr_req;
		break;
	case SPI_TDME_GETSFR_REQUEST:
		reference_buffer = ref_tdme_getsfr_req;
		break;
	case SPI_TDME_TESTMODE_REQUEST:
		reference_buffer = ref_tdme_testmode_req;
		break;
	case SPI_TDME_SET_REQUEST:
		reference_buffer = ref_tdme_set_req;
		break;
	case SPI_TDME_TXPKT_REQUEST:
		reference_buffer = ref_tdme_txpkt_req;
		break;
	case SPI_TDME_LOTLK_REQUEST:
		reference_buffer = ref_tdme_lotlk_req;
		break;
	default:
		printf(ANSI_COLOR_RED "Invalid downstream CmdId: 0x%02x" ANSI_COLOR_RESET ", ", buf[0]);
		return -1;
	}
	/* Get buffer length from data length +cmdid +len bytes */
	if (reference_buffer)
		reference_len = reference_buffer[1] + 2;
	if (len != reference_len)
	{
		printf(ANSI_COLOR_RED "Command length does not match reference" ANSI_COLOR_RESET "\n");
		printf("result  reference\n");
		printf("%2u      %2d\n", len, reference_len);
		return -1;
	}
	else
	{
		/* Length matches */
		if (memcmp(buf, reference_buffer, reference_len))
		{
			/* buffers differ */
			printf(ANSI_COLOR_RED "Output failed" ANSI_COLOR_RESET "\n");
			sReturnValue = -1;
			printf("result  reference\n");
			for (int i = 0; i < reference_len; i++)
			{
				if (buf[i] != reference_buffer[i])
				{
					printf(ANSI_COLOR_RED);
				}
				printf("%02x      %02x\n" ANSI_COLOR_RESET, buf[i], reference_buffer[i]);
			}
			return -1;
		}
		else
		{
			/* buffers match */
			printf(ANSI_COLOR_GREEN "Output verified" ANSI_COLOR_RESET ", ");
		}
	}
	if (response)
	{
		/* populate response buffer */
		populate_response(buf[0], response);
	}
	return 0;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Prints the result of an API function call
 *******************************************************************************
 * \param result - The function's return code
 *******************************************************************************
 ******************************************************************************/
void print_result(uint8_t result)
{
	printf("result: ");
	if (result)
	{
		printf(ANSI_COLOR_RED);
	}
	else
	{
		printf(ANSI_COLOR_GREEN);
	}
	printf("0x%02x\n" ANSI_COLOR_RESET, result);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief API functions test
 *******************************************************************************
 * Tests the cascoda_api command functions to ensure that they produce valid
 * command frames for the transceiver.
 *******************************************************************************
 * \return TODO
 *******************************************************************************
 ******************************************************************************/
int api_functions_test(void)
{
	uint8_t           ret, msdu_buffer[TEST_MSDULENGTH];
	uint8_t           msduhandle         = TEST_MSDUHANDLE;
	uint8_t           pibattributelength = 1, pibattributevalue = TEST_CHANNEL;
	uint8_t           hwattributelength = 1, hwattributevalue = TEST_HWATTRIBUTEVALUE;
	uint8_t           haesdata[16];
	uint8_t           sfrvalue, tdmeattributevalue = TEST_TDMEATTRIBUTEVALUE;
	uint8_t           sequencenum = TEST_SEQUENCENUM, txpktlength = TEST_MSDULENGTH;
	uint8_t           testchannel = TEST_CHANNEL, rxtxb = 0, lo_vals[3];
	uint8_t           interval[2] = {0, 0};
	struct FullAddr   full_address;
	struct ca821x_dev test_dev;
	struct SecSpec    test_secspec = {.SecurityLevel = TEST_SECURITYLEVEL,
                                   .KeyIdMode     = TEST_KEYIDMODE,
                                   .KeySource     = {TEST_KEYSOURCE},
                                   .KeyIndex      = TEST_KEYINDEX};
	printf(ANSI_COLOR_CYAN "Testing API functions...\n" ANSI_COLOR_RESET);
	/* TODO: check return */
	ca821x_api_init(&test_dev);
	test_dev.ca821x_api_downstream = verify_command;
	/* Call each API downstream, check constructed packet */
	full_address.AddressMode = MAC_MODE_LONG_ADDR;
	full_address.PANId[0]    = 0x5C;
	full_address.PANId[1]    = 0xCA;
	memcpy(full_address.PANId, (uint8_t[]){TEST_PANID}, sizeof(full_address.PANId));
	memcpy(full_address.Address, (uint8_t[]){TEST_DSTADDR}, sizeof(full_address.Address));
	memcpy(msdu_buffer, (uint8_t[]){TEST_MSDU}, TEST_MSDULENGTH);
	memcpy(haesdata, (uint8_t[]){TEST_HAESDATA}, sizeof(haesdata));
	printf("%-35s", "MCPS_DATA_request()... ");
	ret = MCPS_DATA_request(MAC_MODE_SHORT_ADDR, full_address, TEST_MSDULENGTH, msdu_buffer, TEST_MSDUHANDLE, 0x00,
	                        &test_secspec, &test_dev);
	print_result(ret);
	printf("%-35s", "MCPS_PURGE_request_sync()... ");
	ret = MCPS_PURGE_request_sync(&msduhandle, &test_dev);
	print_result(ret);
	printf("%-35s", "MLME_ASSOCIATE_request()... ");
	ret = MLME_ASSOCIATE_request(TEST_CHANNEL, full_address, 0, &test_secspec, &test_dev);
	print_result(ret);
	printf("%-35s", "MLME_ASSOCIATE_response()... ");
	ret = MLME_ASSOCIATE_response(full_address.Address, 0xCA01, 0, &test_secspec, &test_dev);
	print_result(ret);
	printf("%-35s", "MLME_DISASSOCIATE_request()... ");
	ret = MLME_DISASSOCIATE_request(full_address, DISASSOC_REASON_EVICT, 0, &test_secspec, &test_dev);
	print_result(ret);
	printf("%-35s", "MLME_SET_request_sync()... ");
	ret = MLME_SET_request_sync(TEST_PIBATTRIBUTE, 0, pibattributelength, &pibattributevalue, &test_dev);
	print_result(ret);
	printf("%-35s", "MLME_GET_request_sync()... ");
	ret = MLME_GET_request_sync(TEST_PIBATTRIBUTE, 0, &pibattributelength, &pibattributevalue, &test_dev);
	print_result(ret);
	printf("%-35s", "MLME_ORPHAN_response()... ");
	ret = MLME_ORPHAN_response(full_address.Address, 0xCA01, 0, &test_secspec, &test_dev);
	print_result(ret);
	printf("%-35s", "MLME_RESET_request_sync()... ");
	ret = MLME_RESET_request_sync(0, &test_dev);
	print_result(ret);
	printf("%-35s", "MLME_RX_ENABLE_request_sync()... ");
	ret = MLME_RX_ENABLE_request_sync(0, 0xA000000A, 0xB000000B, &test_dev);
	print_result(ret);
	printf("%-35s", "MLME_SCAN_request()... ");
	ret = MLME_SCAN_request(ACTIVE_SCAN, 0x07FFF800, 0x01, &test_secspec, &test_dev);
	print_result(ret);
	printf("%-35s", "MLME_START_request_sync()... ");
	ret = MLME_START_request_sync(GETLE16(full_address.PANId), TEST_CHANNEL, 0x0F, 0x0F, 0, 0, 0, &test_secspec,
	                              &test_secspec, &test_dev);
	print_result(ret);
	printf("%-35s", "MLME_POLL_request_sync()... ");
	ret = MLME_POLL_request_sync(full_address,
#if CASCODA_CA_VER == 8210
	                             interval,
#endif
	                             &test_secspec, &test_dev);
	print_result(ret);
	printf("%-35s", "HWME_SET_request_sync()... ");
	ret = HWME_SET_request_sync(TEST_HWATTRIBUTE, 1, &hwattributevalue, &test_dev);
	print_result(ret);
	printf("%-35s", "HWME_GET_request_sync()... ");
	ret = HWME_GET_request_sync(TEST_HWATTRIBUTE, &hwattributelength, &hwattributevalue, &test_dev);
	print_result(ret);
	printf("%-35s", "HWME_HAES_request_sync()... ");
	ret = HWME_HAES_request_sync(TEST_HAESMODE, haesdata, &test_dev);
	print_result(ret);
	printf("%-35s", "TDME_SETSFR_request_sync()... ");
	ret = TDME_SETSFR_request_sync(TEST_SFRPAGE, TEST_SFRADDRESS, TEST_SFRVALUE, &test_dev);
	print_result(ret);
	printf("%-35s", "TDME_GETSFR_request_sync()... ");
	ret = TDME_GETSFR_request_sync(TEST_SFRPAGE, TEST_SFRADDRESS, &sfrvalue, &test_dev);
	print_result(ret);
	printf("%-35s", "TDME_TESTMODE_request_sync()... ");
	ret = TDME_TESTMODE_request_sync(TEST_TESTMODE, &test_dev);
	print_result(ret);
	printf("%-35s", "TDME_SET_request_sync()... ");
	ret = TDME_SET_request_sync(TEST_TDMEATTRIBUTE, 1, &tdmeattributevalue, &test_dev);
	print_result(ret);
	printf("%-35s", "TDME_TXPKT_request_sync()... ");
	ret = TDME_TXPKT_request_sync(TDME_TXD_APPENDED, &sequencenum, &txpktlength, msdu_buffer, &test_dev);
	print_result(ret);
	printf("%-35s", "TDME_LOTLK_request_sync()... ");
	ret = TDME_LOTLK_request_sync(&testchannel, &rxtxb, lo_vals, lo_vals + 1, lo_vals + 2, &test_dev);
	print_result(ret);
	printf("Function test complete\n\n");
	return 0;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief MCPS-DATA.indication callback function
 *******************************************************************************
 * \param params - Primitive parameters
 * \param pDeviceRef - Device reference
 *******************************************************************************
 * \return 1 if this callback was expected, 0 otherwise
 *******************************************************************************
 ******************************************************************************/
int test_MCPS_DATA_indication(struct MCPS_DATA_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	struct test_context *tcontext = pDeviceRef->context;
	if (tcontext->dflags.data_ind)
	{
		tcontext->dflags.data_ind = 0;
		return 1;
	}
	return 0;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief MCPS-DATA.confirm callback function
 *******************************************************************************
 * \param params - Primitive parameters
 * \param pDeviceRef - Device reference
 *******************************************************************************
 * \return 1 if this callback was expected, 0 otherwise
 *******************************************************************************
 ******************************************************************************/
int test_MCPS_DATA_confirm(struct MCPS_DATA_confirm_pset *params, struct ca821x_dev *pDeviceRef)
{
	struct test_context *tcontext = pDeviceRef->context;
	if (tcontext->dflags.data_cnf)
	{
		tcontext->dflags.data_cnf = 0;
		return 1;
	}
	return 0;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief MLME-ASSOCIATE.indication callback function
 *******************************************************************************
 * \param params - Primitive parameters
 * \param pDeviceRef - Device reference
 *******************************************************************************
 * \return 1 if this callback was expected, 0 otherwise
 *******************************************************************************
 ******************************************************************************/
int test_MLME_ASSOCIATE_indication(struct MLME_ASSOCIATE_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	struct test_context *tcontext = pDeviceRef->context;
	if (tcontext->dflags.assoc_ind)
	{
		tcontext->dflags.assoc_ind = 0;
		return 1;
	}
	return 0;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief MLME-ASSOCIATE.confirm callback function
 *******************************************************************************
 * \param params - Primitive parameters
 * \param pDeviceRef - Device reference
 *******************************************************************************
 * \return 1 if this callback was expected, 0 otherwise
 *******************************************************************************
 ******************************************************************************/
int test_MLME_ASSOCIATE_confirm(struct MLME_ASSOCIATE_confirm_pset *params, struct ca821x_dev *pDeviceRef)
{
	struct test_context *tcontext = pDeviceRef->context;
	if (tcontext->dflags.assoc_cnf)
	{
		tcontext->dflags.assoc_cnf = 0;
		return 1;
	}
	return 0;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief MLME-DISASSOCIATE.indication callback function
 *******************************************************************************
 * \param params - Primitive parameters
 * \param pDeviceRef - Device reference
 *******************************************************************************
 * \return 1 if this callback was expected, 0 otherwise
 *******************************************************************************
 ******************************************************************************/
int test_MLME_DISASSOCIATE_indication(struct MLME_DISASSOCIATE_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	struct test_context *tcontext = pDeviceRef->context;
	if (tcontext->dflags.disassoc_ind)
	{
		tcontext->dflags.disassoc_ind = 0;
		return 1;
	}
	return 0;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief MLME-DISASSOCIATE.confirm callback function
 *******************************************************************************
 * \param params - Primitive parameters
 * \param pDeviceRef - Device reference
 *******************************************************************************
 * \return 1 if this callback was expected, 0 otherwise
 *******************************************************************************
 ******************************************************************************/
int test_MLME_DISASSOCIATE_confirm(struct MLME_DISASSOCIATE_confirm_pset *params, struct ca821x_dev *pDeviceRef)
{
	struct test_context *tcontext = pDeviceRef->context;
	if (tcontext->dflags.disassoc_cnf)
	{
		tcontext->dflags.disassoc_cnf = 0;
		return 1;
	}
	return 0;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief MLME-BEACON-NOTIFY.indication callback function
 *******************************************************************************
 * \param params - Primitive parameters
 * \param pDeviceRef - Device reference
 *******************************************************************************
 * \return 1 if this callback was expected, 0 otherwise
 *******************************************************************************
 ******************************************************************************/
int test_MLME_BEACON_NOTIFY_indication(struct MLME_BEACON_NOTIFY_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	struct test_context *tcontext = pDeviceRef->context;
	if (tcontext->dflags.beacon_notify_ind)
	{
		tcontext->dflags.beacon_notify_ind = 0;
		return 1;
	}
	return 0;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief MLME-ORPHAN.indication callback function
 *******************************************************************************
 * \param params - Primitive parameters
 * \param pDeviceRef - Device reference
 *******************************************************************************
 * \return 1 if this callback was expected, 0 otherwise
 *******************************************************************************
 ******************************************************************************/
int test_MLME_ORPHAN_indication(struct MLME_ORPHAN_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	struct test_context *tcontext = pDeviceRef->context;
	if (tcontext->dflags.orphan_ind)
	{
		tcontext->dflags.orphan_ind = 0;
		return 1;
	}
	return 0;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief MLME-SCAN.confirm callback function
 *******************************************************************************
 * \param params - Primitive parameters
 * \param pDeviceRef - Device reference
 *******************************************************************************
 * \return 1 if this callback was expected, 0 otherwise
 *******************************************************************************
 ******************************************************************************/
int test_MLME_SCAN_confirm(struct MLME_SCAN_confirm_pset *params, struct ca821x_dev *pDeviceRef)
{
	struct test_context *tcontext = pDeviceRef->context;
	if (tcontext->dflags.scan_cnf)
	{
		tcontext->dflags.scan_cnf = 0;
		return 1;
	}
	return 0;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief MLME-COMM-STATUS.indication callback function
 *******************************************************************************
 * \param params - Primitive parameters
 * \param pDeviceRef - Device reference
 *******************************************************************************
 * \return 1 if this callback was expected, 0 otherwise
 *******************************************************************************
 ******************************************************************************/
int test_MLME_COMM_STATUS_indication(struct MLME_COMM_STATUS_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	struct test_context *tcontext = pDeviceRef->context;
	if (tcontext->dflags.comm_status_ind)
	{
		tcontext->dflags.comm_status_ind = 0;
		return 1;
	}
	return 0;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief MLME-SYNC-LOSS.indication callback function
 *******************************************************************************
 * \param params - Primitive parameters
 * \param pDeviceRef - Device reference
 *******************************************************************************
 * \return 1 if this callback was expected, 0 otherwise
 *******************************************************************************
 ******************************************************************************/
int test_MLME_SYNC_LOSS_indication(struct MLME_SYNC_LOSS_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	struct test_context *tcontext = pDeviceRef->context;
	if (tcontext->dflags.sync_loss_ind)
	{
		tcontext->dflags.sync_loss_ind = 0;
		return 1;
	}
	return 0;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief HWME-WAKEUP.indication callback function
 *******************************************************************************
 * \param params - Primitive parameters
 * \param pDeviceRef - Device reference
 *******************************************************************************
 * \return 1 if this callback was expected, 0 otherwise
 *******************************************************************************
 ******************************************************************************/
int test_HWME_WAKEUP_indication(struct HWME_WAKEUP_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	struct test_context *tcontext = pDeviceRef->context;
	if (tcontext->dflags.wakeup_ind)
	{
		tcontext->dflags.wakeup_ind = 0;
		return 1;
	}
	return 0;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TDME-RXPKT.indication callback function
 *******************************************************************************
 * \param params - Primitive parameters
 * \param pDeviceRef - Device reference
 *******************************************************************************
 * \return 1 if this callback was expected, 0 otherwise
 *******************************************************************************
 ******************************************************************************/
int test_TDME_RXPKT_indication(struct TDME_RXPKT_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	struct test_context *tcontext = pDeviceRef->context;
	if (tcontext->dflags.rxpkt_ind)
	{
		tcontext->dflags.rxpkt_ind = 0;
		return 1;
	}
	return 0;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TDME-EDDET.indication callback function
 *******************************************************************************
 * \param params - Primitive parameters
 * \param pDeviceRef - Device reference
 *******************************************************************************
 * \return 1 if this callback was expected, 0 otherwise
 *******************************************************************************
 ******************************************************************************/
int test_TDME_EDDET_indication(struct TDME_EDDET_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	struct test_context *tcontext = pDeviceRef->context;
	if (tcontext->dflags.eddet_ind)
	{
		tcontext->dflags.eddet_ind = 0;
		return 1;
	}
	return 0;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TDME-ERROR.indication callback function
 *******************************************************************************
 * \param params - Primitive parameters
 * \param pDeviceRef - Device reference
 *******************************************************************************
 * \return 1 if this callback was expected, 0 otherwise
 *******************************************************************************
 ******************************************************************************/
int test_TDME_ERROR_indication(struct TDME_ERROR_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	struct test_context *tcontext = pDeviceRef->context;
	if (tcontext->dflags.error_ind)
	{
		tcontext->dflags.error_ind = 0;
		return 1;
	}
	return 0;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Generic callback function
 *******************************************************************************
 * \param buf - Message buffer
 * \param len - Length of message
 * \param pDeviceRef - Device reference
 *******************************************************************************
 * \return 1 if this callback was expected, 0 otherwise
 *******************************************************************************
 ******************************************************************************/
int test_generic_dispatch(const uint8_t *buf, size_t len, struct ca821x_dev *pDeviceRef)
{
	struct test_context *tcontext = pDeviceRef->context;
	if (tcontext->dflags.generic)
	{
		tcontext->dflags.generic = 0;
		return 1;
	}
	return 0;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Wrapper for ca821x_downstream_dispatch
 *******************************************************************************
 * Calls ca821x_downstream_dispatch and prints the result
 *******************************************************************************
 * \param buf - Message buffer
 * \param len - Length of message
 * \param pDeviceRef - Device reference
 *******************************************************************************
 ******************************************************************************/
void call_dispatch(uint8_t *buf, size_t len, struct ca821x_dev *pDeviceRef)
{
	if (ca821x_downstream_dispatch(buf, len, pDeviceRef))
	{
		printf(ANSI_COLOR_GREEN "Success\n" ANSI_COLOR_RESET);
	}
	else
	{
		printf(ANSI_COLOR_RED "Fail\n" ANSI_COLOR_RESET);
		sReturnValue = -1;
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Callbacks test
 *******************************************************************************
 * \return TODO
 *******************************************************************************
 ******************************************************************************/
int api_callbacks_test(void)
{
	struct ca821x_dev   test_dev;
	struct MAC_Message  msgbuf;
	struct test_context tcontext;
	printf(ANSI_COLOR_CYAN "Testing callbacks...\n" ANSI_COLOR_RESET);
	memset(&tcontext, 0, sizeof(tcontext));
	ca821x_api_init(&test_dev);
	test_dev.context                                 = &tcontext;
	test_dev.callbacks.MCPS_DATA_indication          = test_MCPS_DATA_indication;
	test_dev.callbacks.MCPS_DATA_confirm             = test_MCPS_DATA_confirm;
	test_dev.callbacks.MLME_ASSOCIATE_indication     = test_MLME_ASSOCIATE_indication;
	test_dev.callbacks.MLME_ASSOCIATE_confirm        = test_MLME_ASSOCIATE_confirm;
	test_dev.callbacks.MLME_DISASSOCIATE_indication  = test_MLME_DISASSOCIATE_indication;
	test_dev.callbacks.MLME_DISASSOCIATE_confirm     = test_MLME_DISASSOCIATE_confirm;
	test_dev.callbacks.MLME_BEACON_NOTIFY_indication = test_MLME_BEACON_NOTIFY_indication;
	test_dev.callbacks.MLME_ORPHAN_indication        = test_MLME_ORPHAN_indication;
	test_dev.callbacks.MLME_SCAN_confirm             = test_MLME_SCAN_confirm;
	test_dev.callbacks.MLME_COMM_STATUS_indication   = test_MLME_COMM_STATUS_indication;
	test_dev.callbacks.MLME_SYNC_LOSS_indication     = test_MLME_SYNC_LOSS_indication;
	test_dev.callbacks.HWME_WAKEUP_indication        = test_HWME_WAKEUP_indication;
	test_dev.callbacks.TDME_RXPKT_indication         = test_TDME_RXPKT_indication;
	test_dev.callbacks.TDME_EDDET_indication         = test_TDME_EDDET_indication;
	test_dev.callbacks.TDME_ERROR_indication         = test_TDME_ERROR_indication;
	test_dev.callbacks.generic_dispatch              = test_generic_dispatch;
	printf("%-35s", "MCPS_DATA_indication... ");
	msgbuf.CommandId         = SPI_MCPS_DATA_INDICATION;
	tcontext.dflags.data_ind = 1;
	call_dispatch(&msgbuf.CommandId, 0, &test_dev);
	printf("%-35s", "MCPS_DATA_confirm... ");
	msgbuf.CommandId         = SPI_MCPS_DATA_CONFIRM;
	tcontext.dflags.data_cnf = 1;
	call_dispatch(&msgbuf.CommandId, 0, &test_dev);
	printf("%-35s", "MLME_ASSOCIATE_indication... ");
	msgbuf.CommandId          = SPI_MLME_ASSOCIATE_INDICATION;
	tcontext.dflags.assoc_ind = 1;
	call_dispatch(&msgbuf.CommandId, 0, &test_dev);
	printf("%-35s", "MLME_ASSOCIATE_confirm... ");
	msgbuf.CommandId          = SPI_MLME_ASSOCIATE_CONFIRM;
	tcontext.dflags.assoc_cnf = 1;
	call_dispatch(&msgbuf.CommandId, 0, &test_dev);
	printf("%-35s", "MLME_DISASSOCIATE_indication... ");
	msgbuf.CommandId             = SPI_MLME_DISASSOCIATE_INDICATION;
	tcontext.dflags.disassoc_ind = 1;
	call_dispatch(&msgbuf.CommandId, 0, &test_dev);
	printf("%-35s", "MLME_DISASSOCIATE_confirm... ");
	msgbuf.CommandId             = SPI_MLME_DISASSOCIATE_CONFIRM;
	tcontext.dflags.disassoc_cnf = 1;
	call_dispatch(&msgbuf.CommandId, 0, &test_dev);
	printf("%-35s", "MLME_BEACON_NOTIFY_indication... ");
	msgbuf.CommandId                  = SPI_MLME_BEACON_NOTIFY_INDICATION;
	tcontext.dflags.beacon_notify_ind = 1;
	call_dispatch(&msgbuf.CommandId, 0, &test_dev);
	printf("%-35s", "MLME_ORPHAN_indication... ");
	msgbuf.CommandId           = SPI_MLME_ORPHAN_INDICATION;
	tcontext.dflags.orphan_ind = 1;
	call_dispatch(&msgbuf.CommandId, 0, &test_dev);
	printf("%-35s", "MLME_SCAN_confirm... ");
	msgbuf.CommandId         = SPI_MLME_SCAN_CONFIRM;
	tcontext.dflags.scan_cnf = 1;
	call_dispatch(&msgbuf.CommandId, 0, &test_dev);
	printf("%-35s", "MLME_COMM_STATUS_indication... ");
	msgbuf.CommandId                = SPI_MLME_COMM_STATUS_INDICATION;
	tcontext.dflags.comm_status_ind = 1;
	call_dispatch(&msgbuf.CommandId, 0, &test_dev);
	printf("%-35s", "MLME_SYNC_LOSS_indication... ");
	msgbuf.CommandId              = SPI_MLME_SYNC_LOSS_INDICATION;
	tcontext.dflags.sync_loss_ind = 1;
	call_dispatch(&msgbuf.CommandId, 0, &test_dev);
	printf("%-35s", "HWME_WAKEUP_indication... ");
	msgbuf.CommandId           = SPI_HWME_WAKEUP_INDICATION;
	tcontext.dflags.wakeup_ind = 1;
	call_dispatch(&msgbuf.CommandId, 0, &test_dev);
	printf("%-35s", "TDME_RXPKT_indication... ");
	msgbuf.CommandId          = SPI_TDME_RXPKT_INDICATION;
	tcontext.dflags.rxpkt_ind = 1;
	call_dispatch(&msgbuf.CommandId, 0, &test_dev);
	printf("%-35s", "TDME_EDDET_indication... ");
	msgbuf.CommandId          = SPI_TDME_EDDET_INDICATION;
	tcontext.dflags.eddet_ind = 1;
	call_dispatch(&msgbuf.CommandId, 0, &test_dev);
	printf("%-35s", "TDME_ERROR_indication... ");
	msgbuf.CommandId          = SPI_TDME_ERROR_INDICATION;
	tcontext.dflags.error_ind = 1;
	call_dispatch(&msgbuf.CommandId, 0, &test_dev);
	printf("%-35s", "generic_dispatch... ");
	test_dev.callbacks.TDME_ERROR_indication = NULL;
	tcontext.dflags.generic                  = 1;
	call_dispatch(&msgbuf.CommandId, 0, &test_dev);
	printf("Callbacks test complete\n\n");
	return 0;
}

int main(void)
{
	sReturnValue = 0;
	api_functions_test();
	api_callbacks_test();
	return sReturnValue;
}
