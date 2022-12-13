/**************************************************************************//**
 * \file SCISlaveTranfer.c
 * \author Roman Holderried
 *
 * \brief Transfer evaluation for the SCI Slave module.
 *
 * <b> History </b>
 * 	- 2022-01-13 - File creation
 *  - 2022-03-17 - Port to C (Originally from SerialProtocol)
 *  - 2022-08-23 - V0.6.0: Upstream data gets not converted into ASCII data
 *  - 2022-12-11 - Adapted code for unified master/slave repo structure.
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "SCISlaveTransfer.h"
#include "SCITransferCommon.h"
#include "SCICommon.h"
#include "SCIVariables.h"
#include "Helpers.h"
#include "SCIconfig.h"


/******************************************************************************
 * Global variable definition
 *****************************************************************************/
extern const uint8_t ui8_byteLength[];

/******************************************************************************
 * Function definitions
 *****************************************************************************/
//=============================================================================
teSCI_SLAVE_ERROR SCIProcessRequest(tsSCI_TRANSFER_SLAVE *psTransfer, tsVAR_ACCESS *pVarAccess, tsREQUEST sReq, tsRESPONSE *psRsp)
{
    // RESPONSE rsp = RESPONSE_DEFAULT;
    teSCI_SLAVE_ERROR eError = eSCI_SLAVE_ERROR_NONE;

    // This is done outside of this method now
    // psRsp->i16Num         = sReq.i16Num;
    // psRsp->eReqType       = sReq.eReqType;

    switch (sReq.eReqType)
    {

        case eREQUEST_TYPE_GETVAR:
            {
                float f_val = 0.0;

                // If there is no readEEPROM callback or this is no EEPROM var, simply skip this step
                if (pVarAccess->pVarStruct[sReq.i16Num - 1].eVartype == eVARTYPE_EEPROM)
                {
                    // If conditions are met, EEPROM read must be successful.
                    eError = ReadEEPROMValueIntoVarStruct(pVarAccess, sReq.i16Num);
                    if (eError != eSCI_SLAVE_ERROR_NONE)
                        goto terminate;
                }

                eError = ReadValFromVarStruct(pVarAccess, sReq.i16Num, &f_val);
                if (eError != eSCI_SLAVE_ERROR_NONE)
                    goto terminate;
                
                psRsp->sTransferData.puRespVals[0].f_float = f_val;
                psRsp->eReqAck = eREQUEST_ACK_STATUS_SUCCESS;
                
                // We invalidate the response control because if there is a command ongoing, it has obviously been cancelled
                clearResponseControl(psTransfer);
            }
            break;

        case eREQUEST_TYPE_SETVAR:
            {
                float f_formerVal, newVal = 0.0;
                //bool writeSuccessful = false;

                eError = ReadValFromVarStruct(pVarAccess, sReq.i16Num, &f_formerVal);

                if (eError != eSCI_SLAVE_ERROR_NONE)
                    goto terminate;

                // Read back actual value and write new one (write will only happen if read was successful)
                eError = WriteValToVarStruct(pVarAccess, sReq.i16Num, sReq.uValArr[0].f_float);
                if (eError != eSCI_SLAVE_ERROR_NONE)
                    goto terminate;

                // If the varStruct write operation was successful, trigger an EEPROM write (if callback present and variable is of type eVARTYPE_EEPROM)
                if (pVarAccess->pVarStruct[sReq.i16Num - 1].eVartype == eVARTYPE_EEPROM)
                {
                    // If conditions are met, write must be successful.
                    eError = WriteEEPROMwithValueFromVarStruct(pVarAccess, sReq.i16Num);
                    if (eError != eSCI_SLAVE_ERROR_NONE)
                    {
                        // If the EEPROM write was not successful, write back the old value to the var struct to keep it in sync with the EEPROM.
                        WriteValToVarStruct(pVarAccess, sReq.i16Num, f_formerVal);
                        goto terminate;
                    }
                }

                // Everything was successful -> We call the action procedure
                if (pVarAccess->pVarStruct[sReq.i16Num - 1].ap != NULL)
                    pVarAccess->pVarStruct[sReq.i16Num - 1].ap();

                ReadValFromVarStruct(pVarAccess, sReq.i16Num, &newVal);

                // If everything happens to be allright, create the response
                psRsp->sTransferData.puRespVals[0].f_float = newVal;
                psRsp->eReqAck = eREQUEST_ACK_STATUS_SUCCESS;

                // We invalidate the response control because if there is a command ongoing, it has obviously been cancelled
                clearResponseControl(psTransfer);
            }
            break;
        
        case eREQUEST_TYPE_COMMAND:
            {
                teREQUEST_ACKNOWLEDGE eReqAck = eREQUEST_ACK_STATUS_UNKNOWN;
                tsTRANSFER_DATA sTransferData = tsTRANSFER_DATA_DEFAULTS;
                // Determine if a new command has been sent or if the ongoing command is to be processed
                bool bNewCmd = psTransfer->sResponseControl.ui8ControlBits.ongoing == false || (psTransfer->sResponseControl.sRsp.i16Num != sReq.i16Num);

                if (bNewCmd)
                {
                    // Check if a command structure has been passed
                    if (psTransfer->pCmdCBStruct != NULL && sReq.i16Num > 0 && sReq.i16Num <= SIZE_OF_CMD_STRUCT)
                    {
                        // TODO: Support for passing values to the command function
                        #ifdef VALUE_MODE_HEX
                        eReqAck = psTransfer->pCmdCBStruct[sReq.i16Num - 1](&sReq.uValArr[0].ui32_hex,sReq.ui8ValArrLen, &sTransferData);
                        #else
                        eReqAck = psTransfer->pCmdCBStruct[sReq.i16Num - 1](&sReq.uValArr[0].f_float,sReq.ui8ValArrLen, &sTransferData);
                        #endif
                    }
                    else
                    {
                        eError = eSCI_SLAVE_ERROR_REQUEST_UNKNOWN;
                        goto terminate;
                    }

                    // Response is getting sent independently of command success
                    
                    psRsp->eReqAck          = eReqAck;
                    psRsp->sTransferData    = sTransferData;

                    // Fill the response control struct
                    psTransfer->sResponseControl.ui8ControlBits.firstPacketNotSent  = true;
                    psTransfer->sResponseControl.ui32DataIdx                        = 0;
                    
                    // Set the control bits if a data transfer has been initiated
                    psTransfer->sResponseControl.ui8ControlBits.ongoing = 
                        ((psRsp->eReqAck == eREQUEST_ACK_STATUS_SUCCESS_DATA) && (psRsp->sTransferData.ui32DatLen > 0));
                    psTransfer->sResponseControl.ui8ControlBits.upstream = 
                        ((psRsp->eReqAck == eREQUEST_ACK_STATUS_SUCCESS_UPSTREAM) && (psRsp->sTransferData.ui32DatLen > 0));
                    
                    // Save the response for later
                    psTransfer->sResponseControl.sRsp = *psRsp;
                    
                }
                else
                {
                    psRsp->eReqAck          = psTransfer->sResponseControl.sRsp.eReqAck;
                    psRsp->sTransferData    = psTransfer->sResponseControl.sRsp.sTransferData;
                    psTransfer->sResponseControl.ui8ControlBits.firstPacketNotSent = false;
                }
            }
            break;
        
        case eREQUEST_TYPE_UPSTREAM:

            // Number must match with the previously sent command
            if (psTransfer->sResponseControl.sRsp.i16Num == sReq.i16Num && psTransfer->sResponseControl.ui8ControlBits.upstream == true)
            {
                psRsp->eReqAck   = eREQUEST_ACK_STATUS_SUCCESS;
                psRsp->sTransferData          = psTransfer->sResponseControl.sRsp.sTransferData;
                // Change the command type
                psTransfer->sResponseControl.sRsp.eReqType = psRsp->eReqType;
            }
            // If conditions are not met, provide the minimal information to construct a proper answer
            // TODO: Error handling
            else
            {
                eError = eSCI_SLAVE_ERROR_UPSTREAM_NOT_INITIATED;
                goto terminate;
            }
            break;

        default:
            // If COMMAND_TYPE_NONE, we don't kill ongoing data transmissions
            break;
    }

    terminate: return eError;
}



//=============================================================================
void clearResponseControl(tsSCI_TRANSFER_SLAVE *psTransfer)
{
    tsRESPONSECONTROL cleanObj = tsRESPONSECONTROL_DEFAULTS;

    // Free previously allocated memory
    if (psTransfer->sResponseControl.sRsp.sTransferData.ui8InfoFlagBits.dataBufDynamic == true)
        free(psTransfer->sResponseControl.sRsp.sTransferData.puRespVals);
    if (psTransfer->sResponseControl.sRsp.sTransferData.ui8InfoFlagBits.upstreamBufDynamic == true)
        free(psTransfer->sResponseControl.sRsp.sTransferData.pui8UpStreamBuf);

    // Clean the structure
    memcpy(&psTransfer->sResponseControl, &cleanObj, sizeof(tsRESPONSECONTROL));
}