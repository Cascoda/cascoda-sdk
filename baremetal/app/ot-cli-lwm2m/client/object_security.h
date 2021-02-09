/*******************************************************************************
 *
 * Copyright (c) 2021 Cascoda Ltd.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Distribution License
 * v1.0 which accompanies this distribution.
 *
 * The Eclipse Distribution License is available at
 *    http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ciaran Woodward - Cascoda Ltd.
 *
 *******************************************************************************/
/*
 * object_security.h
 *
 *  Security object abstraction.
 *
 *  Created on: 21.01.2021
 *  Author: Ciaran Woodward
 *  Copyright (c) Cascoda Ltd. All rights reserved.
 */

#ifndef OBJECT_SECURITY_H_
#define OBJECT_SECURITY_H_

#include "liblwm2m.h"

/**
 * Create the security object and return a pointer to it
 * @param serverId   Unique server ID
 * @param serverUri  URI of the server
 * @param bsPskId    ID of the psk
 * @param psk        Pre shared Key
 * @param pskLen     Length of preshared key
 * @param isBootstrap  true if bootstrapping
 * @return Pointer to created security object, or NULL if error
 */
lwm2m_object_t *get_security_object(int         serverId,
                                    const char *serverUri,
                                    char *      bsPskId,
                                    char *      psk,
                                    uint16_t    pskLen,
                                    bool        isBootstrap);

/**
 * Free and clean up the security object
 * @param objectP Pointer to security object
 */
void clean_security_object(lwm2m_object_t *objectP);

/**
 * Get the server URI from security object
 * @param objectP  Pointer to security object
 * @param secObjInstID  Security object ID
 * @return pointer reference to server URI which must not be modified
 */
const char *security_get_server_uri(lwm2m_object_t *objectP, uint16_t secObjInstID);

/**
 * Get the security mode from security object
 * @param objectP  Pointer to security object
 * @param secObjInstID  Security object ID
 * @return Security mode identifier, or -1 on failure (See LWM2M_SECURITY_MODE_* definitions)
 */
int32_t security_get_mode(lwm2m_object_t *objectP, uint16_t secObjInstID);

/**
 * Get the public id from security object
 * @param objectP  Pointer to security object
 * @param secObjInstID  Security object ID
 * @param[out] idLen Pointer to output the length of the id to.
 * @return const secret key pointer, with length stored in *keyLen
 */
const char *security_get_public_id(lwm2m_object_t *objectP, uint16_t secObjInstID, size_t *idLen);

/**
 * Get the secret key from security object
 * @param objectP  Pointer to security object
 * @param secObjInstID  Security object ID
 * @param[out] keyLen Pointer to output the length of the key to.
 * @return const secret key pointer, with length stored in *keyLen
 */
const char *security_get_secret_key(lwm2m_object_t *objectP, uint16_t secObjInstID, size_t *keyLen);

/**
 * Print the security object to output, for debugging
 * @param objectP Pointer to security object
 */
void security_display_object(lwm2m_object_t *objectP);

/**
 * Create a copy of the security object source into the destination, with freshly allocated internal data.
 *
 * @param objectDest Pointer to the object to write to.
 * @param objectSrc  Pointer to the object to copy from.
 */
void security_copy_object(lwm2m_object_t *objectDest, lwm2m_object_t *objectSrc);

#endif /* OBJECT_SECURITY_H_ */
