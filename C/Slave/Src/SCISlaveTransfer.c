/**************************************************************************//**
 * \file SCISlaveTranfer.c
 * \author Roman Holderried
 *
 * \brief Definitions for the SCI commands module.
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
#include "Variables.h"
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
tSCI_ERROR executeCmd(SCI_COMMANDS *p_inst, VAR_ACCESS *p_varAccess, COMMAND cmd, RESPONSE *pRsp)
{
    // RESPONSE rsp = RESPONSE_DEFAULT;
    tSCI_ERROR eError = eSCI_ERROR_NONE;

    // This is done outside of this method now
    // pRsp->i16_num         = cmd.i16_num;
    // pRsp->e_cmdType       = cmd.e_cmdType;

    switch (cmd.e_cmdType)
    {

        case eCOMMAND_TYPE_GETVAR:
            {
                float f_val = 0.0;

                // If there is no readEEPROM callback or this is no EEPROM var, simply skip this step
                if (p_varAccess->p_varStruct[cmd.i16_num - 1].vartype == eVARTYPE_EEPROM)
                {
                    // If conditions are met, EEPROM read must be successful.
                    eError = readEEPROMValueIntoVarStruct(p_varAccess, cmd.i16_num);
                    if (eError != eSCI_ERROR_NONE)
                        goto terminate;
                }

                eError = readValFromVarStruct(p_varAccess, cmd.i16_num, &f_val);
                if (eError != eSCI_ERROR_NONE)
                    goto terminate;
                
                pRsp->val.f_float = f_val;
                pRsp->e_cmdStatus = eCOMMAND_STATUS_SUCCESS;
                
                // We invalidate the response control because if there is a command ongoing, it has obviously been cancelled
                clearResponseControl(p_inst);
            }
            break;

        case eCOMMAND_TYPE_SETVAR:
            {
                float f_formerVal, newVal = 0.0;
                //bool writeSuccessful = false;

                eError = readValFromVarStruct(p_varAccess, cmd.i16_num, &f_formerVal);

                if (eError != eSCI_ERROR_NONE)
                    goto terminate;

                // Read back actual value and write new one (write will only happen if read was successful)
                eError = writeValToVarStruct(p_varAccess, cmd.i16_num, cmd.valArr[0].f_float);
                if (eError != eSCI_ERROR_NONE)
                    goto terminate;

                // If the varStruct write operation was successful, trigger an EEPROM write (if callback present and variable is of type eVARTYPE_EEPROM)
                if (p_varAccess->p_varStruct[cmd.i16_num - 1].vartype == eVARTYPE_EEPROM)
                {
                    // If conditions are met, write must be successful.
                    eError = writeEEPROMwithValueFromVarStruct(p_varAccess, cmd.i16_num);
                    if (eError != eSCI_ERROR_NONE)
                    {
                        // If the EEPROM write was not successful, write back the old value to the var struct to keep it in sync with the EEPROM.
                        writeValToVarStruct(p_varAccess, cmd.i16_num, f_formerVal);
                        goto terminate;
                    }
                }

                // Everything was successful -> We call the action procedure
                if (p_varAccess->p_varStruct[cmd.i16_num - 1].ap != NULL)
                    p_varAccess->p_varStruct[cmd.i16_num - 1].ap();

                readValFromVarStruct(p_varAccess, cmd.i16_num, &newVal);

                // If everything happens to be allright, create the response
                pRsp->val.f_float = newVal;
                pRsp->e_cmdStatus = eCOMMAND_STATUS_SUCCESS;

                // We invalidate the response control because if there is a command ongoing, it has obviously been cancelled
                clearResponseControl(p_inst);
            }
            break;
        
        case eCOMMAND_TYPE_COMMAND:
            {
                COMMAND_CB_STATUS cmdStatus = eCOMMAND_STATUS_UNKNOWN;
                PROCESS_INFO info = PROCESS_INFO_DEFAULT;
                // Determine if a new command has been sent or if the ongoing command is to be processed
                bool b_newCmd = p_inst->responseControl.ui8_controlBits.ongoing == false || (p_inst->responseControl.rsp.i16_num != cmd.i16_num);

                if (b_newCmd)
                {
                    // Check if a command structure has been passed
                    if (p_inst->p_cmdCBStruct != NULL && cmd.i16_num > 0 && cmd.i16_num <= SIZE_OF_CMD_STRUCT)
                    {
                        // TODO: Support for passing values to the command function
                        #ifdef VALUE_MODE_HEX
                        cmdStatus = p_inst->p_cmdCBStruct[cmd.i16_num - 1](&cmd.valArr[0].ui32_hex,cmd.ui8_valArrLen, &info);
                        #else
                        cmdStatus = p_inst->p_cmdCBStruct[cmd.i16_num - 1](&cmd.valArr[0].f_float,cmd.ui8_valArrLen, &info);
                        #endif
                    }
                    else
                    {
                        eError = eSCI_ERROR_COMMAND_UNKNOWN;
                        goto terminate;
                    }

                    // Response is getting sent independently of command success
                    
                    pRsp->e_cmdStatus     = cmdStatus;
                    pRsp->info            = info;

                    // Fill the response control struct
                    p_inst->responseControl.ui8_controlBits.firstPacketNotSent  = true;
                    p_inst->responseControl.ui32_dataIdx                        = 0;
                    
                    // Set the control bits if a data transfer has been initiated
                    p_inst->responseControl.ui8_controlBits.ongoing = 
                        ((pRsp->e_cmdStatus == eCOMMAND_STATUS_SUCCESS_DATA) && (pRsp->info.ui32_datLen > 0));
                    p_inst->responseControl.ui8_controlBits.upstream = 
                        ((pRsp->e_cmdStatus == eCOMMAND_STATUS_SUCCESS_UPSTREAM) && (pRsp->info.ui32_datLen > 0));
                    
                    // Save the response for later
                    p_inst->responseControl.rsp = *pRsp;
                    
                }
                else
                {
                    pRsp->e_cmdStatus     = p_inst->responseControl.rsp.e_cmdStatus;
                    pRsp->info            = p_inst->responseControl.rsp.info;
                    p_inst->responseControl.ui8_controlBits.firstPacketNotSent = false;
                }
            }
            break;
        
        case eCOMMAND_TYPE_UPSTREAM:

            // Number must match with the previously sent command
            if (p_inst->responseControl.rsp.i16_num == cmd.i16_num && p_inst->responseControl.ui8_controlBits.upstream == true)
            {
                pRsp->e_cmdStatus   = eCOMMAND_STATUS_SUCCESS;
                pRsp->info          = p_inst->responseControl.rsp.info;
                // Change the command type
                p_inst->responseControl.rsp.e_cmdType = pRsp->e_cmdType;
            }
            // If conditions are not met, provide the minimal information to construct a proper answer
            // TODO: Error handling
            else
            {
                eError = eSCI_ERROR_UPSTREAM_NOT_INITIATED;
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
uint8_t fillBufferWithValues(SCI_COMMANDS *p_inst, uint8_t * p_buf, uint8_t ui8_maxSize)
{
    uint8_t ui8_currentDataSize = 0;

    if (p_inst->responseControl.rsp.e_cmdType == eCOMMAND_TYPE_UPSTREAM)
    {
        // Check if there is a valid buffer pointer passed
        if (p_inst->responseControl.rsp.info.pui8_upStreamBuf == NULL)
            return 0;

        // Upstream data does not get converted into an ASCII-stream
        // Determine the actual packet length
        ui8_maxSize = p_inst->responseControl.rsp.info.ui32_datLen < ui8_maxSize ? p_inst->responseControl.rsp.info.ui32_datLen : ui8_maxSize;
        memcpy(p_buf, &p_inst->responseControl.rsp.info.pui8_upStreamBuf[p_inst->responseControl.ui32_dataIdx], ui8_maxSize);

        p_inst->responseControl.rsp.info.ui32_datLen -= ui8_maxSize;
        p_inst->responseControl.ui32_dataIdx += ui8_maxSize;
        ui8_currentDataSize += ui8_maxSize;

        // while (ui8_maxSize > (ui8_currentDataSize + 1) && p_inst->responseControl.rsp.info.ui32_datLen > 0)
        // {
        //     // ui8_currentDataSize += hexToStr(p_buf,(uint32_t*)&p_inst->responseControl.rsp.info.pui8_buf[p_inst->responseControl.ui32_byteIdx],2,false);
        //     ui8_currentDataSize += hexToStrByte(p_buf,&p_inst->responseControl.rsp.info.pui8_upStreamBuf[p_inst->responseControl.ui32_dataIdx], false);

        //     p_inst->responseControl.rsp.info.ui32_datLen--;
        //     p_inst->responseControl.ui32_dataIdx++;
        //     p_buf += 2;
        // }
    }
    else if (p_inst->responseControl.rsp.e_cmdType == eCOMMAND_TYPE_COMMAND)
    {
        bool    b_commaSet = false;
        uint8_t ui8_asciiSize;
        uint8_t ui8_datBuf[20];

        #ifndef VALUE_MODE_HEX
        float   f_passVal;
        #endif

        // Check if there is a valid data format table pointer passed
        if (p_inst->responseControl.rsp.info.pui32_dataBuf == NULL)
            return 0;

        while (true)
        {
            // All data is handled
            if (p_inst->responseControl.rsp.info.ui32_datLen == 0)
            {
                if (b_commaSet)
                {
                    ui8_currentDataSize--;
                    p_buf--;
                }
                break;
            }

            ui8_asciiSize = (uint8_t)hexToStrDword(ui8_datBuf, &p_inst->responseControl.rsp.info.pui32_dataBuf[p_inst->responseControl.ui32_dataIdx], true);

            // Fits the value in the buffer?
            if ((ui8_currentDataSize + ui8_asciiSize) < ui8_maxSize)
            {
                // Copy the value in the send buffer
                memcpy(p_buf, ui8_datBuf, ui8_asciiSize);
                ui8_currentDataSize += ui8_asciiSize;

                // Handle all indices
                p_inst->responseControl.rsp.info.ui32_datLen--;
                p_inst->responseControl.ui32_dataIdx++;
                p_buf += ui8_asciiSize;

                if (ui8_maxSize > ui8_currentDataSize)
                {
                    *p_buf++ = ',';
                    ui8_currentDataSize++;
                    b_commaSet = true;
                }
                else
                    break;
            }
            else
            {
                if (b_commaSet)
                {
                    ui8_currentDataSize--;
                    p_buf--;
                }
                break;
            }
        }
    }

    // Reset the ongoing flag when no data is left to transmit
    if (p_inst->responseControl.rsp.info.ui32_datLen == 0)
        clearResponseControl(p_inst);

    return ui8_currentDataSize;
}

//=============================================================================
void clearResponseControl(SCI_COMMANDS *p_inst)
{
    RESPONSECONTROL cleanObj = RESPONSECONTROL_DEFAULT;

    // Free previously allocated memory
    if (p_inst->responseControl.rsp.info.ui8_infoFlagBits.dataBufDynamic == true)
        free(p_inst->responseControl.rsp.info.pui32_dataBuf);
    if (p_inst->responseControl.rsp.info.ui8_infoFlagBits.upstreamBufDynamic == true)
        free(p_inst->responseControl.rsp.info.pui8_upStreamBuf);

    // Clean the structure
    memcpy(&p_inst->responseControl, &cleanObj, sizeof(RESPONSECONTROL));
}