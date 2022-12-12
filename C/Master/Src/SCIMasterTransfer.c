/**************************************************************************//**
 * \file SCIMasterTransfer.c
 * \author Roman Holderried
 *
 * \brief Transfer control functions for the master module
 *
 * <b> History </b>
 * 	- 2022-11-21 - File creation 
 *  - 2022-12-11 - Adapted code for unified master/slave repo structure.
 * 
 * TODOs:
 * ======
 * @todo    Proper error handling - What is happening if there is a transfer
 *          error when handling a multiple-message command (or upstream)?
 * 
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "SCIMasterTransfer.h"

/******************************************************************************
 * Global variable definition
 *****************************************************************************/

/******************************************************************************
 * Function definitions
 *****************************************************************************/
bool SCITransferStart (tsSCI_TRANSFER *psSciTransfer, teREQUEST_TYPE eReqType, int16_t i16CmdNum, tuREQUESTVALUE *uVal, uint8_t ui8ArgNum)
{
    tsREQUEST sReq = tsREQUEST_DEFAULTS;
    // Take over the arguments
    sReq.eReqType       = eReqType;
    sReq.i16Num         = i16CmdNum;
    sReq.uValArr        = uVal;
    sReq.ui8ValArrLen   = ui8ArgNum;

    if(!psSciTransfer->sCallbacks.RequestCB(sReq))
        return false;

    psSciTransfer->sTransferInfo.sReq = sReq;
}

bool SCITransferControl (tsSCI_TRANSFER *psSciTransfer, tsRESPONSE sRsp)
{
    teTRANSFER_ACK eTransferAck = eTRANSFER_ACK_ABORT;
    bool ret = true;

    switch (sRsp.eReqType)
    {
        case eREQUEST_TYPE_SETVAR:
            if (psSciTransfer->sCallbacks.GetVarCB != NULL)
            {
                eTransferAck = psSciTransfer->sCallbacks.SetVarCB(sRsp.eReqAck, sRsp.i16Num, sRsp.sTransferData.ui16Error);
            }

            if (eTransferAck != eTRANSFER_ACK_REPEAT_REQUEST)
                psSciTransfer->sCallbacks.ReleaseProtocolCB();
            else
                ; // TODO: Repeat?
            break;
        
        case eREQUEST_TYPE_GETVAR:
            if (psSciTransfer->sCallbacks.GetVarCB != NULL)
            {
                eTransferAck = psSciTransfer->sCallbacks.GetVarCB(sRsp.eReqAck, sRsp.i16Num, sRsp.sTransferData.puRespVals[0].ui32_hex, sRsp.sTransferData.ui16Error);
            }

            if (eTransferAck != eTRANSFER_ACK_REPEAT_REQUEST)
                psSciTransfer->sCallbacks.ReleaseProtocolCB();
            else
                ; // TODO: Repeat?

            break;

        case eREQUEST_TYPE_COMMAND:

            switch (sRsp.eReqAck)
            {
                case eREQUEST_ACK_STATUS_SUCCESS_DATA:

                    // Generate a transfer value buffer and copy data
                    // In first message
                    if (psSciTransfer->sTransferInfo.ui32TransferCnt == 0)
                    {
                        psSciTransfer->sTransferInfo.ui32ExpectedDataCnt = sRsp.sTransferData.ui32DatLen;

                        // Allocate the memory for the COMMAND results
                        psSciTransfer->sTransferInfo.uTransferResults = malloc(psSciTransfer->sTransferInfo.ui32ExpectedDataCnt * sizeof(tuRESPONSEVALUE));

                        // TODO: Handling of not enough memory ?!?
                        if(psSciTransfer->sTransferInfo.uTransferResults == NULL)
                            return false;
                    }

                    // Copy buffer values into the transfer memory
                    memcpy(&psSciTransfer->sTransferInfo.uTransferResults[psSciTransfer->sTransferInfo.ui32ReceivedDataCnt], 
                            sRsp.sTransferData.puRespVals, 
                            sRsp.sTransferData.ui32DatLen * sizeof(tuRESPONSEVALUE));

                    psSciTransfer->sTransferInfo.ui32ReceivedDataCnt += psSciTransfer->sTransferInfo.ui8MessageDataCnt;

                    // Increment number of COMMAND transfers
                    psSciTransfer->sTransferInfo.ui32TransferCnt++;

                    // All command transfers ready
                    if (psSciTransfer->sTransferInfo.ui32ExpectedDataCnt == 
                        psSciTransfer->sTransferInfo.ui32ReceivedDataCnt)
                    {
                        // Callback invocation
                        if (psSciTransfer->sCallbacks.CommandCB != NULL)
                        {
                            eTransferAck = psSciTransfer->sCallbacks.CommandCB(sRsp.eReqAck, sRsp.i16Num, &psSciTransfer->sTransferInfo.uTransferResults[0].ui32_hex, psSciTransfer->sTransferInfo.ui32ReceivedDataCnt, sRsp.sTransferData.ui16Error);
                        }

                        // Free data memory
                        free(psSciTransfer->sTransferInfo.uTransferResults);

                        // Reset the count variables
                        psSciTransfer->sTransferInfo.ui32ReceivedDataCnt = 0;
                        psSciTransfer->sTransferInfo.ui32TransferCnt = 0;
                        psSciTransfer->sTransferInfo.ui32ExpectedDataCnt = 0;
                        psSciTransfer->sTransferInfo.ui8MessageDataCnt = 0;

                        if (eTransferAck != eTRANSFER_ACK_REPEAT_REQUEST)
                            psSciTransfer->sCallbacks.ReleaseProtocolCB();
                        else
                            ; // TODO: Repeat?
                    }
                    // Invoke command again to get the remaining data
                    else
                    {
                        // For all consecutive transfers, parameters do not have to be passed.
                        psSciTransfer->sTransferInfo.sReq.ui8ValArrLen = 0;

                        psSciTransfer->sCallbacks.ReleaseProtocolCB();
                        psSciTransfer->sCallbacks.RequestCB(psSciTransfer->sTransferInfo.sReq);
                    }
                    break;

                // Upstream invocation
                case eREQUEST_ACK_STATUS_SUCCESS_UPSTREAM:
                {
                    tsREQUEST sUpstreamRequest = tsREQUEST_DEFAULTS;

                    // Allocate memory for the upstream data
                    psSciTransfer->sTransferInfo.pui8UpstreamBuffer = malloc(sRsp.sTransferData.ui32DatLen);

                    // TODO: What to do if memory allocation failed?
                    if (psSciTransfer->sTransferInfo.pui8UpstreamBuffer == NULL)
                        return false;

                    psSciTransfer->sTransferInfo.ui32ExpectedDataCnt = sRsp.sTransferData.ui32DatLen;

                    // Switch the receive mode to stream
                    psSciTransfer->sCallbacks.InitiateStreamCB(psSciTransfer->sTransferInfo.ui32ExpectedDataCnt);

                    // Generate new upstream request message
                    sUpstreamRequest.eReqType = eREQUEST_TYPE_UPSTREAM;
                    sUpstreamRequest.i16Num = psSciTransfer->sTransferInfo.sReq.i16Num;

                    // Initiate the upstream request
                    psSciTransfer->sCallbacks.ReleaseProtocolCB();
                    psSciTransfer->sCallbacks.RequestCB(sUpstreamRequest);

                    psSciTransfer->sTransferInfo.sReq = sUpstreamRequest;

                    break;
                }

                // In case of a regular COMMAND without result values or an error
                default:
                    if (psSciTransfer->sCallbacks.CommandCB != NULL)
                    {
                        eTransferAck = psSciTransfer->sCallbacks.CommandCB(sRsp.eReqAck, sRsp.i16Num, NULL, 0, sRsp.sTransferData.ui16Error);
                    }
                    
                    psSciTransfer->sCallbacks.ReleaseProtocolCB();
                    break;

            // TODO: Transfer handling depending on eTransferAck
            }
            break;
        
        case eREQUEST_TYPE_UPSTREAM:

            // Copy transfer data from receive buffer into upstream memory
            memcpy(&psSciTransfer->sTransferInfo.pui8UpstreamBuffer[psSciTransfer->sTransferInfo.ui32ReceivedDataCnt], 
                    sRsp.sTransferData.pui8UpStreamBuf, psSciTransfer->sTransferInfo.ui8MessageDataCnt);
            
            psSciTransfer->sTransferInfo.ui32ReceivedDataCnt += psSciTransfer->sTransferInfo.ui8MessageDataCnt;

            // There is additional data to transfer
            if (psSciTransfer->sTransferInfo.ui32ReceivedDataCnt < psSciTransfer->sTransferInfo.ui32ExpectedDataCnt)
            {
                // New request
                psSciTransfer->sCallbacks.RequestCB(psSciTransfer->sTransferInfo.sReq);
            }
            // All data arrived
            else
            {
                // Switch back receive mode
                psSciTransfer->sCallbacks.FinishStreamCB();

                // Call the Upstream CB
                if (psSciTransfer->sCallbacks.UpstreamCB != NULL)
                {
                    psSciTransfer->sCallbacks.UpstreamCB(psSciTransfer->sTransferInfo.sReq.i16Num, 
                                                        psSciTransfer->sTransferInfo.pui8UpstreamBuffer,
                                                        psSciTransfer->sTransferInfo.ui32ReceivedDataCnt);
                }

                // Reset the count variables
                psSciTransfer->sTransferInfo.ui32ReceivedDataCnt = 0;
                psSciTransfer->sTransferInfo.ui32TransferCnt = 0;
                psSciTransfer->sTransferInfo.ui32ExpectedDataCnt = 0;

                // Free the formerly allocated memory
                free(psSciTransfer->sTransferInfo.pui8UpstreamBuffer);

                psSciTransfer->sCallbacks.ReleaseProtocolCB();
            }
            break;
        
        default:
            break;

    }
    return true;
}

