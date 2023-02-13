/* define delay functions used in mikrosdk click app code */

#include "cascoda-bm/cascoda_wait.h"

void Delay_ms(uint32_t delay)
{
	WAIT_ms(delay);
}

void Delay_10ms(void)
{
	WAIT_ms(10);
}

void Delay_100ms(void)
{
	WAIT_ms(100);
}

void Delay_1sec(void)
{
	WAIT_ms(1000);
}
