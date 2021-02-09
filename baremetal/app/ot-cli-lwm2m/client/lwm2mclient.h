/*******************************************************************************
 *
 * Copyright (c) 2014 Bosch Software Innovations GmbH, Germany and others
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v20.html
 * The Eclipse Distribution License is available at
 *    http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Bosch Software Innovations GmbH - Please refer to git log
 *    Ciaran Woodward, Cascoda Ltd.
 *
 *******************************************************************************/

#ifndef LWM2MCLIENT_H_
#define LWM2MCLIENT_H_

#include "openthread/instance.h"
#include "liblwm2m.h"

/**
 * Get the global ot instance.
 *
 * @warning The current implementation only supports a single OT instance to be used for lwm2m due architectural constraints
 * within wakaama making it impossible to extract the lwm2m context from object callbacks. (On baremetal we only support a single
 * openthread instance anyway)
 * @return The global ot instance.
 */
otInstance *get_ot_instance(void);

/*
 * object_device.c
 */
lwm2m_object_t *get_object_device(void);
void            free_object_device(lwm2m_object_t *objectP);
uint8_t         device_change(lwm2m_data_t *dataArray, lwm2m_object_t *objectP);
void            display_device_object(lwm2m_object_t *objectP);
/*
 * object_firmware.c
 */
lwm2m_object_t *get_object_firmware(void);
void            free_object_firmware(lwm2m_object_t *objectP);
void            display_firmware_object(lwm2m_object_t *objectP);
/*
 * object_location.c
 */
lwm2m_object_t *get_object_location(void);
void            free_object_location(lwm2m_object_t *object);
void            display_location_object(lwm2m_object_t *objectP);
/*
 * object_test.c
 */
#define TEST_OBJECT_ID 31024
lwm2m_object_t *get_test_object(void);
void            free_test_object(lwm2m_object_t *object);
void            display_test_object(lwm2m_object_t *objectP);
/*
 * object_server.c
 */
lwm2m_object_t *get_server_object(int serverId, const char *binding, int lifetime, bool storing);
void            clean_server_object(lwm2m_object_t *object);
void            display_server_object(lwm2m_object_t *objectP);
void            copy_server_object(lwm2m_object_t *objectDest, lwm2m_object_t *objectSrc);

/*
 * object_connectivity_moni.c
 */
lwm2m_object_t *get_object_conn_m(void);
void            free_object_conn_m(lwm2m_object_t *objectP);

/*
 * object_connectivity_stat.c
 */
extern lwm2m_object_t *get_object_conn_s(void);
void                   free_object_conn_s(lwm2m_object_t *objectP);
extern void            conn_s_updateTxStatistic(lwm2m_object_t *objectP, uint16_t txDataByte);
extern void            conn_s_updateRxStatistic(lwm2m_object_t *objectP, uint16_t rxDataByte);

/*
 * object_access_control.c
 */
lwm2m_object_t *acc_ctrl_create_object(void);
void            acl_ctrl_free_object(lwm2m_object_t *objectP);
bool            acc_ctrl_obj_add_inst(lwm2m_object_t *accCtrlObjP,
                                      uint16_t        instId,
                                      uint16_t        acObjectId,
                                      uint16_t        acObjInstId,
                                      uint16_t        acOwner);
bool acc_ctrl_oi_add_ac_val(lwm2m_object_t *accCtrlObjP, uint16_t instId, uint16_t aclResId, uint16_t acValue);
/*
 * lwm2mclient.c
 */
void handle_value_changed(lwm2m_context_t *lwm2mH, lwm2m_uri_t *uri, const char *value, size_t valueLength);

#endif /* LWM2MCLIENT_H_ */
