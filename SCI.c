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
static const uint8_t responseDesignator [5][3] = {"ACK", "DAB", "DAV", "ERR", "NAK"};
static const uint8_t cmdIdArr[4] = {'#', '?', '!', ':'};

/******************************************************************************
 * Function definitions
 *****************************************************************************/

//=============================================================================
bool SCI_init(SCI_CALLBACKS callbacks, VAR *p_varStruct, COMMAND_CB *p_cmdStruct)
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
                COMMAND     cmd;
                RESPONSE    rsp; 
                uint8_t     ui8_msgSize = readBuf(&sci.rxFIFO, &pui8_buf);

                // Clear Datalink State
                acknowledgeRx(&sci.datalink);

                // Parse the command (skip STX and don't care for ETX)
                cmd = commandParser(pui8_buf, ui8_msgSize);

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
COMMAND commandParser(uint8_t* pui8_buf, uint8_t ui8_stringSize)
{
    uint8_t i = 0;
    uint32_t ui32_tmp;
    // uint8_t cmdIdx  = 0;
    COMMAND cmd     = COMMAND_DEFAULT;

    for (; i < ui8_stringSize; i++)
    {

        if (pui8_buf[i] == GETVAR_IDENTIFIER)
        {
            cmd.e_cmdType = eCOMMAND_TYPE_GETVAR;
            break;
        }
        else if (pui8_buf[i] == SETVAR_IDENTIFIER)
        {
            cmd.e_cmdType = eCOMMAND_TYPE_SETVAR;
            break;
        }
        else if (pui8_buf[i] == COMMAND_IDENTIFIER)
        {
            cmd.e_cmdType = eCOMMAND_TYPE_COMMAND;
            break;
        }
        
        // TODO: Error handling when no command identifier has been received
    }

    // No valid command identifier found (TODO: Error handling)
    if (cmd.e_cmdType == eCOMMAND_TYPE_NONE)
        goto terminate;
    
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
            ; // TODO: Error handling
        cmd.i16_num = *(int16_t*)(&ui32_tmp);
        #else
        cmd.i16_num = (int16_t)(atoi((char*)p_numStr));
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
            if(!strToHex(p_valStr, &cmd.valArr[ui8_numOfVals - 1].ui32_hex))
                ; // TODO: Error handling
            #else
            cmd.valArr[ui8_numOfVals - 1].f_float = atof((char*)p_valStr);
            #endif

            free(p_valStr);

            if (j == ui8_valStrLen)
                break;
            
            ui8_valueLen = 0;
            j++;
        }
        cmd.ui8_valArrLen = ui8_numOfVals;
    }

    terminate: return cmd;
}

//=============================================================================
uint8_t responseBuilder(uint8_t *pui8_buf, RESPONSE response)
{
    uint8_t ui8_size    = 0;
    

    // Convert variable number to ASCII
    #ifdef VALUE_MODE_HEX
    ui8_size = (uint8_t)hexToStr(pui8_buf, (uint32_t*)&response.i16_num, 4, true);
    #else
    ui8_size = ftoa(pui8_buf, (float)response.i16_num, true);
    #endif

    // Increase Buffer index and write command type identifier
    pui8_buf += ui8_size;
    *pui8_buf++ = cmdIdArr[response.e_cmdType];
    ui8_size++;

    if (response.b_valid)
    {
        switch (response.e_cmdType)
        {
            case eCOMMAND_TYPE_GETVAR:
                // Write the data value into the buffer
                #ifdef VALUE_MODE_HEX
                ui8_size += (uint8_t)hexToStr(pui8_buf, &response.val.ui32_hex, 8, true);
                #else
                ui8_size += ftoa(pui8_buf, response.val.f_float, true);
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
                if (sci.sciCommands.responseControl.b_firstPacketNotSent)
                {
                    memcpy(pui8_buf, &responseDesignator[(uint8_t)response.e_cmdStatus], 3);
                    pui8_buf+=3;
                    ui8_size += 3;
                }
                
                if (response.e_cmdStatus == eCOMMAND_STATUS_DATA_BYTES ||response.e_cmdStatus == eCOMMAND_STATUS_DATA_VALUES)
                {
                    // No datalength on every consecutive packet
                    if (sci.sciCommands.responseControl.b_firstPacketNotSent)
                    {
                        uint8_t ui8_asciiSize;

                        *pui8_buf++ = ';';
                        ui8_size++;
                        #ifdef VALUE_MODE_HEX
                        ui8_asciiSize = (uint8_t)hexToStr(pui8_buf, &response.info.ui32_datLen, 8, true);
                        #else
                        ui8_asciiSize = ftoa(pui8_buf, (float)response.info.ui32_datLen, true);
                        #endif
                        pui8_buf += ui8_asciiSize;
                        ui8_size += ui8_asciiSize;
                        *pui8_buf++ = ';';
                        ui8_size++;
                    }
                    
                    // Fill the rest of the packet with data
                    if (sci.sciCommands.responseControl.b_ongoing)
                        ui8_size += fillBufferWithValues(&sci.sciCommands, pui8_buf, TX_PACKET_LENGTH - ui8_size);
                }
                break;

            default:
                break;
        }
    }
    else
    {
        // We get here for example if there was no valid command identifier found
        if (response.info.ui16_error == 0)
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
            ui8_size += (uint8_t)hexToStr(pui8_buf, (uint32_t*)&response.info.ui16_error, 4, true);
            #else
            ui8_size += ftoa(pui8_buf, (float)response.info.ui16_error, true);
            #endif
        }

        ui8_size += 3;
    }


    return ui8_size;
}