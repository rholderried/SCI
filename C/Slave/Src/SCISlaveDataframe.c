/**************************************************************************//**
 * \file SCIDataframe.c
 * \author Roman Holderried
 *
 * \brief Dataframe parsing functionality of the SCI protocol.
 *
 * <b> History </b>
 * 	- 2022-11-21 - File creation
 *  - 2022-12-13 - Adapted code for unified master/slave repo structure.
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "SCICommon.h"
#include "SCISlaveDataframe.h"
#include "SCITransferCommon.h"
#include "Helpers.h"

/******************************************************************************
 * Global variable definition
 *****************************************************************************/
// Note: The idizes correspond to the values of the C enum values!
static const char cAcknowledgeArr [5][4] = {"ACK", "DAT", "UPS", "ERR", "NAK"};
static const uint8_t ui8CmdIdArr[6] = { UNKNOWN_IDENTIFIER, 
                                        GETVAR_IDENTIFIER,
                                        SETVAR_IDENTIFIER,
                                        COMMAND_IDENTIFIER,
                                        UPSTREAM_IDENTIFIER,
                                        DOWNSTREAM_IDENTIFIER};
// const uint8_t ui8_byteLength[7] = {1,1,2,2,4,4,4};

/******************************************************************************
 * Function declarations
 *****************************************************************************/
teSCI_SLAVE_ERROR SCISlaveRequestParser(uint8_t* pui8Buf, uint8_t ui8StringSize, tsREQUEST *psReq)
{
    uint8_t i = 0;
    uint32_t ui32_tmp;
    // uint8_t cmdIdx  = 0;
    // tsREQUEST cmd     = COMMAND_DEFAULT;

    for (; i < ui8StringSize; i++)
    {

        if (pui8Buf[i] == GETVAR_IDENTIFIER)
        {
            psReq->eReqType = eREQUEST_TYPE_GETVAR;
            break;
        }
        else if (pui8Buf[i] == SETVAR_IDENTIFIER)
        {
            psReq->eReqType = eREQUEST_TYPE_SETVAR;
            break;
        }
        else if (pui8Buf[i] == COMMAND_IDENTIFIER)
        {
            psReq->eReqType = eREQUEST_TYPE_COMMAND;
            break;
        }
        else if (pui8Buf[i] == UPSTREAM_IDENTIFIER)
        {
            psReq->eReqType = eREQUEST_TYPE_UPSTREAM;
            break;
        }
        else if (pui8Buf[i] == DOWNSTREAM_IDENTIFIER)
        {
            psReq->eReqType = eREQUEST_TYPE_DOWNSTREAM;
            break;
        }      
    }

    // No valid command identifier found (TODO: Error handling)
    if (psReq->eReqType == eREQUEST_TYPE_NONE)
        return eSCI_SLAVE_ERROR_REQUEST_IDENTIFIER_NOT_FOUND;
    
    /*******************************************************************************************
     * Variable number conversion
    *******************************************************************************************/
    // Loop breaks when i reflects the buffer position of the command identifier
    // Variable number conversion
    {
        // One additional character necessary for string termination
        uint8_t *p_numStr = (uint8_t*)malloc(i+1);

        // copy the number string into new array
        memcpy(p_numStr,pui8Buf,i);
        // Properly terminate string to use the atoi buildin
        p_numStr[i] = '\0';
        // Convert
        #ifdef VALUE_MODE_HEX
        if(!strToHex(p_numStr, &ui32_tmp))
           return eSCI_SLAVE_ERROR_VARIABLE_NUMBER_CONVERSION_FAILED; 
        psReq->i16Num = *(int16_t*)(&ui32_tmp);
        #else
        psReq->i16Num = (int16_t)(atoi((char*)p_numStr));
        #endif

        free(p_numStr);
    }

    /************************************s*******************************************************
     * Variable value conversion
    *******************************************************************************************/
   // Only if a parameter has been passed
   if (ui8StringSize > i + 1)
   {
        uint8_t ui8_valStrLen = ui8StringSize - i;
        uint8_t j = 1;
        uint8_t ui8NumOfVals = 0;
        uint8_t ui8_valueLen = 0;
        uint8_t *p_valStr = NULL;

        while (ui8NumOfVals <= MAX_NUM_REQUEST_VALUES)
        {
            ui8NumOfVals++;

            while (j < ui8_valStrLen)
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
            if(!strToHex(p_valStr, &psReq->uValArr[ui8NumOfVals - 1].ui32_hex))
                return eSCI_SLAVE_ERROR_REQUEST_VALUE_CONVERSION_FAILED;
            #else
            psReq->valArr[ui8NumOfVals - 1].f_float = atof((char*)p_valStr);
            #endif

            free(p_valStr);

            if (j == ui8_valStrLen)
                break;
            
            ui8_valueLen = 0;
            j++;
        }
        psReq->ui8ValArrLen = ui8NumOfVals;
    }

    return eSCI_SLAVE_ERROR_NONE;
}

//=============================================================================
uint8_t SCISlaveResponseBuilder(uint8_t *pui8Buf, bool bFirstPacketNotSent, bool bOngoing, uint32_t *pui32DataIdx, tsRESPONSE *psRsp)
{
    uint8_t ui8_size    = 0;

    // Convert variable number to ASCII
    #ifdef VALUE_MODE_HEX
    ui8_size = (uint8_t)hexToStrWord(pui8Buf, (uint16_t*)&psRsp->i16Num, true);
    #else
    ui8_size = ftoa(pui8Buf, (float)sRsp.i16Num, true);
    #endif

    // Increase Buffer index and write command type identifier
    pui8Buf += ui8_size;
    *pui8Buf++ = ui8CmdIdArr[psRsp->eReqType];
    ui8_size++;


    if (!(psRsp->eReqAck == eREQUEST_ACK_STATUS_ERROR || psRsp->eReqAck == eREQUEST_ACK_STATUS_UNKNOWN))
    {
        switch (psRsp->eReqType)
        {
            case eREQUEST_TYPE_GETVAR:
                // Fill the response designator
                memcpy(pui8Buf, &cAcknowledgeArr[(uint8_t)eREQUEST_ACK_STATUS_SUCCESS], 3);
                pui8Buf+=3;
                *pui8Buf++ = ';';
                ui8_size += 4;
                // Write the data value into the buffer
                #ifdef VALUE_MODE_HEX
                ui8_size += (uint8_t)hexToStrDword(pui8Buf, &psRsp->sTransferData.puRespVals[0].ui32_hex, true);
                #else
                ui8_size += ftoa(pui8Buf, psRsp->sTransferData.puRespVals[0].f_float, true);
                #endif
                break;
            
            case eREQUEST_TYPE_SETVAR:
                // If we got here, the operation was successful
                memcpy(pui8Buf, &cAcknowledgeArr[(uint8_t)eREQUEST_ACK_STATUS_SUCCESS], 3);
                pui8Buf+=3;
                ui8_size += 3;
                break;

            case eREQUEST_TYPE_COMMAND:
                // No response designator on every consecutive packet
                if (bFirstPacketNotSent)
                {
                    memcpy(pui8Buf, &cAcknowledgeArr[(uint8_t)psRsp->eReqAck], 3);
                    pui8Buf+=3;
                    ui8_size += 3;

                    if (psRsp->eReqAck == eREQUEST_ACK_STATUS_SUCCESS_DATA ||psRsp->eReqAck == eREQUEST_ACK_STATUS_SUCCESS_UPSTREAM)
                    {
                        uint8_t ui8AsciiSize;

                        *pui8Buf++ = ';';
                        ui8_size++;
                        #ifdef VALUE_MODE_HEX
                        ui8AsciiSize = (uint8_t)hexToStrDword(pui8Buf, &psRsp->sTransferData.ui32DatLen, true);
                        #else
                        ui8AsciiSize = ftoa(pui8Buf, (float)psRsp->sTransferData.ui32_datLen, true);
                        #endif
                        pui8Buf += ui8AsciiSize;
                        ui8_size += ui8AsciiSize;
                    }
                }

                // Fill the rest of the packet with data
                if (bOngoing)
                {
                    if(bFirstPacketNotSent)
                    {
                        *pui8Buf++ = ';';
                        ui8_size++;
                    }
                    ui8_size += _SCIFillBufferWithValues(pui8Buf, TX_PACKET_LENGTH - ui8_size, pui32DataIdx, psRsp);
                }

                break;
            
            case eREQUEST_TYPE_UPSTREAM:
                // upstream is sent without command ID overhead
                pui8Buf -= ui8_size;
                ui8_size = 0;
                ui8_size += _SCIFillBufferWithValues(pui8Buf, TX_PACKET_LENGTH, pui32DataIdx, psRsp);
                break;

            default:
                break;
        }
    }
    else
    {
        // We get here for example if there was no valid command identifier found
        if (psRsp->sTransferData.ui16Error == 0)
        {
            memcpy(pui8Buf, &cAcknowledgeArr[(uint8_t)eREQUEST_ACK_STATUS_UNKNOWN], 3);
            pui8Buf += 3;
        }
        else
        {
            memcpy(pui8Buf, &cAcknowledgeArr[(uint8_t)eREQUEST_ACK_STATUS_ERROR], 3);
            pui8Buf+=3;
            *pui8Buf++ = ';';
            ui8_size ++;
            // Write the data value into the buffer
            #ifdef VALUE_MODE_HEX
            ui8_size += (uint8_t)hexToStrWord(pui8Buf, &psRsp->sTransferData.ui16Error, true);
            #else
            ui8_size += ftoa(pui8Buf, (float)psRsp->sTransferData.ui16Error, true);
            #endif
        }

        ui8_size += 3;
    }

    return ui8_size;

}

//=============================================================================
uint8_t _SCIFillBufferWithValues(uint8_t * pui8Buf, uint8_t ui8MaxSize, uint32_t *pui32DataIdx, tsRESPONSE *psRsp)
{
    uint8_t ui8_currentDataSize = 0;

    if (psRsp->eReqType == eREQUEST_TYPE_UPSTREAM)
    {
        // Check if there is a valid buffer pointer passed
        if (psRsp->sTransferData.pui8UpStreamBuf == NULL)
            return 0;

        // Upstream data does not get converted into an ASCII-stream
        // Determine the actual packet length
        ui8MaxSize = psRsp->sTransferData.ui32DatLen < ui8MaxSize ? psRsp->sTransferData.ui32DatLen : ui8MaxSize;
        memcpy(pui8Buf, &psRsp->sTransferData.pui8UpStreamBuf[*pui32DataIdx], ui8MaxSize);

        psRsp->sTransferData.ui32DatLen -= ui8MaxSize;
        *pui32DataIdx += ui8MaxSize;
        ui8_currentDataSize += ui8MaxSize;

        // while (ui8MaxSize > (ui8_currentDataSize + 1) && psTransfer->sResponseControl.sRsp.sTransferData.ui32DatLen > 0)
        // {
        //     // ui8_currentDataSize += hexToStr(pui8Buf,(uint32_t*)&psTransfer->sResponseControl.sRsp.sTransferData.pui8_buf[psTransfer->sResponseControl.ui32_byteIdx],2,false);
        //     ui8_currentDataSize += hexToStrByte(pui8Buf,&psTransfer->sResponseControl.sRsp.sTransferData.pui8UpStreamBuf[psTransfer->sResponseControl.ui32DataIdx], false);

        //     psTransfer->sResponseControl.sRsp.sTransferData.ui32DatLen--;
        //     psTransfer->sResponseControl.ui32DataIdx++;
        //     pui8Buf += 2;
        // }
    }
    else if (psRsp->eReqType == eREQUEST_TYPE_COMMAND)
    {
        bool    bCommaSet = false;
        uint8_t ui8AsciiSize;
        uint8_t ui8DataBuf[20];

        #ifndef VALUE_MODE_HEX
        float   f_passVal;
        #endif

        // Check if there is a valid data format table pointer passed
        if (psRsp->sTransferData.puRespVals == NULL)
            return 0;

        while (true)
        {
            // All data is handled
            if (psRsp->sTransferData.ui32DatLen == 0)
            {
                if (bCommaSet)
                {
                    ui8_currentDataSize--;
                    pui8Buf--;
                }
                break;
            }

            ui8AsciiSize = (uint8_t)hexToStrDword(ui8DataBuf, (uint32_t*)&psRsp->sTransferData.puRespVals[*pui32DataIdx], true);

            // Fits the value in the buffer?
            if ((ui8_currentDataSize + ui8AsciiSize) < ui8MaxSize)
            {
                // Copy the value in the send buffer
                memcpy(pui8Buf, ui8DataBuf, ui8AsciiSize);
                ui8_currentDataSize += ui8AsciiSize;

                // Handle all indices
                psRsp->sTransferData.ui32DatLen--;
                *pui32DataIdx++;
                pui8Buf += ui8AsciiSize;

                if (ui8MaxSize > ui8_currentDataSize)
                {
                    *pui8Buf++ = ',';
                    ui8_currentDataSize++;
                    bCommaSet = true;
                }
                else
                    break;
            }
            else
            {
                if (bCommaSet)
                {
                    ui8_currentDataSize--;
                    pui8Buf--;
                }
                break;
            }
        }
    }

    return ui8_currentDataSize;
}