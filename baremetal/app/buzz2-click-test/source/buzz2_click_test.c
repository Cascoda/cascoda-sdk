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
/**
 * @file
 *
 * @brief Test for buzz2 click
 */

#include "buzz2_click_test.h"
#include "buzz2_click.h"

#include "cascoda-bm/cascoda_sensorif.h"
#include "cascoda-util/cascoda_tasklet.h"

static bool g_play_one_note_done   = false;
static bool g_ascending_scale_done = false;

ca_error buzz2_play_one_note(void)
{
	if (!g_play_one_note_done)
	{
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_E5, 1, 250, NON_BLOCKING);
		g_play_one_note_done = true;
	}
}

ca_error buzz2_ascending_scale(void)
{
	if (!g_ascending_scale_done)
	{
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_C2, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_Db2, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_D2, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_Eb2, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_E2, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_F2, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_Gb2, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_G2, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_Ab2, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_A2, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_Bb2, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_B2, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_C3, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_Db3, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_D3, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_Eb3, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_E3, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_F3, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_Gb3, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_G3, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_Ab3, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_A3, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_Bb3, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_B3, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_C4, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_Db4, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_D4, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_Eb4, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_E4, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_F4, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_Gb4, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_G4, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_Ab4, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_A4, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_Bb4, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_B4, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_C5, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_Db5, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_D5, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_Eb5, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_E5, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_F5, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_Gb5, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_Gb5, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_G5, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_Ab5, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_A5, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_Bb5, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_B5, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_C6, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_Db6, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_D6, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_Eb6, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_E6, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_F6, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_Gb6, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_G6, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_Ab6, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_A6, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_Bb6, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_B6, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_C7, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_Db7, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_D7, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_Eb7, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_E7, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_F7, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_Gb7, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_G7, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_Ab7, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_A7, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_Bb7, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_B7, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_C8, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_Db8, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_Eb8, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_E8, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_Gb8, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_Ab8, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_A8, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_Bb8, 1, 50, BLOCKING);
		MIKROSDK_BUZZ2_play_note(BUZZ2_NOTE_B8, 1, 50, BLOCKING);
		g_ascending_scale_done = true;
	}
}