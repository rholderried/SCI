/**************************************************************************//**
 * \file SCIMaster.c
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
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "SCIMaster.h"
#include "SCIDataframe.h"
#include "SCITransfer.h"
#include "SCIDataLink.h"
#include "Buffer.h"
#include "Helpers.h"

/******************************************************************************
 * Global variable definition
 *****************************************************************************/
static tsSCI_MASTER sSciMaster = tsSCI_MASTER_DEFAULTS;

/******************************************************************************
 * Function declarations
 *****************************************************************************/
void SCIMasterInit (tsSCI_MASTER_CALLBACKS sCallbacks)
{
    // Connect the internal callbacks
    sSciMaster.sSCITransfer.sCallbacks.InitiateStreamCB = SCIInitiateStreamReceive;
    sSciMaster.sSCITransfer.sCallbacks.FinishStreamCB = SCIFinishStreamReceive;
    sSciMaster.sSCITransfer.sCallbacks.ReleaseProtocolCB = SCIReleaseProtocol;
    sSciMaster.sSCITransfer.sCallbacks.RequestCB = SCIInitiateRequest;

    // Connect the external callbacks
    sSciMaster.sSCITransfer.sCallbacks.GetVarCB = sCallbacks.GetVarExternalCB;
    sSciMaster.sSCITransfer.sCallbacks.SetVarCB = sCallbacks.SetVarExternalCB;
    sSciMaster.sSCITransfer.sCallbacks.CommandCB = sCallbacks.CommandExternalCB;
    sSciMaster.sSCITransfer.sCallbacks.UpstreamCB = sCallbacks.UpstreamExternalCB;
    sSciMaster.sDatalink.txBlockingCallback = sCallbacks.BlockingTxExternalCB;
    sSciMaster.sDatalink.txNonBlockingCallback = sCallbacks.NonBlockingTxExternalCB;
    sSciMaster.sDatalink.txGetBusyStateCallback = sCallbacks.GetTxBusyStateExternalCB;

    // Configure data structures
    fifoBufInit(&sSciMaster.sRxFIFO, sSciMaster.ui8RxBuffer, RX_PACKET_LENGTH);
    fifoBufInit(&sSciMaster.sTxFIFO, sSciMaster.ui8TxBuffer, TX_PACKET_LENGTH);
}

//=============================================================================
void SCIMasterSM (void)
{
    teSCI_ERROR eError = eSCI_ERROR_NONE;

    switch (sSciMaster.eProtocolState)
    {
        case ePROTOCOL_IDLE:
            break;

        case ePROTOCOL_SENDING:

            if (sSciMaster.sDatalink.tState != eDATALINK_TSTATE_READY)
                SCIDatalinkTransmitStateMachine(&sSciMaster.sDatalink);
            
            // Transition to next protocol state if tx is ready
            else
            {
                // Reset Datalink Tx State
                SCIDatalinkAcknowledgeTx(&sSciMaster.sDatalink);

                // Reset the Rx Buffer
                // flushBuf(&sSciMaster.sRxFIFO);

                sSciMaster.eProtocolState = ePROTOCOL_RECEIVING;

                // Enable data receive
                SCIDatalinkStartRx(&sSciMaster.sDatalink);
            }    
            break;

        case ePROTOCOL_RECEIVING:

            // Wait until all data has been received
            if (sSciMaster.sDatalink.rState == eDATALINK_RSTATE_PENDING)
            {
                SCIDatalinkAcknowledgeRx(&sSciMaster.sDatalink);

                sSciMaster.eProtocolState = ePROTOCOL_EVALUATING;
            }

            break;

        case ePROTOCOL_EVALUATING:
            {
                tsRESPONSE sRsp = tsRESPONSE_DEFAULTS;
                uint8_t *pui8Buf;
                uint8_t ui8DframeLen = readBuf(&sSciMaster.sRxFIFO, &pui8Buf);

                // Parse the response
                if (sSciMaster.ui8RecMode == SCI_RECEIVE_MODE_TRANSFER)
                    SCIMasterResponseParser(pui8Buf, ui8DframeLen, &sRsp);
                else if (sSciMaster.ui8RecMode == SCI_RECEIVE_MODE_STREAM)
                    SCIMasterStreamParser(pui8Buf, ui8DframeLen, &sRsp);

                // Process the response
                SCITransferControl(&sSciMaster.sSCITransfer, sRsp);
            }
            break;

        default:
            break;
    }
}

//=============================================================================
void SCIReceive (uint8_t *pui8RecBuf, uint16_t ui16ByteCount)
{
    uint8_t i = 0;

    while (ui16ByteCount)
    {
        // Call the datalink-level data receiver
        if (sSciMaster.ui8RecMode == SCI_RECEIVE_MODE_TRANSFER)
            SCIDataLinkReceiveTransfer(&sSciMaster.sDatalink, &sSciMaster.sRxFIFO, pui8RecBuf[i]);
        else if (sSciMaster.ui8RecMode == SCI_RECEIVE_MODE_STREAM)
            SCIDataLinkReceiveStream(&sSciMaster.sDatalink, &sSciMaster.sRxFIFO, pui8RecBuf[i]);

        i++;
        ui16ByteCount--;
    }
}

//=============================================================================
void SCIInitiateStreamReceive (uint32_t ui32ByteCount)
{
    sSciMaster.ui8RecMode = SCI_RECEIVE_MODE_STREAM;
    sSciMaster.sDatalink.sRxInfo.ui32BytesToGo = ui32ByteCount;
}

//=============================================================================
void SCIFinishStreamReceive (void)
{
    sSciMaster.ui8RecMode = SCI_RECEIVE_MODE_TRANSFER;
    sSciMaster.sDatalink.sRxInfo.ui32BytesToGo = 0;
}

//=============================================================================
bool SCIInitiateRequest (tsREQUEST sReq)
{
    uint8_t ui8Size = 0;

    // Interface busy -> Don't start transmission
    if (sSciMaster.eProtocolState != ePROTOCOL_IDLE)
        return false;

    // Prepare transmission buffer
    flushBuf(&sSciMaster.sTxFIFO);

    // Assemble message
    if (SCIMasterRequestBuilder(sSciMaster.sTxFIFO.pui8_bufPtr, &ui8Size, sReq) == eSCI_ERROR_NONE)
    {
        increaseBufIdx(&sSciMaster.sTxFIFO, ui8Size);

        SCIDatalinkTransmit(&sSciMaster.sDatalink, &sSciMaster.sTxFIFO);

        sSciMaster.eProtocolState = ePROTOCOL_SENDING;
    }
    else
    {
        // TODO: What to do on error?
        ;
    }

    return true;
}

//=============================================================================
void SCIReleaseProtocol (void)
{
    sSciMaster.eProtocolState = ePROTOCOL_IDLE;
}

//=============================================================================
void SCIRequestGetVar (int16_t i16VarNum)
{
    // Request generation by the Transfer control module
    SCITransferStart(&sSciMaster.sSCITransfer, eREQUEST_TYPE_GETVAR, i16VarNum, NULL, 0);
}

//=============================================================================
void SCIRequestSetVar (int16_t i16VarNum, tuREQUESTVALUE uVal)
{
    // Request generation by the Transfer control module
    SCITransferStart(&sSciMaster.sSCITransfer, eREQUEST_TYPE_SETVAR, i16VarNum, &uVal, 1);
}

//=============================================================================
void SCIRequestCommand (int16_t i16CmdNum, tuREQUESTVALUE *puValArr, uint8_t ui8ArgNum)
{
    // Request generation by the Transfer control module
    SCITransferStart(&sSciMaster.sSCITransfer, eREQUEST_TYPE_COMMAND, i16CmdNum, puValArr, ui8ArgNum);
}

//=============================================================================
tePROTOCOL_STATE SCIGetProtocolState (void)
{
    return sSciMaster.eProtocolState;
}
