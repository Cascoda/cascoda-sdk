/*
 *  Copyright (c) 2019, Cascoda Ltd.
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
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
//cmocka must be after system
#include <cmocka.h>

#include "ca821x_api_helper.h"

//Stub as these tests are build without platform layer
void ca_log(ca_loglevel loglevel, const char *format, va_list argp)
{
	(void)loglevel;
	(void)format;
	(void)argp;
}

/** Test that the security spec can be retrieved from the end of an MCPS data
 *  request successfully.
 */
static void msdu_secspec_test(void **state)
{
	struct MCPS_DATA_indication_pset dataInd     = {0};
	struct SecSpec                  *trueSecSpec = NULL;
	(void)state;

	dataInd.MsduLength = 1;
#if CASCODA_CA_VER >= 8212
	uint8_t msdu_shift = dataInd.HeaderIELength + dataInd.PayloadIELength;
	trueSecSpec        = (struct SecSpec *)(dataInd.Data[msdu_shift] + 1);
#else
	trueSecSpec = (struct SecSpec *)(dataInd.Msdu + 1);
#endif
	assert_ptr_equal(MCPS_DATA_indication_get_secspec(&dataInd), trueSecSpec);

	dataInd.MsduLength = MAX_DATA_SIZE;
#if CASCODA_CA_VER >= 8212
	trueSecSpec = (struct SecSpec *)(dataInd.Data[msdu_shift] + MAX_DATA_SIZE);
#else
	trueSecSpec = (struct SecSpec *)(dataInd.Msdu + MAX_DATA_SIZE);
#endif
	assert_ptr_equal(MCPS_DATA_indication_get_secspec(&dataInd), trueSecSpec);
}

/** Test that the address lists and SDU can be successfully pulled from a beacon
 *  notify indication.
 */
static void beacon_notify_test(void **state)
{
	struct testBeaconNotify
	{
		struct MLME_BEACON_NOTIFY_indication_pset stat;
		uint8_t                                   pendAddrSpec;
		struct ShortAddr                          shAddrs[3];
		struct ExtAddr                            exAddrs[2];
		uint8_t                                   sduLen;
		uint8_t                                   sdu[10];

	} testBeaconNotify = {};

	testBeaconNotify.sduLen                                    = 10;
	testBeaconNotify.stat.PanDescriptor.Security.SecurityLevel = 1;
	testBeaconNotify.pendAddrSpec                              = 0x23;

	struct MLME_BEACON_NOTIFY_indication_pset *testPset =
	    (struct MLME_BEACON_NOTIFY_indication_pset *)&testBeaconNotify;
	uint8_t len = 0;

	assert_ptr_equal(MLME_BEACON_NOTIFY_indication_get_shortaddrs(&len, testPset), testBeaconNotify.shAddrs);
	assert_int_equal(len, 3);

	assert_ptr_equal(MLME_BEACON_NOTIFY_indication_get_extaddrs(&len, testPset), testBeaconNotify.exAddrs);
	assert_int_equal(len, 2);

	assert_ptr_equal(MLME_BEACON_NOTIFY_indication_get_sdu(&len, testPset), testBeaconNotify.sdu);
	assert_int_equal(len, 10);
}

/** Test that a beacon notify indication with no addresses can still be successfully
 *  parsed.
 */
static void beacon_notify_zerotest(void **state)
{
	//Has to be manually built in order to have securitylevel 0
	struct testBeaconNotify
	{
		uint8_t         BSN;
		struct FullAddr Coord;
		uint8_t         LogicalChannel;
		uint8_t         SuperframeSpec[2];
		uint8_t         GTSPermit;
		uint8_t         LinkQuality;
		uint8_t         TimeStamp[4];
		uint8_t         SecurityFailure;
		uint8_t         SecurityLevel;
		uint8_t         pendAddrSpec;
		uint8_t         sduLen;
		uint8_t         sdu[10];
	} testBeaconNotify = {0};

	testBeaconNotify.sduLen        = 1;
	testBeaconNotify.SecurityLevel = 0;
	testBeaconNotify.pendAddrSpec  = 0;

	struct MLME_BEACON_NOTIFY_indication_pset *testPset =
	    (struct MLME_BEACON_NOTIFY_indication_pset *)&testBeaconNotify;
	uint8_t len = 0;

	assert_ptr_equal(MLME_BEACON_NOTIFY_indication_get_shortaddrs(&len, testPset), &testBeaconNotify.sduLen);
	assert_int_equal(len, 0);

	assert_ptr_equal(MLME_BEACON_NOTIFY_indication_get_extaddrs(&len, testPset), &testBeaconNotify.sduLen);
	assert_int_equal(len, 0);

	assert_ptr_equal(MLME_BEACON_NOTIFY_indication_get_sdu(&len, testPset), testBeaconNotify.sdu);
	assert_int_equal(len, 1);
}

/** Test that a scan confirm can be successfully parsed when containing both secured
 *  and non secured beacons.
 */
static void scan_confirm_test(void **state)
{
	struct nsPanDescriptor
	{
		struct FullAddr Coord;
		uint8_t         LogicalChannel;
		uint8_t         SuperframeSpec[2];
		uint8_t         GTSPermit;
		uint8_t         LinkQuality;
		uint8_t         TimeStamp[4];
		uint8_t         SecurityFailure;
		uint8_t         SecurityLevel;
	};

	struct scanConfirmTest
	{
		uint8_t                Status;
		uint8_t                ScanType;
		uint8_t                UnscannedChannels[4];
		uint8_t                ResultListSize;
		struct nsPanDescriptor pd0;
		struct PanDescriptor   pd1;
		struct nsPanDescriptor pd2;
		struct PanDescriptor   pd3;
	} scanConfirmTest                  = {0};
	struct MLME_SCAN_confirm_pset *sct = (struct MLME_SCAN_confirm_pset *)&scanConfirmTest;

	scanConfirmTest.ScanType                   = ACTIVE_SCAN;
	scanConfirmTest.ResultListSize             = 4;
	scanConfirmTest.pd1.Security.SecurityLevel = 1;
	scanConfirmTest.pd3.Security.SecurityLevel = 1;

	assert_ptr_equal(MLME_SCAN_confirm_get_pandescriptor(0, sct), &(scanConfirmTest.pd0));
	assert_ptr_equal(MLME_SCAN_confirm_get_pandescriptor(1, sct), &(scanConfirmTest.pd1));
	assert_ptr_equal(MLME_SCAN_confirm_get_pandescriptor(2, sct), &(scanConfirmTest.pd2));
	assert_ptr_equal(MLME_SCAN_confirm_get_pandescriptor(3, sct), &(scanConfirmTest.pd3));
	assert_ptr_equal(MLME_SCAN_confirm_get_pandescriptor(4, sct), NULL);

	//Also passive
	scanConfirmTest.ScanType = PASSIVE_SCAN;

	assert_ptr_equal(MLME_SCAN_confirm_get_pandescriptor(0, sct), &(scanConfirmTest.pd0));
	assert_ptr_equal(MLME_SCAN_confirm_get_pandescriptor(1, sct), &(scanConfirmTest.pd1));
	assert_ptr_equal(MLME_SCAN_confirm_get_pandescriptor(2, sct), &(scanConfirmTest.pd2));
	assert_ptr_equal(MLME_SCAN_confirm_get_pandescriptor(3, sct), &(scanConfirmTest.pd3));
	assert_ptr_equal(MLME_SCAN_confirm_get_pandescriptor(4, sct), NULL);
}

/** Test that a scan confirm helper function doesn't try and analyse energy and orphan scans
 */
static void scan_confirm_null_test(void **state)
{
	struct MLME_SCAN_confirm_pset scanTest = {0};

	scanTest.ScanType       = ENERGY_DETECT;
	scanTest.ResultListSize = 5;

	assert_null(MLME_SCAN_confirm_get_pandescriptor(0, &scanTest));
	scanTest.ScanType = ORPHAN_SCAN;
	assert_null(MLME_SCAN_confirm_get_pandescriptor(0, &scanTest));
}

#if CASCODA_CA_VER <= 8211
/** Test that a key table entry can be correctly parsed
 */
static void key_table_entry_test(void **state)
{
	struct testKte
	{
		struct M_KeyTableEntryFixed fixed;
		struct M_KeyIdLookupDesc    KeyIdLookupList[5];
		struct M_KeyDeviceDesc      KeyDeviceList[2];
		struct M_KeyUsageDesc       KeyUsageList[6];
	} testKte                        = {0};
	struct M_KeyTableEntryFixed *kte = (struct M_KeyTableEntryFixed *)&testKte;

	testKte.fixed.KeyIdLookupListEntries = 5;
	testKte.fixed.KeyDeviceListEntries   = 2;
	testKte.fixed.KeyUsageListEntries    = 6;

	assert_ptr_equal(KeyTableEntry_get_keyidlookupdescs(kte), testKte.KeyIdLookupList);
	assert_ptr_equal(KeyTableEntry_get_keydevicedescs(kte), testKte.KeyDeviceList);
	assert_ptr_equal(KeyTableEntry_get_keyusagedescs(kte), testKte.KeyUsageList);
}

/** Test that a key table entry with zero-length lists can be correctly parsed
 */
static void key_table_entry_zerotest(void **state)
{
	struct testKte
	{
		struct M_KeyTableEntryFixed fixed;
		struct M_KeyIdLookupDesc    KeyIdLookupList[3];
		struct M_KeyUsageDesc       KeyUsageList[2];
	} testKte                        = {0};
	struct M_KeyTableEntryFixed *kte = (struct M_KeyTableEntryFixed *)&testKte;

	testKte.fixed.KeyIdLookupListEntries = 3;
	testKte.fixed.KeyDeviceListEntries   = 0;
	testKte.fixed.KeyUsageListEntries    = 2;

	assert_ptr_equal(KeyTableEntry_get_keyidlookupdescs(kte), testKte.KeyIdLookupList);
	assert_non_null(KeyTableEntry_get_keydevicedescs(kte));
	assert_ptr_equal(KeyTableEntry_get_keyusagedescs(kte), testKte.KeyUsageList);
}
#endif // CASCODA_CA_VER <= 8211

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(&msdu_secspec_test),
		cmocka_unit_test(&beacon_notify_test),
		cmocka_unit_test(&beacon_notify_zerotest),
		cmocka_unit_test(&scan_confirm_test),
		cmocka_unit_test(&scan_confirm_null_test),
#if CASCODA_CA_VER <= 8211
		cmocka_unit_test(&key_table_entry_test),
		cmocka_unit_test(&key_table_entry_zerotest),
#endif // CASCODA_CA_VER <= 8211
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
