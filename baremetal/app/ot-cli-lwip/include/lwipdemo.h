#ifndef LWIPDEMO_H
#define LWIPDEMO_H

#include "ca821x_api.h"

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Process a CLI command to change the state of the lwip demo.
 * Should be linked to the openthread CLI using otCliSetUserCommands.
 *******************************************************************************
 ******************************************************************************/
void handle_cli_lwipdemo(int argc, char *argv[]);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Initialise the lwip demo.
 * Should be called once at program startup.
 *******************************************************************************
 * \param aInstance - Pointer to an OpenThread instance.
 * \param pDeviceRef - Pointer to initialised ca821x_device_ref struct.
 *******************************************************************************
 * \return CA_ERROR_SUCCESS for success.
 *******************************************************************************
 ******************************************************************************/
ca_error init_lwipdemo(otInstance *aInstance, struct ca821x_dev *pDeviceRef);

#endif //LWIPDEMO_H
