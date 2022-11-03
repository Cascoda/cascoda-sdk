#include <stdio.h>

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-bm/cascoda_wait.h"

#include "cascoda_devboard_btn.h"

#include "ca821x_api.h"

void short_press_cb()
{
	printf("Button short pressed!\n");
}

//void long_press_cb()
//{
//	printf("Button long pressed!\n");
//}

void hold_cb()
{
	printf("Button held!\n");
}

int main(void)
{
	struct ca821x_dev dev;
	ca821x_api_init(&dev);

	EVBMEInitialise(CA_TARGET_NAME, &dev);

	// Register the first 3 LEDS as output
	DVBD_RegisterLEDOutput(LED_BTN_0, JUMPER_POS_2);
	DVBD_RegisterLEDOutput(LED_BTN_1, JUMPER_POS_2);
	DVBD_RegisterLEDOutput(LED_BTN_2, JUMPER_POS_2);

	// Register the last button as input
	DVBD_RegisterButtonInput(LED_BTN_3, JUMPER_POS_2);
	DVBD_SetButtonShortPressCallback(LED_BTN_3, &short_press_cb);
	//DVBD_SetButtonLongPressCallback(LED_BTN_3, &long_press_cb, 5);
	DVBD_SetButtonHoldCallback(LED_BTN_3, &hold_cb);

	// Toggles for the power to each LED
	u8_t power0 = LED_OFF;
	u8_t power1 = LED_OFF;
	u8_t power2 = LED_OFF;

	u16_t tick = 0;
	while (1)
	{
		cascoda_io_handler(&dev);

		// Every 4th cycle, the power to each LED will be toggled
		// Each LEDs toggle time is shifted by 1 cycle to produce a "wave" effect
		u8_t remainder = tick % 5;
		if (remainder == 0)
		{
			power0 = !power0;
		}
		else if (remainder == 1)
		{
			power1 = !power1;
		}
		else if (remainder == 2)
		{
			power2 = !power2;
		}

		// Apply the new LED power states
		DVBD_SetLED(LED_BTN_0, power0);
		DVBD_SetLED(LED_BTN_1, power1);
		DVBD_SetLED(LED_BTN_2, power2);

		DVBD_PollButtons();

		WAIT_ms(50);

		tick++;
	}

	return 0;
}