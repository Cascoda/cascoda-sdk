/*
 * Copyright (C) 2019  Cascoda, Ltd.
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
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
//cmocka must be after system
#include <cmocka.h>

#include "ca821x_api_helper.h"

/** Test that the security spec can be retrieved from the end of an MCPS data
 *  request successfully.
 */
static void msdu_secspec_test(void **state)
{
	struct MCPS_DATA_indication_pset dataInd     = {0};
	struct SecSpec *                 trueSecSpec = NULL;
	(void)state;

	dataInd.MsduLength = 1;
	trueSecSpec        = (struct SecSpec *)(dataInd.Msdu + 1);
	assert_ptr_equal(MCPS_DATA_indication_get_secspec(&dataInd), trueSecSpec);

	dataInd.MsduLength = MAX_DATA_SIZE;
	trueSecSpec        = (struct SecSpec *)(dataInd.Msdu + MAX_DATA_SIZE);
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

int main(void)
{
	const struct CMUnitTest tests[] = {
	    cmocka_unit_test(&msdu_secspec_test),
	    cmocka_unit_test(&beacon_notify_test),
	    cmocka_unit_test(&beacon_notify_zerotest),
	    cmocka_unit_test(&scan_confirm_test),
	    cmocka_unit_test(&scan_confirm_null_test),
	    cmocka_unit_test(&key_table_entry_test),
	    cmocka_unit_test(&key_table_entry_zerotest),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
