#ifndef SENSORDEMO_H
#define SENSORDEMO_H

#include "ca821x_api.h"

/**
 * Process a CLI command to change the state of the sensor demo.
 *
 * Should be linked to the openthread CLI using otCliSetUserCommands
 */
void handle_cli_sensordemo(int argc, char *argv[]);

/**
 * Initialise the sensor demo.
 *
 * Should be called once at program startup
 */
ca_error init_sensordemo(otInstance *aInstance, struct ca821x_dev *pDeviceRef);

/**
 * Handle all sensordemo functionality.
 *
 * Should be called regularly by the program main loop.
 */
ca_error handle_sensordemo(struct ca821x_dev *pDeviceRef);

#endif //SENSORDEMO_H
