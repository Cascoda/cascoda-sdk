#ifndef ACTUATORDEMO_H
#define ACTUATORDEMO_H

#include "ca821x_api.h"

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Process a CLI command to change the state of the actuator demo.
 * Should be linked to the openthread CLI using otCliSetUserCommands.
 *******************************************************************************
 ******************************************************************************/
void handle_cli_actuatordemo(void *aContext, uint8_t aArgsLength, char *aArgs[]);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Initialise the actuator demo.
 * Should be called once at program startup.
 *******************************************************************************
 * \param aInstance - Pointer to an OpenThread instance.
 * \param pDeviceRef - Pointer to initialised ca821x_device_ref struct.
 *******************************************************************************
 * \return CA_ERROR_SUCCESS for success.
 *******************************************************************************
 ******************************************************************************/
ca_error init_actuatordemo(otInstance *aInstance, struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Handle all actuatordemo functionality.
 * Should be called regularly by the program main loop.
 *******************************************************************************
 * \param pDeviceRef - Pointer to initialised ca821x_device_ref struct.
 *******************************************************************************
 * \return CA_ERROR_SUCCESS for success.
 *******************************************************************************
 ******************************************************************************/
ca_error handle_actuatordemo(struct ca821x_dev *pDeviceRef);

#endif //ACTUATORDEMO_H
