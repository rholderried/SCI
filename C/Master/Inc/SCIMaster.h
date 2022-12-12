/**************************************************************************//**
 * \file SCIMaster.h
 * \author Roman Holderried
 *
 * \brief Master functions for SCI communications.
 * 
 * This serial protocol has been initially written for the MMX heater controller
 * module. It provides data read/write access and a command interface to the 
 * application.
 *
 * <b> History </b>
 * 	- 2022-11-17 - File creation -
 *  - 2022-12-12 - Adapted code for unified master/slave repo structure.
 * 
 * <b> TODOs </b>
 * @todo Response Timeout
 * @todo Clean Error tracking and response
 *****************************************************************************/

#ifndef _SCIMASTER_H_
#define _SCIMASTER_H_

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "SCIMasterTransfer.h"
#include "Buffer.h"
#include "SCIDataLink.h"
#include "SCICommon.h"

/******************************************************************************
 * Defines
 *****************************************************************************/

#define SCI_RECEIVE_MODE_TRANSFER   0
#define SCI_RECEIVE_MODE_STREAM     1

/******************************************************************************
 * Type definitions
 *****************************************************************************/

typedef teTRANSFER_ACK (*SETVAR_CB)(teREQUEST_ACKNOWLEDGE eAck, int16_t i16Num, uint16_t ui16ErrNum);
typedef teTRANSFER_ACK (*GETVAR_CB)(teREQUEST_ACKNOWLEDGE eAck, int16_t i16Num, uint32_t ui32Data, uint16_t ui16ErrNum);
typedef teTRANSFER_ACK (*COMMAND_CB)(teREQUEST_ACKNOWLEDGE eAck, int16_t i16Num, uint32_t *pui32Data, uint8_t ui8DataCnt, uint16_t ui16ErrNum);
typedef teTRANSFER_ACK (*UPSTREAM_CB)(int16_t i16Num, uint8_t *pui8Data, uint32_t ui32ByteCnt);

typedef struct
{
    // Result external callbacks
    SETVAR_CB SetVarExternalCB;
    GETVAR_CB GetVarExternalCB;
    COMMAND_CB CommandExternalCB;
    UPSTREAM_CB UpstreamExternalCB;

    // Transmission related external callbacks
    void        (*BlockingTxExternalCB)(uint8_t* pui8Buf, uint8_t ui8Len);
    uint8_t     (*NonBlockingTxExternalCB)(uint8_t* pui8Buf, uint8_t ui8Len);
    bool        (*GetTxBusyStateExternalCB)(void);

}tsSCI_MASTER_CALLBACKS;

#define tsSCI_MASTER_CALLBACKS_DEFAULTS {NULL}

/** \brief SCI Master main structure */
typedef struct
{
    tsSCI_VERSION   sVersion;
    tePROTOCOL_STATE eProtocolState;

    uint8_t ui8RxBuffer[RX_PACKET_LENGTH];   /*!< RX buffer space. */ 
    uint8_t ui8TxBuffer[TX_PACKET_LENGTH];   /*!< TX buffer space. */ 

    tsFIFO_BUF sRxFIFO;  /*!< RX buffer management. */ 
    tsFIFO_BUF sTxFIFO;  /*!< TX buffer management. */

    uint8_t ui8RecMode;  /*!< Current receive mode of the protocol */
    tsDATALINK     sDatalink;
    // SCI_COMMANDS sciCommands;   /*!< Commands variable structure. */

    tsSCI_TRANSFER sSCITransfer;

}tsSCI_MASTER;

#define tsSCI_MASTER_DEFAULTS { \
    tsSCI_VERSION_VALUE, \
    ePROTOCOL_IDLE, \
    {0},{0}, \
    tsFIFO_BUF_DEFAULTS, \
    tsFIFO_BUF_DEFAULTS, \
    SCI_RECEIVE_MODE_TRANSFER, \
    tsDATALINK_DEFAULTS, \
    tsSCI_TRANSFER_DEFAULTS \
}

/******************************************************************************
 * Function declarations
 *****************************************************************************/
/** \brief Initializes the SCI Master.
 * 
 * @param sCallbacks    External functions to call by the SCI Master.
*/
void SCIMasterInit (tsSCI_MASTER_CALLBACKS sCallbacks);

/** \brief Main state machine for the SCI master.
 * 
 * Handles the SCI protocol.
*/
void SCIMasterSM (void);

/** \brief Non blocking SCI data transmission.
 * 
 * Passes protocol handling to the SCIMasterSM state machine.
 * 
 * @param eCmdType  Command type to be sent
 * @param i16Num    Command number
 * @param uVal      Pointer to the values to be sent
*/
// void _SCIMasterQueryNonBlocking (teREQUEST_TYPE eCmdType, int16_t i16CmdNum, tuREQUESTVALUE *uVal, int16_t i16ArgNum);

/** \brief High level receive routine.
 * 
 * @param pui8RecBuf    Pointer to the receive buffer or FIFO
 * @param ui8ByteCount  Number of bytes to process
*/
void SCIReceive (uint8_t *pui8RecBuf, uint16_t ui8ByteCount);

/** \brief Switch the receive mode of the protocol.
 * 
 * @param ui32ByteCount    Number of bytes that are to be expected from the stream
*/
void SCIInitiateStreamReceive (uint32_t ui32ByteCount);

/** \brief End Stream receive immediately.*/
void SCIFinishStreamReceive (void);

/** \brief Initiate a SCI request.
 * 
 * @param sReq Request data structure
 * 
 * @returns Success indicator
*/
bool SCIInitiateRequest (tsREQUEST sReq);

/** \brief Releases the SCI protocol into IDLE state.*/
void SCIReleaseProtocol (void);

/******************************************************************************
 * Interface functions
 *****************************************************************************/
/** \brief Initiate a GETVAR request
 * 
 * @param i16VarNum Variable number to request
 */
void SCIRequestGetVar (int16_t i16VarNum);

/** \brief Initiate a SETVAR request
 * 
 * @param i16VarNum Variable number to request
 * @param uVal      Variable value to set
 */
void SCIRequestSetVar (int16_t i16VarNum, tuREQUESTVALUE uVal);

/** \brief Initiate a COMMAND request
 * 
 * @param i16CmdNum Variable number to request
 * @param puValArr  Pointer to the value array to transmit
 * @param ui8ArgNum Number of elements in the value array
 */
void SCIRequestCommand (int16_t i16CmdNum, tuREQUESTVALUE *puValArr, uint8_t ui8ArgNum);

/** \brief Returns the current protocol state
 * 
 * @returns SCI protocol state
 */
tePROTOCOL_STATE SCIGetProtocolState (void);

#ifdef __cplusplus
}
#endif

#endif //_SCIMASTER_H_