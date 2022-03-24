/**************************************************************************//**
 * \file DataLink.h
 * \author Roman Holderried
 *
 * \brief Data link layer functionality for the SCI protocol.
 *
 * <b> History </b>
 * 	- 2022-03-18 - File creation
 *****************************************************************************/
#ifndef _DATALINK_H_
#define _DATALINK_H_

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "Buffer.h"

/******************************************************************************
 * Defines
 *****************************************************************************/
#define STX 0x02
#define ETX 0x03

#define MAX_NUMBER_OF_DBG_FUNCTIONS 5
/******************************************************************************
 * Type definitions
 *****************************************************************************/

typedef void(*DBG_FCN_CB)(void);
typedef uint8_t(*TX_CB)(uint8_t*, uint8_t);
typedef bool(*GET_BUSY_STATE_CB)(void);

typedef enum
{
    eDATALINK_RSTATE_ERROR     = -1,
    eDATALINK_RSTATE_IDLE       = 0,
    eDATALINK_RSTATE_BUSY       = 1,
    eDATALINK_RSTATE_PENDING    = 2
}DATALINK_RECEIVE_STATE;

typedef enum
{
    eDATALINK_DBGSTATE_IDLE,
    eDATALINK_DBGSTATE_S1,
    eDATALINK_DBGSTATE_S2,
    eDATALINK_DBGSTATE_S3,
    eDATALINK_DBGSTATE_PENDING
}DATALINK_DBGACT_STATE;

typedef enum
{
    eDATALINK_TSTATE_ERROR     = -1,
    eDATALINK_TSTATE_IDLE      =  0,
    eDATALINK_TSTATE_SEND_STX,
    eDATALINK_TSTATE_SEND_BUFFER,
    eDATALINK_TSTATE_SEND_ETX,
    eDATALINK_TSTATE_READY
}DATALINK_TRANSMIT_STATE;

typedef enum
{
    eDATALINK_ERROR_NONE,
    eDATALINK_ERROR_CHECKSUM,
    eDATALINK_ERRIR_TIMEOUT
}DATALINK_ERROR;

typedef struct
{
    DATALINK_RECEIVE_STATE rState;
    DATALINK_TRANSMIT_STATE tState;
    DATALINK_DBGACT_STATE dbgActState;

    DBG_FCN_CB dbgFcnArray[MAX_NUMBER_OF_DBG_FUNCTIONS];
    TX_CB txCallback;
    GET_BUSY_STATE_CB txGetBusyStateCallback;

    struct 
    {
        uint8_t * pui8_buf;
        uint8_t ui8_bufLen;
    }txInfo;

}DATALINK;

#define DATALINK_DEFAULT {eDATALINK_RSTATE_IDLE, eDATALINK_TSTATE_IDLE, eDATALINK_DBGSTATE_IDLE, NULL, NULL, NULL, {NULL, 0}}



/******************************************************************************
 * Function declarations
 *****************************************************************************/
void receive(DATALINK *p_inst, FIFO_BUF *p_rBuf, uint8_t ui8_data);
DATALINK_RECEIVE_STATE getDatalinkReceiveState(DATALINK *p_inst);
DATALINK_TRANSMIT_STATE getDatalinkTransmitState(DATALINK *p_inst);

bool transmit(DATALINK *p_inst, FIFO_BUF * p_tBuf);//uint8_t *pui8_buf, uint8_t ui8_bufLen);
void transmitStateMachine(DATALINK *p_inst);
void acknowledgeTx(DATALINK *p_inst);




#endif // _DATALINK_H_