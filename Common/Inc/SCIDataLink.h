/**************************************************************************//**
 * \file SCIDataLink.h
 * \author Roman Holderried
 *
 * \brief Data link layer functionality for the SCI protocol.
 *
 * <b> History </b>
 * 	- 2022-11-17 - Copy from SCI
 *  - 2022-12-13 - Adapted code for unified master/slave repo structure.
 *
 * <b> TODOs </b>
 * @todo Addressable clients
 * @todo Checksum
 *****************************************************************************/
#ifndef _SCIDATALINK_H_
#define _SCIDATALINK_H_

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
typedef void(*BLOCKING_TX_CB)(uint8_t*, uint8_t);
typedef uint8_t(*NONBLOCKING_TX_CB)(uint8_t*, uint8_t);
typedef bool(*GET_BUSY_STATE_CB)(void);

typedef enum
{
    eDATALINK_RSTATE_ERROR      = -1,
    eDATALINK_RSTATE_IDLE       = 0,
    eDATALINK_RSTATE_WAIT_STX   = 1,
    eDATALINK_RSTATE_BUSY       = 2,
    eDATALINK_RSTATE_PENDING    = 3
}teDATALINK_RECEIVE_STATE;

typedef enum
{
    eDATALINK_DBGSTATE_IDLE,
    eDATALINK_DBGSTATE_S1,
    eDATALINK_DBGSTATE_S2,
    eDATALINK_DBGSTATE_S3,
    eDATALINK_DBGSTATE_PENDING
}teDATALINK_DBGACT_STATE;

typedef enum
{
    eDATALINK_TSTATE_ERROR     = -1,
    eDATALINK_TSTATE_IDLE      =  0,
    eDATALINK_TSTATE_SEND_STX,
    eDATALINK_TSTATE_SEND_BUFFER,
    eDATALINK_TSTATE_SEND_ETX,
    eDATALINK_TSTATE_READY
}teDATALINK_TRANSMIT_STATE;

typedef enum
{
    eDATALINK_ERROR_NONE,
    eDATALINK_ERROR_CHECKSUM,
    eDATALINK_ERRIR_TIMEOUT
}teDATALINK_ERROR;

typedef struct
{
    teDATALINK_RECEIVE_STATE rState;
    teDATALINK_TRANSMIT_STATE tState;
    teDATALINK_DBGACT_STATE dbgActState;

    DBG_FCN_CB dbgFcnArray[MAX_NUMBER_OF_DBG_FUNCTIONS];
    BLOCKING_TX_CB txBlockingCallback;
    NONBLOCKING_TX_CB txNonBlockingCallback;
    GET_BUSY_STATE_CB txGetBusyStateCallback;

    struct 
    {
        uint8_t * pui8_buf;
        uint8_t ui8_bufLen;
    }sTxInfo;

    struct
    {
        uint32_t ui32BytesToGo;
        uint8_t ui8MsgByteCnt;
    }sRxInfo;

}tsDATALINK;

#define tsDATALINK_DEFAULTS {eDATALINK_RSTATE_IDLE, eDATALINK_TSTATE_IDLE, eDATALINK_DBGSTATE_IDLE, {NULL}, NULL, NULL, NULL, {NULL, 0}, {0,0}}



/******************************************************************************
 * Function declarations
 *****************************************************************************/
void SCIDataLinkReceiveTransfer(tsDATALINK *p_inst, tsFIFO_BUF *p_rBuf, uint8_t ui8_data);
void SCIDataLinkReceiveStream(tsDATALINK *p_inst, tsFIFO_BUF *p_rBuf, uint8_t ui8_data);
teDATALINK_RECEIVE_STATE SCIDatalinkGetReceiveState(tsDATALINK *p_inst);
teDATALINK_TRANSMIT_STATE SCIDatalinkGetTransmitState(tsDATALINK *p_inst);

bool SCIDatalinkTransmit(tsDATALINK *p_inst, tsFIFO_BUF * p_tBuf);//uint8_t *pui8_buf, uint8_t ui8_bufLen);
void SCIDatalinkTransmitStateMachine(tsDATALINK *p_inst);
void SCIDatalinkAcknowledgeRx(tsDATALINK *p_inst);
void SCIDatalinkAcknowledgeTx(tsDATALINK *p_inst);
void SCIDatalinkStartRx(tsDATALINK *p_inst);

#endif // _SCIDATALINK_H_