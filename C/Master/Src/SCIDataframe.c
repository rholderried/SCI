/**************************************************************************//**
 * \file SCIDataframe.c
 * \author Roman Holderried
 *
 * \brief Dataframe parsing functionality of the SCI protocol.
 *
 * <b> History </b>
 * 	- 2022-11-21 - File creation -
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "SCICommon.h"
#include "SCIDataframe.h"
#include "SCITransfer.h"
#include "Helpers.h"

/******************************************************************************
 * Global variable definition
 *****************************************************************************/
// Note: The idizes correspond to the values of the C enum values!
static const char acknowledgeArr [5][4] = {"ACK", "DAT", "UPS", "ERR", "NAK"};
static const uint8_t cmdIdArr[6] = {'#', '?', '!', ':', '>', '<'};
// const uint8_t ui8_byteLength[7] = {1,1,2,2,4,4,4};

/******************************************************************************
 * Function declarations
 *****************************************************************************/

teSCI_ERROR SCIMasterRequestBuilder(uint8_t *pui8Buf, uint8_t *pui8Size, tsREQUEST sReq)
{
    uint8_t ui8AsciiSize;
    uint8_t ui8DatBuf[30]   = {0};
    uint8_t ui8DataCnt      = 0;
    bool bCommaSet = false;

    // Convert variable number to ASCII
    #ifdef VALUE_MODE_HEX
    *pui8Size = (uint8_t)hexToStrWord(pui8Buf, (uint16_t*)&sReq.i16Num, true);
    #else
    *pui8Size = ftoa(pui8Buf, (float)sReq.i16Num, true);
    #endif

    // Increase Buffer index and write request type identifier
    pui8Buf += *pui8Size;
    *pui8Buf++ = cmdIdArr[sReq.eReqType];
    (*pui8Size)++;

    for(uint8_t i = 0; i < sReq.ui8ValArrLen; i++)
    {
        if (i >= MAX_NUM_REQUEST_VALUES)
            break;

        #ifdef VALUE_MODE_HEX
        ui8AsciiSize = (uint8_t)hexToStrDword(ui8DatBuf, &sReq.uValArr[i].ui32_hex, true);
        #else
        ui8AsciiSize = ftoa(ui8DatBuf, sReq.uValArr[i].f_float, true);
        #endif

        if((*pui8Size + ui8AsciiSize) < TX_PACKET_LENGTH)
        {
            memcpy(pui8Buf, ui8DatBuf, ui8AsciiSize);
            pui8Buf += ui8AsciiSize;
            (*pui8Size) += ui8AsciiSize;
            ui8DataCnt++;

            if (ui8DataCnt < sReq.ui8ValArrLen)
            {
                *pui8Buf++ = ',';
                (*pui8Size)++;
            }
            else
                break;
        }            
        
        else
        {
            // Ignore the last comma
            (*pui8Size)--;

            return eSCI_ERROR_MESSAGE_EXCEEDS_TX_BUFFER_SIZE;
        }

    }

    return eSCI_ERROR_NONE;
}

//=============================================================================
teSCI_ERROR SCIMasterResponseParser(uint8_t* pui8Buf, uint8_t ui8DataframeLen, tsRESPONSE *psRsp)
{
    uint8_t i = 0;
    bool bAckPresent = false;
    int8_t i8Ack;
    int16_t i16BytesToGo = (int16_t)ui8DataframeLen;
    psRsp->pui8Raw = pui8Buf;
    
    // uint8_t cmdIdx  = 0;
    // COMMAND cmd     = COMMAND_DEFAULT;

    for (; i < i16BytesToGo; i++)
    {

        if (pui8Buf[i] == GETVAR_IDENTIFIER)
        {
            psRsp->eReqType = eREQUEST_TYPE_GETVAR;
            break;
        }
        else if (pui8Buf[i] == SETVAR_IDENTIFIER)
        {
            psRsp->eReqType = eREQUEST_TYPE_SETVAR;
            break;
        }
        else if (pui8Buf[i] == COMMAND_IDENTIFIER)
        {
            psRsp->eReqType = eREQUEST_TYPE_COMMAND;
            break;
        }
        else if (pui8Buf[i] == UPSTREAM_IDENTIFIER)
        {
            psRsp->eReqType = eREQUEST_TYPE_UPSTREAM;
            break;
        }
        else if (pui8Buf[i] == DOWNSTREAM_IDENTIFIER)
        {
            psRsp->eReqType = eREQUEST_TYPE_DOWNSTREAM;
            break;
        }      
    }

    // No valid command identifier found (TODO: Error handling)
    if (psRsp->eReqType == eREQUEST_TYPE_NONE)
        return eSCI_ERROR_COMMAND_IDENTIFIER_NOT_FOUND;
    
    /*******************************************************************************************
     * Command Number Conversion
    *******************************************************************************************/
    // Loop breaks when i reflects the buffer position of the command identifier
    // Variable number conversion
    {
        uint32_t ui32_tmp;
        // One additional character necessary for string termination
        uint8_t *pui8NumStr = (uint8_t*)malloc(i+1);

        // copy the number string into new array
        memcpy(pui8NumStr,pui8Buf,i);
        // Properly terminate string to use the atoi buildin
        pui8NumStr[i] = '\0';
        // Convert
        #ifdef VALUE_MODE_HEX
        if(!strToHex(pui8NumStr, &ui32_tmp))
           return eSCI_ERROR_NUMBER_CONVERSION_FAILED; 
        psRsp->i16Num = *(int16_t*)(&ui32_tmp);
        #else
        psRsp->i16Num = (int16_t)(atoi((char*)pui8NumStr));
        #endif

        free(pui8NumStr);
    }

    // let i correspond to the position of the char after the ID
    i++;
    i16BytesToGo -= i;

    /*******************************************************************************************
     * Find the command acknowledge
    *******************************************************************************************/
    // UPSTREAM message has no acknowledge, just data and is not going to be processed by this 
    // function
    i8Ack = _CheckAcknowledge(&pui8Buf[i], i16BytesToGo) ;

    if (i8Ack >= 0)
    {
        psRsp->eReqAck = (teREQUEST_ACKNOWLEDGE)i8Ack;
        // For i: Take care of the ';'
        i += 4;
        i16BytesToGo -= 4;
    }

    // Message could be complete here (COMMAND without results)
    if (i16BytesToGo <= 0)
        return eSCI_ERROR_NONE;

    // Get the control number after the acknowledge (Which can only happen if there is an acknowledge in the message)
    if (i8Ack >= 0)
    {
        uint8_t j = 0;
        tuREQUESTVALUE uNum = {.ui32_hex = 0};
        uint8_t *pui8NumStr;

        while (j < i16BytesToGo)
        {
            if (pui8Buf[j + i] == ';')
                break;

            j++;
        }

        pui8NumStr = (uint8_t*)malloc(j+1);
        memcpy(pui8NumStr,&pui8Buf[i],j);
        pui8NumStr[j] = '\0';

        #ifdef VALUE_MODE_HEX
        if(!strToHex(pui8NumStr, &uNum.ui32_hex))
            return eSCI_ERROR_PARAMETER_CONVERSION_FAILED; 
        #else
            uNum.f_float = atof((char*)pui8NumStr);
        #endif

        free(pui8NumStr);

        // Assign the number to the data field
        
        switch (psRsp->eReqAck)
        {
            case eREQUEST_ACK_STATUS_SUCCESS_DATA:
            case eREQUEST_ACK_STATUS_SUCCESS_UPSTREAM:
                #ifdef VALUE_MODE_HEX
                psRsp->ui32DataLength = uNum.ui32_hex;
                #else
                psRsp->ui32DataLength = uNum.f_float;
                #endif
                break;

            case eREQUEST_ACK_STATUS_ERROR:
                #ifdef VALUE_MODE_HEX
                psRsp->ui16ErrNum = uNum.ui32_hex;
                #else
                psRsp->ui16ErrNum = uNum.f_float;
                #endif
                break;

            default:
                // Save GetVar result
                if (psRsp->eReqType == eREQUEST_TYPE_GETVAR)
                {
                    psRsp->uValArr[0] = uNum;
                }
                // Don't handle eREQUEST_ACK_STATUS_SUCCESS of a COMMAND here, because all
                // data afterwards is to be threated as return values.
                break;
        }

        //let i correspond to the position of the char after the first data number
        i += (j + 1);
        i16BytesToGo -= (j + 1);
    }
    // If we get into this else, that means we are dealing with a consecutive Command Data message,
    // which has no acknowledge, only data
    else
    {
        // We need to set this field here. Otherwise, the SCITransferControl function does not
        // know what to do with this message
        psRsp->eReqAck = eREQUEST_ACK_STATUS_SUCCESS_DATA;
    }

    /*******************************************************************************************
     * Variable value conversion (Values that are comma separated)
    *******************************************************************************************/
   // Only if at least 1 return value has been passed
   if (i16BytesToGo > 0)
   {
        uint8_t j = 0;
        uint8_t ui8_numOfVals = 0;
        uint8_t ui8_valueLen = 0;
        uint8_t *p_valStr = NULL;

        while (ui8_numOfVals < MAX_NUM_RESPONSE_VALUES)
        {
            ui8_numOfVals++;

            while (j < i16BytesToGo)
            {
                // Value seperator found
                if (pui8Buf[i + j] == ',')
                    break;
                
                j++;
                ui8_valueLen++;
            }

            p_valStr = (uint8_t*)malloc(ui8_valueLen + 1);

            // copy the number string into new array
            memcpy(p_valStr, &pui8Buf[i + j - ui8_valueLen], ui8_valueLen);

            p_valStr[ui8_valueLen] = '\0';

            #ifdef VALUE_MODE_HEX
            if(!strToHex(p_valStr, &psRsp->uValArr[ui8_numOfVals - 1].ui32_hex))
                return eSCI_ERROR_PARAMETER_CONVERSION_FAILED;
            #else
            psRsp->uValArr[ui8_numOfVals - 1].f_float = atof((char*)p_valStr);
            #endif

            free(p_valStr);

            if (j == i16BytesToGo)
                break;
            
            ui8_valueLen = 0;
            j++;
        }
        psRsp->ui8ResponseDataLength = ui8_numOfVals;

        // if (ui8_numOfVals != psRsp->ui32DataLength)
        //     return eSCI_ERROR_EXPECTED_DATALENGTH_NOT_MET;
    }

    return eSCI_ERROR_NONE;
}

//=============================================================================
teSCI_ERROR SCIMasterStreamParser (uint8_t* pui8Buf, uint8_t ui8DataframeLen, tsRESPONSE *psRsp)
{
    psRsp->eReqType = eREQUEST_TYPE_UPSTREAM;
    psRsp->ui8ResponseDataLength = ui8DataframeLen;
    psRsp->pui8Raw = pui8Buf;

    return eSCI_ERROR_NONE;
}

//=============================================================================
int16_t _CheckAcknowledge (uint8_t *pui8Buf, uint8_t i16BytesToGo)
{
    char cAck[4];
    uint8_t j = 0;

    if (i16BytesToGo < 3)
        return REQUEST_ACKNOWLEDGE_NOT_FOUND;

    memcpy(cAck, pui8Buf, 3);

    cAck[3]='\0';

    for ( ;j < 5; j++)
    {
        if (!strcmp(acknowledgeArr[j], cAck))
            break;
    }

    if (j < 5)
        return j;
    else
        return REQUEST_ACKNOWLEDGE_NOT_FOUND;
}

// //=============================================================================
// void ReturnDataLength (uint8_t *puiBuf)
// {
    
// }

// //=============================================================================
// void GetReturnValues (uint8_t *puiBuf, tuRESPONSEVALUE *puRspValArr)
// {
    
// }