/**
 * @file
 * @brief mikrosdk interface
 */
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
/*
 * Example click interface driver
*/

#ifndef BUZZ2_CLICK_H
#define BUZZ2_CLICK_H

#include <stdint.h>

/**
 * @brief Defines for music note frequencies, from Octave 2 to Octave 8
 * 
 */
#define BUZZ2_NOTE_C2 65
#define BUZZ2_NOTE_Db2 69
#define BUZZ2_NOTE_D2 73
#define BUZZ2_NOTE_Eb2 78
#define BUZZ2_NOTE_E2 82
#define BUZZ2_NOTE_F2 87
#define BUZZ2_NOTE_Gb2 93
#define BUZZ2_NOTE_G2 98
#define BUZZ2_NOTE_Ab2 104
#define BUZZ2_NOTE_A2 110
#define BUZZ2_NOTE_Bb2 117
#define BUZZ2_NOTE_B2 123
#define BUZZ2_NOTE_C3 131
#define BUZZ2_NOTE_Db3 139
#define BUZZ2_NOTE_D3 147
#define BUZZ2_NOTE_Eb3 156
#define BUZZ2_NOTE_E3 165
#define BUZZ2_NOTE_F3 175
#define BUZZ2_NOTE_Gb3 185
#define BUZZ2_NOTE_G3 196
#define BUZZ2_NOTE_Ab3 208
#define BUZZ2_NOTE_A3 220
#define BUZZ2_NOTE_Bb3 233
#define BUZZ2_NOTE_B3 247
#define BUZZ2_NOTE_C4 262
#define BUZZ2_NOTE_Db4 277
#define BUZZ2_NOTE_D4 294
#define BUZZ2_NOTE_Eb4 311
#define BUZZ2_NOTE_E4 330
#define BUZZ2_NOTE_F4 349
#define BUZZ2_NOTE_Gb4 370
#define BUZZ2_NOTE_G4 392
#define BUZZ2_NOTE_Ab4 415
#define BUZZ2_NOTE_A4 440
#define BUZZ2_NOTE_Bb4 466
#define BUZZ2_NOTE_B4 494
#define BUZZ2_NOTE_C5 523
#define BUZZ2_NOTE_Db5 554
#define BUZZ2_NOTE_D5 587
#define BUZZ2_NOTE_Eb5 622
#define BUZZ2_NOTE_E5 659
#define BUZZ2_NOTE_F5 698
#define BUZZ2_NOTE_Gb5 740
#define BUZZ2_NOTE_G5 784
#define BUZZ2_NOTE_Ab5 831
#define BUZZ2_NOTE_A5 880
#define BUZZ2_NOTE_Bb5 932
#define BUZZ2_NOTE_B5 988
#define BUZZ2_NOTE_C6 1047
#define BUZZ2_NOTE_Db6 1109
#define BUZZ2_NOTE_D6 1175
#define BUZZ2_NOTE_Eb6 1245
#define BUZZ2_NOTE_E6 1319
#define BUZZ2_NOTE_F6 1397
#define BUZZ2_NOTE_Gb6 1480
#define BUZZ2_NOTE_G6 1568
#define BUZZ2_NOTE_Ab6 1661
#define BUZZ2_NOTE_A6 1760
#define BUZZ2_NOTE_Bb6 1865
#define BUZZ2_NOTE_B6 1976
#define BUZZ2_NOTE_C7 2093
#define BUZZ2_NOTE_Db7 2217
#define BUZZ2_NOTE_D7 2349
#define BUZZ2_NOTE_Eb7 2489
#define BUZZ2_NOTE_E7 2637
#define BUZZ2_NOTE_F7 2794
#define BUZZ2_NOTE_Gb7 2960
#define BUZZ2_NOTE_G7 3136
#define BUZZ2_NOTE_Ab7 3322
#define BUZZ2_NOTE_A7 3520
#define BUZZ2_NOTE_Bb7 3729
#define BUZZ2_NOTE_B7 3951
#define BUZZ2_NOTE_C8 4186
#define BUZZ2_NOTE_Db8 4435
#define BUZZ2_NOTE_D8 4699
#define BUZZ2_NOTE_Eb8 4978
#define BUZZ2_NOTE_E8 5274
#define BUZZ2_NOTE_F8 5588
#define BUZZ2_NOTE_Gb8 5920
#define BUZZ2_NOTE_Ab8 6645
#define BUZZ2_NOTE_A8 7040
#define BUZZ2_NOTE_Bb8 7459
#define BUZZ2_NOTE_B8 7902

/**
 * @brief PWM pin define on the devboard
 * 
 */
#define BUZZ2_PWM_PIN 35

/**
 * @brief Enum (buzz2 status)
 * 
 */
enum buzz2_status
{
	BUZZ2_ST_OK   = 0,
	BUZZ2_ST_FAIL = 1,
};

/**
 * @brief Enum (mode for play note function)
 * 
 */
enum play_note_mode
{
	BLOCKING     = 0,
	NON_BLOCKING = 1,
};

/* new functions */
/**
 * @brief Function that plays a note
 * 
 * @param freq Note frequency
 * @param volume Volume[%] of buzz2 (0 = off, 100 = max)
 * @param duration Duration[ms] of the note to be played (min = 0, max = 65535)
 * @param mode Mode of the function (BLOCKING/NON-BLOCKING)
 * Blocking: Uses delay, can call this function successively, not recommended for KNX applications
 * Non-blocking: Schedules tasklet, can only call this function once within the specified duration
 * @return uint8_t (buzz2 status)
 */
uint8_t MIKROSDK_BUZZ2_play_note(uint32_t freq, uint32_t volume, uint16_t duration, uint8_t mode);

/**
 * @brief Buzz2 Initialisation function
 * 
 * @return uint8_t (buzz2 status)
 */
uint8_t MIKROSDK_BUZZ2_Initialise(void);

#endif // BUZZ2_CLICK_H