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
 * Sensor interface for Diodes/Pericom PI4IOE5V6408 8-bit gpio expander
*/

#ifndef SIF_PI4IOE5V6408_H
#define SIF_PI4IOE5V6408_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* i2c address */
#define SIF_PI4IOE5V6408_I2C_ADDR 0x43

/* interrupt pin */
#define SIF_PI4IOE5V6408_INT_PIN 31

/* data length, fixed to 1 */
#define SIF_PI4IOE5V6408_DATALEN 1
/* I2C register address length */
#define SIF_PI4IOE5V6408_ADDLEN 1

/* register addresses */
#define SIF_PI4IOE5V6408_REG_ID_CTRL 0x01     /* device id and control */
#define SIF_PI4IOE5V6408_REG_IO_DIR 0x03      /* i/o direction */
#define SIF_PI4IOE5V6408_REG_OUT_STATE 0x05   /* output state */
#define SIF_PI4IOE5V6408_REG_OUT_HIGHZ 0x07   /* output high impedance */
#define SIF_PI4IOE5V6408_REG_INP_DEFAULT 0x09 /* input default state */
#define SIF_PI4IOE5V6408_REG_PUP_ENABLE 0x0B  /* pullup / pulldown enable */
#define SIF_PI4IOE5V6408_REG_PUP_DOWNB 0x0D   /* pullup / pulldown select */
#define SIF_PI4IOE5V6408_REG_INP_STATUS 0x0F  /* input status */
#define SIF_PI4IOE5V6408_REG_IRQ_MASK 0x11    /* interrupt mask */
#define SIF_PI4IOE5V6408_REG_IRQ_STATUS 0x13  /* interrupt status */

/* timing parameters [ms] */
#define SIF_PI4IOE5V6408_T_RESET 1 /* startup time after power-up */

/* i/o values / states */
#define SIF_PI4IOE5V6408_STATE_LO 0 /* low */
#define SIF_PI4IOE5V6408_STATE_HI 1 /* high */

/* pull-up / pull-down */
#define SIF_PI4IOE5V6408_PUP_OFF 0 /* no pull-up */
#define SIF_PI4IOE5V6408_PUP_EN 1  /* pull-up enabled */

/* i/o interrupt on or off (masked) */
#define SIF_PI4IOE5V6408_IRQ_OFF 0 /* irq off (masked) for specific i/o */
#define SIF_PI4IOE5V6408_IRQ_ON 1  /* irq on for specific i/o */

/* control / status register bit masks */
#define SIF_PI4IOE5V6408_CTRL_RESET 0x01  /* software reset */
#define SIF_PI4IOE5V6408_CTRL_IRQRST 0x02 /* reset interrupt */
#define SIF_PI4IOE5V6408_ID_FW_REV 0x1C   /* firmware revision */
#define SIF_PI4IOE5V6408_ID_DEV_ID 0xE0   /* device id */
#define SIF_PI4IOE5V6408_ID_FW_REV_SHR 2  /* shift for firmware revision */
#define SIF_PI4IOE5V6408_ID_DEV_ID_SHR 5  /* shift for devie id */

/* data acquisition status */
enum pi4ioe5v6408_status
{
	SIF_PI4IOE5V6408_ST_OK          = 0, /* read values ok */
	SIF_PI4IOE5V6408_ST_FAIL        = 1, /* acquisition failed */
	SIF_PI4IOE5V6408_ST_UNAVAILABLE = 2  /* unavailable / not connected */
};

/* Functions */

/**
 * \brief Device initialisation
 * \return true if active, false if not
 *
 */
bool SIF_PI4IOE5V6408_IsInterruptActive(void);

/**
 * \brief Device initialisation
 * \return pi4ioe5v6408_status
 *
 */
uint8_t SIF_PI4IOE5V6408_Initialise(void);

/**
 * \brief Read chip id and revision id
 * \param chip_id - device id code
 * \param rev_id - firmware revision
 * \return pi4ioe5v6408_status
 *
 */
uint8_t SIF_PI4IOE5V6408_ReadDeviceId(uint8_t *chip_id, uint8_t *rev_id);

/**
 * \brief sense specific input
 * \param io - port bit number (0-7)
 * \param val - returned i/o state
 * \return pi4ioe5v6408_status
 *
 */
uint8_t SIF_PI4IOE5V6408_SenseInput(uint8_t io, uint8_t *val);

/**
 * \brief sense specific output
 * \param io - port bit number (0-7)
 * \param val - returned i/o state
 * \return pi4ioe5v6408_status
 *
 */
uint8_t SIF_PI4IOE5V6408_SenseOutput(uint8_t io, uint8_t *val);

/**
 * \brief sense all inputs (0-7) for port
 * \param port - returned i/o states
 * \return pi4ioe5v6408_status
 *
 */
uint8_t SIF_PI4IOE5V6408_SenseAllInputs(uint8_t *port);

/**
 * \brief set specific output
 * \param io - port bit number (0-7)
 * \param val - i/o state
 * \return pi4ioe5v6408_status
 *
 */
uint8_t SIF_PI4IOE5V6408_SetOutput(uint8_t io, uint8_t val);

/**
 * \brief configure input
 * \param io - port bit number (0-7)
 * \param irq_on - irq enabled when (1)
 * \param pullup_on - pull-up enabled when (1)
 * \return pi4ioe5v6408_status
 *
 */
uint8_t SIF_PI4IOE5V6408_ConfigureInput(uint8_t io, uint8_t irq_on, uint8_t pullup_on);

/**
 * \brief configure output
 * \param io - port bit number (0-7)
 * \return pi4ioe5v6408_status
 *
 */
uint8_t SIF_PI4IOE5V6408_ConfigureOutput(uint8_t io);

#endif // SIF_PI4IOE5V6408_H
