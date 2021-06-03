#ifdef NDEBUG
#undef NDEBUG
#endif

#include <unistd.h>

#include <openthread/thread.h>
#include <platform.h>

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "ca821x_api.h"

#include "oc_api.h"

otInstance *OT_INSTANCE;

int main(void)
{
	int               init;
	struct ca821x_dev dev;

	ca821x_api_init(&dev);

	// Initialisation of Chip and EVBME
	EVBMEInitialise(CA_TARGET_NAME, &dev);
	PlatformRadioInitWithDev(&dev);

	// OpenThread Configuration
	OT_INSTANCE = otInstanceInitSingle();

	const size_t  ITEM_SIZE  = 1400;
	const int32_t ITERATIONS = 20;
	char          changing_data[ITEM_SIZE];
	char          CHANGING_KEY[] = "changing";
	char          static_data[ITEM_SIZE];
	char          STATIC_KEY[] = "static";
	char          read_data[ITEM_SIZE];
	char          letter_to_write = 'A'; // starts at 0x41, goes up one by one

	memset(changing_data, letter_to_write, ITEM_SIZE);
	memset(static_data, '9', ITEM_SIZE); //0x39

	// make sure the flash is empty
	otInstanceErasePersistentInfo(OT_INSTANCE);

	oc_storage_config("./storage_test");
	// write unchanging data
	oc_storage_write(STATIC_KEY, static_data, ITEM_SIZE);
	oc_storage_read(STATIC_KEY, read_data, ITEM_SIZE);
	assert(memcmp(static_data, read_data, ITEM_SIZE) == 0);

	for (int i = 0; i < ITERATIONS; ++i)
	{
		// Rewrite something over and over
		oc_storage_write(CHANGING_KEY, changing_data, ITEM_SIZE);
		oc_storage_read(CHANGING_KEY, read_data, ITEM_SIZE);
		assert(memcmp(changing_data, read_data, ITEM_SIZE) == 0);
		// Make sure you can still read the static data
		oc_storage_read(STATIC_KEY, read_data, ITEM_SIZE);
		assert(memcmp(static_data, read_data, ITEM_SIZE) == 0);

		// Change the next thing to write to flash
		letter_to_write++;
		for (int j = 0; j < ITEM_SIZE; ++j) changing_data[j] = ++letter_to_write;

		memset(read_data, 0, ITEM_SIZE);
	}

	while (1)
	{
	}

	/* shut down the stack */
	return 0;
}
