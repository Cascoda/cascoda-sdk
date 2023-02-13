/******************************************************************************/
/******************************************************************************/
/****** Cascoda Ltd. 2015, CA-821X OT Code                           ******/
/******************************************************************************/
/******************************************************************************/
/****** Wing Commander message formats for the OT API                ******/
/******************************************************************************/
/******************************************************************************/
/****** Revision           Notes                                         ******/
/******************************************************************************/
/****** 0.1  06/09/15  PB  Release Baseline                              ******/
/******************************************************************************/
/******************************************************************************/

#include "string.h"

#ifndef OT_API_MESSAGES_H
#define OT_API_MESSAGES_H

/** Downwards command ids */
#define OT_CMD_IFCONFIG (0x00)
#define OT_CMD_THREAD (0x01)
#define OT_CMD_SET (0x02)
#define OT_CMD_GET (0x03)
#define OT_CMD_STATE (0x04)
#define OT_CMD_FACTORY_RESET (0x05)
#define OT_CMD_APPLICATION_CMD (0x06)

/** Upwards command ids */
#define OT_CNF_IFCONFIG (0x80)
#define OT_CNF_THREAD (0x81)
#define OT_CNF_SET (0x82)
#define OT_CNF_GET (0x83)
#define OT_CNF_STATE (0x84)
#define OT_CNF_FACTORY_RESET (0x85)
#define OT_CNF_APPLICATION_CMD (0x86)

typedef struct OT_IF_CONFIG
{
	/** CommandId always THREAD_DOWNLINK_ID */
	u8_t CommandId;
	/** Length always 2 */
	u8_t Length;
	/** DispatchId always OT_CMD_IFCONFIG */
	u8_t DispatchId;
	/** DOWN (0), UP (1) or QUERY (2) */
	u8_t Control;
} OT_IF_CONFIG_t;

typedef struct OT_THREAD
{
	/** CommandId always THREAD_DOWNLINK_ID */
	u8_t CommandId;
	/** Length always 2 */
	u8_t Length;
	/** DispatchId always OT_CMD_ID_THREAD */
	u8_t DispatchId;
	/** STOP (0), START (1) */
	u8_t Control;
} OT_THREAD_t;

typedef struct OT_SET
{
	/** CommandId always THREAD_DOWNLINK_ID */
	u8_t CommandId;
	/** Length always 2 more than AttributeValue length */
	u8_t Length;
	/** DispatchId always OT_CMD_SET */
	u8_t DispatchId;
	/** AttributeId */
	u8_t AttributeId;
	/* AttributeValue depends on AttributeId */
	u8_t AttributeValue[];
} OT_SET_t;

typedef struct OT_GET
{
	/** CommandId always THREAD_DOWNLINK_ID */
	u8_t CommandId;
	/** Length always 2 */
	u8_t Length;
	/** DispatchId always OT_CMD_GET */
	u8_t DispatchId;
	/** AttributeId  */
	u8_t AttributeId;
} OT_GET_t;

typedef struct OT_STATE
{
	/** CommandId always THREAD_DOWNLINK_ID */
	u8_t CommandId;
	/** Length always 2 */
	u8_t Length;
	/** DispatchId always OT_CMD_STATE */
	u8_t DispatchId;
	/** QUERY (0), DETACHED (1) or CHILD (2) or ROUTER (3) or LEADER (4)*/
	u8_t Control;
} OT_STATE_t;

typedef struct OT_FACTORY_RESET
{
	/** CommandId always THREAD_DOWNLINK_ID */
	u8_t CommandId;
	/** Length always 2 */
	u8_t Length;
	/** DispatchId always OT_CMD_FACTORY_RESET */
	u8_t DispatchId;
	/** Not used yet. Will choose application or openthread flash areas*/
	u8_t Control;
} OT_FACTORY_RESET_t;

typedef struct OT_APPLICATION_CMD
{
	/** CommandId always THREAD_DOWNLINK_ID */
	u8_t CommandId;
	/** Length always 1 + length of Command[] below */
	u8_t Length;
	/** DispatchId always OT_CMD_APPLICATION_CMD */
	u8_t DispatchId;
	/** Command length - will be Length-1 */
	u8_t CommandLength;
	/** Application command. Its length given by CommandLength */
	u8_t Command[];
} OT_APPLICATION_CMD_t;

typedef struct OT_GEN_CNF
{
	/** CommandId always THREAD_UPLINK_ID */
	u8_t CommandId;
	/** Length always 2 */
	u8_t Length;
	/** DispatchId OT_CNF_SET, OT_CNF_THREAD, OT_CNF_IFCONFIG */
	u8_t DispatchId;
	/** otError Status */
	u8_t Status;
} OT_GEN_CNF_t;

typedef struct OT_GET_CNF
{
	/** CommandId always THREAD_UPLINK_ID */
	u8_t CommandId;
	/** Length always 3 + length of AttributeValue */
	u8_t Length;
	/** DispatchId OT_CNF_GET */
	u8_t DispatchId;
	/** otError Status */
	u8_t Status;
	/** AttributeId */
	u8_t AttributeId;
	/** AttributeValue depends on AttributeId */
	u8_t AttributeValue[];
} OT_GET_CNF_t;

typedef struct OT_STATE_CNF
{
	/** CommandId always THREAD_UPLINK_ID */
	u8_t CommandId;
	/** Length always 3 */
	u8_t Length;
	/** DispatchId OT_STATE_CNF */
	u8_t DispatchId;
	/** otError Status */
	u8_t Status;
	/** State of device; Detached(1), Child(2) */
	u8_t State;
} OT_STATE_CNF_t;

typedef union OTApiMsg
{
	OT_IF_CONFIG_t       *IfConfig;
	OT_THREAD_t          *Thread;
	OT_SET_t	         *Set;
	OT_GET_t	         *Get;
	OT_STATE_t           *State;
	OT_FACTORY_RESET_t   *FactoryReset;
	OT_APPLICATION_CMD_t *ApplicationCmd;
	u8_t                 *Ptr;
} OTApiMsg_t;

/* Attribute ids */
#define OT_ATTR_CHANNEL (0)
#define OT_ATTR_PANID (1)
#define OT_ATTR_MODE_RX_ON_WHEN_IDLE (2)
#define OT_ATTR_MODE_SECURE (3)
#define OT_ATTR_MODE_FFD (4)
#define OT_ATTR_MODE_NETDATA (5)
#define OT_ATTR_POLL_PERIOD (6)
#define OT_ATTR_MASTER_KEY (7)
#define OT_ATTR_NETWORK_NAME (8)
#define OT_ATTR_EXT_PANID (9)
#define OT_ATTR_KEY_SEQUENCE (10)
#define OT_ATTR_EXT_ADDR (11)
#define OT_ATTR_MESH_LOCAL_PREFIX (12)

#define OT_ATTR_CHILD_TIMEOUT (13)
#define OT_ATTR_CONTEXT_REUSE_DELAY (14)
#define OT_ATTR_LEADER_DATA (15)
#define OT_ATTR_LEADER_WEIGHT (16)
#define OT_ATTR_NETWORK_ID_TIMEOUT (17)
#define OT_ATTR_RLOC16 (18)
#define OT_ATTR_ROUTER_UPGRADE_THRESHOLD (19)

#define OT_ATTR_AUTO_START (20)

#endif // OT_API_MESSAGES_H
