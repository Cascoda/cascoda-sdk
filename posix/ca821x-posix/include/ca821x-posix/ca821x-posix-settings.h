#ifndef CA821X_POSIX_SETTINGS_H
#define CA821X_POSIX_SETTINGS_H

#include <stdint.h>

/**
 * @brief Get the directory where of the persistent storage file
 * for a particular node
 * 
 * @param[in] aNodeId The ID of the node to get the directory for
 * 
 */
char *posixGetDataDir(uint32_t aNodeId);

#endif