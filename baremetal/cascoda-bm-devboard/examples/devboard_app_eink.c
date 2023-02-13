/**
 * @file
 *//*
 *  Copyright (c) 2023, Cascoda Ltd.
 *  All rights reserved.
 *
 */
/*
 * Application for E-ink display of images.
*/
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_sensorif.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-bm/cascoda_wait.h"
#include "cascoda-util/cascoda_time.h"
#include "ca821x_api.h"

/* Insert Application-Specific Includes here */
#include "cascoda-bm/test15_4_evbme.h"
#include "devboard_btn.h"
#include "sif_il3820.h"
#include "sif_il3820_image.h"

#define SPI_NUM 1             // SPI interface used for display
#define NUM_IMAGES 4          // 4 images can be loaded
#define SLEEP_LONG_TIME 60000 // sleep time without button interrupt

static uint8_t *image_pointers[] = {cascoda_img_2in9, lanpo_a1_img_2in9, lanpo_a2_img_2in9, lanpo_a4_img_2in9};
static u8_t     image_nr         = 0;

// load image
static void load_image(u8_t nr)
{
	uint8_t *image_pointer = image_pointers[nr];
	printf("Loading image %u ..\n", (nr + 1));
	if (nr == 0)
		SIF_IL3820_overlay_qr_code("https://www.cascoda.com", image_pointer, 90, 20);
	SIF_IL3820_Initialise(&lut_full_update);
	SIF_IL3820_ClearAndDisplayImage(image_pointer);
	SIF_IL3820_Deinitialise();
	printf("Loading complete.\n");
}

// button callback
static void button_pressed(void *context)
{
	(void)context;
	if (image_nr < (NUM_IMAGES - 1))
		++image_nr;
	else
		image_nr = 0;
	load_image(image_nr);
}

// application initialisation
void hardware_init(void)
{
	/* SW3 is LED output; static for power-on */
	DVBD_RegisterLEDOutput(LED_BTN_2, JUMPER_POS_1);
	DVBD_SetLED(LED_BTN_2, LED_ON);
	/* SW4 is IRQ button to test button interrupts */
	DVBD_RegisterButtonIRQInput(LED_BTN_3, JUMPER_POS_1);
	DVBD_SetButtonShortPressCallback(LED_BTN_3, &button_pressed, NULL, BTN_SHORTPRESS_PRESSED);

	/* Eink Initialisation */
	SENSORIF_SPI_Config(SPI_NUM);
	load_image(image_nr);
}

// application polling function
void hardware_poll(void)
{
	/* check buttons */
	DVBD_PollButtons();
}

// application level check if device can go to sleep
bool hardware_can_sleep(void)
{
	if (!DVBD_CanSleep())
		return false;

	return true;
}

// application sleep (power-down) function
void hardware_sleep(struct ca821x_dev *pDeviceRef)
{
	/* no scheduled tasks */

	/* and sleep */
	DVBD_DevboardSleep(SLEEP_LONG_TIME, pDeviceRef);
}

// application reinitialise after wakeup
void hardware_reinitialise(void)
{
}

// re-initialise after wakeup from sleep
static int reinitialise_after_wakeup(struct ca821x_dev *pDeviceRef)
{
	(void)pDeviceRef;

	/* for OpenThread: */
	//otLinkSyncExternalMac(OT_INSTANCE);

	/* reinitialise hardware (application-specific) */
	hardware_reinitialise();

	return 0;
}

// sleep if possible
static void sleep_if_possible(struct ca821x_dev *pDeviceRef)
{
	/* for OpenThread: */
	//if (!PlatformCanSleep(OT_INSTANCE))
	//    return;

	/* check for hardware (application-specific) */
	if (!hardware_can_sleep())
		return;

	hardware_sleep(pDeviceRef);
}

// main loop
int main(void)
{
	struct ca821x_dev dev;

	ca821x_api_init(&dev);
	EVBMEInitialise(CA_TARGET_NAME, &dev);

	/* Application-Specific Initialisation Routines */
	hardware_init();

	/* Endless Polling Loop */
	while (1)
	{
		cascoda_io_handler(&dev);
		hardware_poll();
		sleep_if_possible(&dev);

	} /* while(1) */
}
