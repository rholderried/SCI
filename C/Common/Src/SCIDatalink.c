/**************************************************************************//**
 * \file SCIDataLink.c
 * \author Roman Holderried
 *
 * \brief Data link layer functionality for the SCI protocol.
 *
 * <b> History </b>
 * 	- 2022-11-17 - Copy from SCI
 *  - 2022-12-13 - Adapted code for unified master/slave repo structure.
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include <stddef.h>
#include "SCIDataLink.h"
#include "Buffer.h"
#include "SCIconfig.h"

/******************************************************************************
 * Function definitions
 *****************************************************************************/

void SCIDataLinkReceiveTransfer(tsDATALINK *p_inst, tsFIFO_BUF *p_rBuf, uint8_t ui8_data)
{
    if (ui8_data == STX)
    {
        if (p_inst->rState == eDATALINK_RSTATE_WAIT_STX)
        {
            flushBuf(p_rBuf);
            p_inst->rState = eDATALINK_RSTATE_BUSY;
            // putElem(p_rBuf, ui8_data);
        }
        else
        // TODO: Error handling
        {
            p_inst->rState = eDATALINK_RSTATE_IDLE;
        }
    }
    else if (ui8_data == ETX)
    {
        
        if (p_inst->rState == eDATALINK_RSTATE_BUSY)
        {
            p_inst->rState = eDATALINK_RSTATE_PENDING;
            // rxBuffer.putElem(ui8_data);
        }
        else
        // TODO: Error handling
        {
            p_inst->rState = eDATALINK_RSTATE_IDLE;
        }
    }
    else if (p_inst->rState == eDATALINK_RSTATE_BUSY)
    {
        putElem(p_rBuf, ui8_data);
    }

    // Debug function activation (Function call directly from datalink layer)
    if (p_inst->rState == eDATALINK_RSTATE_IDLE)
    {
        switch(p_inst->dbgActState)
        {
            case eDATALINK_DBGSTATE_IDLE:
                p_inst->dbgActState = ui8_data == 'D' ? eDATALINK_DBGSTATE_S1 : eDATALINK_DBGSTATE_IDLE;
                break;
            case eDATALINK_DBGSTATE_S1:
                p_inst->dbgActState = ui8_data == 'b' ? eDATALINK_DBGSTATE_S2 : eDATALINK_DBGSTATE_IDLE;
                break;
            case eDATALINK_DBGSTATE_S2:
                p_inst->dbgActState = ui8_data == 'g' ? eDATALINK_DBGSTATE_S3 : eDATALINK_DBGSTATE_IDLE;
                break;
            case eDATALINK_DBGSTATE_S3:
                {
                    int8_t ui8_parsed = -1;
                    ui8_parsed = ui8_data - '0';

                    // If the number is in range, execute callback
                    if (ui8_parsed >= 0 && ui8_parsed < 10)
                    {
                        if(p_inst->dbgFcnArray[ui8_parsed] != NULL)
                            p_inst->dbgFcnArray[ui8_parsed]();
                    }
                    p_inst->dbgActState = eDATALINK_DBGSTATE_IDLE;
                }
                break;       
        
            default:
                break;
        }
    }
    else
        p_inst->dbgActState = eDATALINK_DBGSTATE_IDLE;
}

//=============================================================================
void SCIDataLinkReceiveStream(tsDATALINK *p_inst, tsFIFO_BUF *p_rBuf, uint8_t ui8_data)
{
    // STX triggers the receive state to be busy, but can also appear anywhere in the message.
    if (p_inst->rState == eDATALINK_RSTATE_WAIT_STX)
    {
        if (ui8_data == STX)
        {
            // Prepare receive buffer
            flushBuf(p_rBuf);
            p_inst->rState = eDATALINK_RSTATE_BUSY;
            // putElem(p_rBuf, ui8_data);
            p_inst->sRxInfo.ui8MsgByteCnt = 0;
            // Receiver now ready to receive stream bytes
        }
    }
    else if (p_inst->rState == eDATALINK_RSTATE_BUSY)
    {
        if (p_inst->sRxInfo.ui32BytesToGo > 0 && p_inst->sRxInfo.ui8MsgByteCnt < RX_PACKET_LENGTH)
        {
            putElem(p_rBuf, ui8_data);
            p_inst->sRxInfo.ui32BytesToGo--;
            p_inst->sRxInfo.ui8MsgByteCnt++;
        }
        // Last byte (of transfer or message) must be ETX
        else if (ui8_data == ETX)
            p_inst->rState = eDATALINK_RSTATE_PENDING;
        else
            p_inst->rState = eDATALINK_RSTATE_IDLE;
    }
}

//=============================================================================
teDATALINK_RECEIVE_STATE SCIDatalinkGetReceiveState(tsDATALINK *p_inst)
{
    return (p_inst->rState);
}

//=============================================================================
teDATALINK_TRANSMIT_STATE SCIDatalinkGetTransmitState(tsDATALINK *p_inst)
{
    return (p_inst->tState);
}

//=============================================================================
bool SCIDatalinkTransmit(tsDATALINK *p_inst, tsFIFO_BUF * p_tBuf)
{
    // We can't send without the proper callbacks
    #ifndef SEND_MODE_BYTE_BY_BYTE
    if (p_inst->txNonBlockingCallback == NULL || p_inst->txGetBusyStateCallback == NULL)
        return (false);
    #else
    if (p_inst->txBlockingCallback == NULL)
        return (false);
    #endif
    

    if (p_inst->tState == eDATALINK_TSTATE_IDLE)
    {
        p_inst->sTxInfo.ui8_bufLen = readBuf(p_tBuf, &p_inst->sTxInfo.pui8_buf);
        p_inst->tState = eDATALINK_TSTATE_SEND_STX;     
    }

    return (true);
}

//=============================================================================
void SCIDatalinkTransmitStateMachine(tsDATALINK *p_inst)
{    
    // Prevent from entering this function if the Tx interface is still busy
    if (p_inst->txGetBusyStateCallback != NULL)
        if (p_inst->txGetBusyStateCallback())
            return;

    switch (p_inst->tState)
    {
        case eDATALINK_TSTATE_SEND_STX:
            {
                uint8_t ui8_data = STX;
                #ifdef SEND_MODE_BYTE_BY_BYTE
                p_inst->txBlockingCallback(&ui8_data, 1);
                #else
                p_inst->txNonBlockingCallback(&ui8_data, 1);
                #endif
                p_inst->tState = eDATALINK_TSTATE_SEND_BUFFER;
            }
            break;

        case eDATALINK_TSTATE_SEND_BUFFER:
            {   
                #ifdef SEND_MODE_BYTE_BY_BYTE
                p_inst->txBlockingCallback(p_inst->sTxInfo.pui8_buf++, 1);
                p_inst->sTxInfo.ui8_bufLen--;

                if (p_inst->sTxInfo.ui8_bufLen == 0)
                {
                    p_inst->tState = eDATALINK_TSTATE_SEND_ETX;
                }
                #else
               
                p_inst->txNonBlockingCallback(p_inst->sTxInfo.pui8_buf, p_inst->sTxInfo.ui8_bufLen);
                p_inst->tState = eDATALINK_TSTATE_SEND_ETX;
                
                #endif   
                    
            }
            break;

        case eDATALINK_TSTATE_SEND_ETX:
            {
                uint8_t ui8_data = ETX;
                #ifdef SEND_MODE_BYTE_BY_BYTE
                p_inst->txBlockingCallback(&ui8_data, 1);
                #else
                p_inst->txNonBlockingCallback(&ui8_data, 1);
                #endif
                p_inst->tState = eDATALINK_TSTATE_READY;
            }
            break;
            
        default:
            break;
    }
}

//=============================================================================
void SCIDatalinkAcknowledgeRx(tsDATALINK *p_inst)
{
    p_inst->rState = eDATALINK_RSTATE_IDLE;
}

//=============================================================================
void SCIDatalinkAcknowledgeTx(tsDATALINK *p_inst)
{
    p_inst->tState = eDATALINK_TSTATE_IDLE;
}

//=============================================================================
void SCIDatalinkStartRx(tsDATALINK *p_inst)
{
    p_inst->rState = eDATALINK_RSTATE_WAIT_STX;
}