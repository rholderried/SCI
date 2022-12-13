/**************************************************************************//**
 * \file SCISlave.c
 * \author Roman Holderried
 *
 * \brief Definitions for the SerialProtocol module.
 *
 * <b> History </b>
 * 	- 2022-01-13 - File creation
 *  - 2022-03-17 - Port to C (Originally from SerialProtocol)
 *  - 2022-12-11 - Adapted code for unified master/slave repo structure.
 *****************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "SCISlave.h"
#include "SCISlaveTransfer.h"
#include "SCISlaveDataframe.h"
#include "SCIconfig.h"
#include "CommandStucture.h"
#include "Helpers.h"
#include "SCIVariables.h"
#include "Buffer.h"


/******************************************************************************
 * Defines
 *****************************************************************************/
#define GET_SCI_ERROR_NUMBER(e) (e + SCI_ERROR_OFFSET)

/******************************************************************************
 * Global variable definition
 *****************************************************************************/
static tsSCI_SLAVE sSciSlave = tsSCI_SLAVE_DEFAULTS;
// Note: The idizes correspond to the values of the COMMAND_CB_STATUS enum values!
// static const uint8_t ui8RequestAck [5][3]   = {"ACK", "DAT", "UPS", "ERR", "NAK"};
// static const uint8_t ui8CmdIdArr[6]         = {'#', '?', '!', ':', '>', '<'};
// const uint8_t ui8ByteLength[7]              = {1,1,2,2,4,4,4};

/******************************************************************************
 * Function definitions
 *****************************************************************************/
//===================================================================================
tsSCI_VERSION SCISlaveGetVersion(void)
{
    return sSciSlave.sVersion;
}

//=============================================================================
teSCI_SLAVE_ERROR SCISlaveInit(tsSCI_SLAVE_CALLBACKS sCallbacks, const tsSCIVAR *pVarStruct, const COMMAND_CB *pCmdStruct)
{
    // Initialize the callbacks
    sSciSlave.sVarAccess.cbReadEEPROM           = sCallbacks.cbReadEEPROM;
    sSciSlave.sVarAccess.cbWriteEEPROM          = sCallbacks.cbWriteEEPROM;
    sSciSlave.sDatalink.txBlockingCallback      = sCallbacks.cbTransmitBlocking;
    sSciSlave.sDatalink.txNonBlockingCallback   = sCallbacks.cbTransmitNonBlocking;
    sSciSlave.sDatalink.txGetBusyStateCallback  = sCallbacks.cbGetTxBusyState;

    // Hand over the pointers to the var and cmd structs
    sSciSlave.sVarAccess.pVarStruct       = pVarStruct;
    sSciSlave.sSciTransfer.pCmdCBStruct   = pCmdStruct;

    // Configure data structures
    fifoBufInit(&sSciSlave.sRxFIFO, sSciSlave.ui8RxBuffer, RX_PACKET_LENGTH);
    fifoBufInit(&sSciSlave.sTxFIFO, sSciSlave.ui8TxBuffer, TX_PACKET_LENGTH);

    // Start to receive data
    SCIDatalinkStartRx(&sSciSlave.sDatalink);

    // Initialize the variable structure
    return (InitVarstruct(&sSciSlave.sVarAccess));

}

//=============================================================================
void SCISlaveReceiveData (uint8_t ui8Data)
{
    // Call the lower level datalink level functionality
    SCIDataLinkReceiveTransfer(&sSciSlave.sDatalink, &sSciSlave.sRxFIFO, ui8Data);
}


//=============================================================================
void SCISlaveStatemachine (void)
{
    // Check the lower level datalink states and set the protocol state accordingly
    if (sSciSlave.e_state > ePROTOCOL_ERROR)
    {
        // Datalink state dependend Protocol states
        switch(sSciSlave.sDatalink.rState)
        {
            case eDATALINK_RSTATE_BUSY:
                sSciSlave.e_state = ePROTOCOL_RECEIVING;
                break;
            case eDATALINK_RSTATE_PENDING:
                sSciSlave.e_state = ePROTOCOL_EVALUATING;
                break;
            default:
                break;
        }
    }

    switch(sSciSlave.e_state)
    {
        case ePROTOCOL_IDLE:
            break;
        case ePROTOCOL_RECEIVING:
            break;
        case ePROTOCOL_EVALUATING:
            {
                uint8_t *   pui8Buf;
                tsREQUEST    sReq = tsREQUEST_DEFAULTS;
                tsRESPONSE   sRsp = tsRESPONSE_DEFAULTS; 
                uint8_t     ui8_msgSize = readBuf(&sSciSlave.sRxFIFO, &pui8Buf);
                teSCI_SLAVE_ERROR  eError = eSCI_SLAVE_ERROR_NONE;

                // Parse the command (skip STX and don't care for ETX)
                eError = SCISlaveRequestParser(pui8Buf, ui8_msgSize, &sReq);

                // Take over command number and type
                sRsp.i16Num = sReq.i16Num;
                sRsp.eReqType = sReq.eReqType;

                // Execute the command
                if (eError == eSCI_SLAVE_ERROR_NONE)
                    eError = SCIProcessRequest(&sSciSlave.sSciTransfer, &sSciSlave.sVarAccess, sReq, &sRsp);

                // If there was an SCI error, return the error number with offset
                if(eError != eSCI_SLAVE_ERROR_NONE)
                    sRsp.sTransferData.ui16Error = GET_SCI_ERROR_NUMBER((uint16_t)eError);


                flushBuf(&sSciSlave.sTxFIFO);
                
                // "Put" the date into the tx buffer
                pui8Buf = sSciSlave.ui8TxBuffer;
                increaseBufIdx(&sSciSlave.sTxFIFO, SCISlaveResponseBuilder( pui8Buf,
                                                                            sSciSlave.sSciTransfer.sResponseControl.ui8ControlBits.firstPacketNotSent,
                                                                            sSciSlave.sSciTransfer.sResponseControl.ui8ControlBits.ongoing,
                                                                            &sSciSlave.sSciTransfer.sResponseControl.ui32DataIdx,
                                                                            &sRsp));

                // Reset the ongoing flag when no data is left to transmit
                if (sSciSlave.sSciTransfer.sResponseControl.sRsp.sTransferData.ui32DatLen == 0)
                    clearResponseControl(&sSciSlave.sSciTransfer);

                /// @todo Error handling -> Message too long

                if (SCIDatalinkTransmit(&sSciSlave.sDatalink, &sSciSlave.sTxFIFO))
                    sSciSlave.e_state = ePROTOCOL_SENDING;
                /// @todo Error handling?
                else 
                    sSciSlave.e_state = ePROTOCOL_IDLE;  
            }
    
            break;

        case ePROTOCOL_SENDING:

            SCIDatalinkTransmitStateMachine(&sSciSlave.sDatalink);
            
            if (SCIDatalinkGetTransmitState(&sSciSlave.sDatalink) == eDATALINK_TSTATE_READY)
            {
                SCIDatalinkAcknowledgeTx(&sSciSlave.sDatalink);
                sSciSlave.e_state = ePROTOCOL_IDLE;

                // Clear Datalink State
                SCIDatalinkStartRx(&sSciSlave.sDatalink);
            }
            
            break;
        
        default:
            break;
    }
}

//=============================================================================
teSCI_SLAVE_ERROR SCISlaveGetVarFromStruct(int16_t i16VarNum, tsSCIVAR* pVar)
{
    return GetVar(&sSciSlave.sVarAccess, pVar, i16VarNum);
}