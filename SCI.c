/**************************************************************************//**
 * \file SCI.c
 * \author Roman Holderried
 *
 * \brief Definitions for the SerialProtocol module.
 *
 * <b> History </b>
 * 	- 2022-01-13 - File creation
 *  - 2022-03-17 - Port to C (Originally from SerialProtocol)
 *****************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "SCI.h"
#include "SCICommands.h"
#include "SCIconfig.h"
#include "CommandStucture.h"
#include "Helpers.h"
#include "Variables.h"
#include "Buffer.h"

/******************************************************************************
 * Global variable definition
 *****************************************************************************/
static SCI sci = SCI_DEFAULT;
// Note: The idizes correspond to the values of the COMMAND_CB_STATUS enum values!
static const uint8_t responseDesignator [5][3] = {"ACK", "DAT", "UPS", "ERR", "NAK"};
static const uint8_t cmdIdArr[6] = {'#', '?', '!', ':', '>', '<'};
const uint8_t ui8_byteLength[7] = {1,1,2,2,4,4,4};

/******************************************************************************
 * Function definitions
 *****************************************************************************/

//=============================================================================
tSCI_ERROR SCI_init(SCI_CALLBACKS callbacks, VAR *p_varStruct, COMMAND_CB *p_cmdStruct)
{
    // Initialize the callbacks
    sci.varAccess.readEEPROM_cb                 = callbacks.readEEPROMCallback;
    sci.varAccess.writeEEPROM_cb                = callbacks.writeEEPROMCallback;
    sci.datalink.txBlockingCallback             = callbacks.transmitBlockingCallback;
    sci.datalink.txNonBlockingCallback          = callbacks.transmitNonBlockingCallback;
    sci.datalink.txGetBusyStateCallback         = callbacks.getTxBusyStateCallback;

    // Hand over the pointers to the var and cmd structs
    sci.varAccess.p_varStruct       = p_varStruct;
    sci.sciCommands.p_cmdCBStruct   = p_cmdStruct;

    // Configure data structures
    fifoBufInit(&sci.rxFIFO, sci.rxBuffer, RX_PACKET_LENGTH);
    fifoBufInit(&sci.txFIFO, sci.txBuffer, TX_PACKET_LENGTH);

    // Initialize the variable structure
    return (initVarstruct(&sci.varAccess));
}

//=============================================================================
void SCI_receiveData(uint8_t ui8_data)
{
    // Call the lower level datalink level functionality
    receive(&sci.datalink, &sci.rxFIFO, ui8_data);
}


//=============================================================================
void SCI_statemachine(void)
{
    // Check the lower level datalink states and set the protocol state accordingly
    if (sci.e_state > ePROTOCOL_ERROR)
    {
        if (sci.e_state < ePROTOCOL_EVALUATING)
            sci.e_state = (PROTOCOL_STATE)getDatalinkReceiveState(&sci.datalink);
        // else
        //     sci.e_state = sci.datalink.tState;
    }

    switch(sci.e_state)
    {
        case ePROTOCOL_IDLE:
            break;
        case ePROTOCOL_RECEIVING:
            break;
        case ePROTOCOL_EVALUATING:
            {
                uint8_t *   pui8_buf;
                COMMAND     cmd = COMMAND_DEFAULT;
                RESPONSE    rsp = RESPONSE_DEFAULT; 
                uint8_t     ui8_msgSize = readBuf(&sci.rxFIFO, &pui8_buf);
                tSCI_ERROR  eError = eSCI_ERROR_NONE;

                // Clear Datalink State
                acknowledgeRx(&sci.datalink);

                // Parse the command (skip STX and don't care for ETX)
                eError = commandParser(pui8_buf, ui8_msgSize, &cmd);

                rsp = executeCmd(&sci.sciCommands, &sci.varAccess, cmd);

                // TODO: Error behaviour if there is no valid response
                // Currently, the arduino won't send any response

                flushBuf(&sci.txFIFO);
                
                // "Put" the date into the tx buffer
                pui8_buf = sci.txBuffer;
                increaseBufIdx(&sci.txFIFO, responseBuilder(pui8_buf, rsp));
                // TODO: Error handling -> Message too long

                if (transmit(&sci.datalink, &sci.txFIFO))
                    sci.e_state = ePROTOCOL_SENDING;
                // TODO: Error handling?
                else 
                    sci.e_state = ePROTOCOL_IDLE;  
            }
    
            break;

        case ePROTOCOL_SENDING:

            transmitStateMachine(&sci.datalink);
            
            if (getDatalinkTransmitState(&sci.datalink) == eDATALINK_TSTATE_READY)
            {
                acknowledgeTx(&sci.datalink);
                sci.e_state = ePROTOCOL_IDLE;
            }
            
            break;
        
        default:
            break;
    }
}

//=============================================================================
bool SCI_GetVarFromStruct(int16_t i16_varNum, VAR** p_Var)
{
    return getVarPtr(&sci.varAccess, p_Var, i16_varNum);
}

//=============================================================================
tSCI_ERROR commandParser(uint8_t* pui8_buf, uint8_t ui8_stringSize, COMMAND *pCmd)
{
    uint8_t i = 0;
    uint32_t ui32_tmp;
    // uint8_t cmdIdx  = 0;
    // COMMAND cmd     = COMMAND_DEFAULT;

    for (; i < ui8_stringSize; i++)
    {

        if (pui8_buf[i] == GETVAR_IDENTIFIER)
        {
            pCmd->e_cmdType = eCOMMAND_TYPE_GETVAR;
            break;
        }
        else if (pui8_buf[i] == SETVAR_IDENTIFIER)
        {
            pCmd->e_cmdType = eCOMMAND_TYPE_SETVAR;
            break;
        }
        else if (pui8_buf[i] == COMMAND_IDENTIFIER)
        {
            pCmd->e_cmdType = eCOMMAND_TYPE_COMMAND;
            break;
        }
        else if (pui8_buf[i] == UPSTREAM_IDENTIFIER)
        {
            pCmd->e_cmdType = eCOMMAND_TYPE_UPSTREAM;
            break;
        }
        else if (pui8_buf[i] == DOWNSTREAM_IDENTIFIER)
        {
            pCmd->e_cmdType = eCOMMAND_TYPE_DOWNSTREAM;
            break;
        }
        else
            return eSCI_ERROR_COMMAND_IDENTIFIER_NOT_FOUND;   
    }

    // No valid command identifier found (TODO: Error handling)
    // if (pCmd->e_cmdType == eCOMMAND_TYPE_NONE)
    //     goto terminate;
    
    /*******************************************************************************************
     * Variable number conversion
    *******************************************************************************************/
    // Loop breaks when i reflects the buffer position of the command identifier
    // Variable number conversion
    {
        // One additional character necessary for string termination
        uint8_t *p_numStr = (uint8_t*)malloc(i+1);

        // copy the number string into new array
        memcpy(p_numStr,pui8_buf,i);
        // Properly terminate string to use the atoi buildin
        p_numStr[i] = '\0';
        // Convert
        #ifdef VALUE_MODE_HEX
        if(!strToHex(p_numStr, &ui32_tmp))
           return eSCI_ERROR_VARIABLE_NUMBER_CONVERSION_FAILED; 
        pCmd->i16_num = *(int16_t*)(&ui32_tmp);
        #else
        pCmd->i16_num = (int16_t)(atoi((char*)p_numStr));
        #endif

        free(p_numStr);
    }

    /************************************s*******************************************************
     * Variable value conversion
    *******************************************************************************************/
   // Only if a parameter has been passed
   if (ui8_stringSize > i + 1)
   {
        uint8_t ui8_valStrLen = ui8_stringSize - i;
        uint8_t j = 1;
        uint8_t ui8_numOfVals = 0;
        uint8_t ui8_valueLen = 0;
        uint8_t *p_valStr = NULL;

        while (ui8_numOfVals <= MAX_NUM_COMMAND_VALUES)
        {
            ui8_numOfVals++;

            while (j < ui8_valStrLen)
            {
                // Value seperator found
                if (pui8_buf[i + j] == ',')
                    break;
                
                j++;
                ui8_valueLen++;
            }

            p_valStr = (uint8_t*)malloc(ui8_valueLen + 1);

            // copy the number string into new array
            memcpy(p_valStr, &pui8_buf[i + j - ui8_valueLen], ui8_valueLen);

            p_valStr[ui8_valueLen] = '\0';

            #ifdef VALUE_MODE_HEX
            if(!strToHex(p_valStr, &pCmd->valArr[ui8_numOfVals - 1].ui32_hex))
                return eSCI_ERROR_COMMAND_VALUE_CONVERSION_FAILED;
            #else
            pCmd->valArr[ui8_numOfVals - 1].f_float = atof((char*)p_valStr);
            #endif

            free(p_valStr);

            if (j == ui8_valStrLen)
                break;
            
            ui8_valueLen = 0;
            j++;
        }
        pCmd->ui8_valArrLen = ui8_numOfVals;
    }

    return eSCI_ERROR_NONE;
}

//=============================================================================
uint8_t responseBuilder(uint8_t *pui8_buf, RESPONSE rsp)
{
    uint8_t ui8_size    = 0;

    // Convert variable number to ASCII
    #ifdef VALUE_MODE_HEX
    ui8_size = (uint8_t)hexToStrWord(pui8_buf, &rsp.i16_num, true);
    #else
    ui8_size = ftoa(pui8_buf, (float)rsp.i16_num, true);
    #endif

    // Increase Buffer index and write command type identifier
    pui8_buf += ui8_size;
    *pui8_buf++ = cmdIdArr[rsp.e_cmdType];
    ui8_size++;

    if (rsp.b_valid)
    {
        switch (rsp.e_cmdType)
        {
            case eCOMMAND_TYPE_GETVAR:
                // Fill the response designator
                memcpy(pui8_buf, &responseDesignator[(uint8_t)eCOMMAND_STATUS_SUCCESS], 3);
                pui8_buf+=3;
                *pui8_buf++ = ';';
                ui8_size += 4;
                // Write the data value into the buffer
                #ifdef VALUE_MODE_HEX
                ui8_size += (uint8_t)hexToStrDword(pui8_buf, &rsp.val.ui32_hex, true);
                #else
                ui8_size += ftoa(pui8_buf, rsp.val.f_float, true);
                #endif
                break;
            
            case eCOMMAND_TYPE_SETVAR:
                // If we got here, the operation was successful
                memcpy(pui8_buf, &responseDesignator[(uint8_t)eCOMMAND_STATUS_SUCCESS], 3);
                pui8_buf+=3;
                ui8_size += 3;
                break;

            case eCOMMAND_TYPE_COMMAND:
                // No response designator on every consecutive packet
                if (sci.sciCommands.responseControl.ui8_controlBits.firstPacketNotSent)
                {
                    memcpy(pui8_buf, &responseDesignator[(uint8_t)rsp.e_cmdStatus], 3);
                    pui8_buf+=3;
                    ui8_size += 3;

                    if (rsp.e_cmdStatus == eCOMMAND_STATUS_SUCCESS_DATA ||rsp.e_cmdStatus == eCOMMAND_STATUS_SUCCESS_UPSTREAM)
                    {
                        uint8_t ui8_asciiSize;

                        *pui8_buf++ = ';';
                        ui8_size++;
                        #ifdef VALUE_MODE_HEX
                        ui8_asciiSize = (uint8_t)hexToStrDword(pui8_buf, &rsp.info.ui32_datLen, true);
                        #else
                        ui8_asciiSize = ftoa(pui8_buf, (float)rsp.info.ui32_datLen, true);
                        #endif
                        pui8_buf += ui8_asciiSize;
                        ui8_size += ui8_asciiSize;
                    }
                }

                // Fill the rest of the packet with data
                if (sci.sciCommands.responseControl.ui8_controlBits.ongoing)
                {
                    if(sci.sciCommands.responseControl.ui8_controlBits.firstPacketNotSent)
                    {
                        *pui8_buf++ = ';';
                        ui8_size++;
                    }
                    ui8_size += fillBufferWithValues(&sci.sciCommands, pui8_buf, TX_PACKET_LENGTH - ui8_size);
                }

                break;
            
            case eCOMMAND_TYPE_UPSTREAM:
                ui8_size += fillBufferWithValues(&sci.sciCommands, pui8_buf, TX_PACKET_LENGTH - ui8_size);
                break;

            default:
                break;
        }
    }
    else
    {
        // We get here for example if there was no valid command identifier found
        if (rsp.info.ui16_error == 0)
        {
            memcpy(pui8_buf, &responseDesignator[(uint8_t)eCOMMAND_STATUS_UNKNOWN], 3);
            pui8_buf += 3;
        }
        else
        {
            memcpy(pui8_buf, &responseDesignator[(uint8_t)eCOMMAND_STATUS_ERROR], 3);
            pui8_buf+=3;
            *pui8_buf++ = ';';
            ui8_size ++;
            // Write the data value into the buffer
            #ifdef VALUE_MODE_HEX
            ui8_size += (uint8_t)hexToStrWord(pui8_buf, &rsp.info.ui16_error, true);
            #else
            ui8_size += ftoa(pui8_buf, (float)rsp.info.ui16_error, true);
            #endif
        }

        ui8_size += 3;
    }

    return ui8_size;
}