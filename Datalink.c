/**************************************************************************//**
 * \file DataLink.c
 * \author Roman Holderried
 *
 * \brief Data link layer functionality for the SCI protocol.
 *
 * <b> History </b>
 * 	- 2022-03-20 - File creation
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include <stddef.h>
#include "DataLink.h"
#include "Buffer.h"

/******************************************************************************
 * Function definitions
 *****************************************************************************/

void receive(DATALINK *p_inst, FIFO_BUF *p_rBuf, uint8_t ui8_data)
{
    if (ui8_data == STX)
    {
        if (p_inst->rState == eDATALINK_RSTATE_IDLE)
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
DATALINK_RECEIVE_STATE getDatalinkReceiveState(DATALINK *p_inst)
{
    return (p_inst->rState);
}

//=============================================================================
DATALINK_TRANSMIT_STATE getDatalinkTransmitState(DATALINK *p_inst)
{
    return (p_inst->tState);
}

//=============================================================================
bool transmit(DATALINK *p_inst, FIFO_BUF * p_tBuf)
{
    // We can't send without the proper callbacks
    if (p_inst->txCallback == NULL || p_inst->txGetBusyStateCallback == NULL)
        return (false);

    if (p_inst->tState == eDATALINK_TSTATE_IDLE)
    {
        p_inst->txInfo.ui8_bufLen = readBuf(p_tBuf, &p_inst->txInfo.pui8_buf);
        p_inst->tState = eDATALINK_TSTATE_SEND_STX;     
    }

    return (true);
}

//=============================================================================
void transmitStateMachine(DATALINK *p_inst)
{    
    if (p_inst->txGetBusyStateCallback())
        if (p_inst->txGetBusyStateCallback())
            return;

    switch (p_inst->tState)
    {
        case eDATALINK_TSTATE_SEND_STX:
            {
                uint8_t ui8_data = STX;
                p_inst->txCallback(&ui8_data, 1);
                p_inst->tState = eDATALINK_TSTATE_SEND_BUFFER;
            }
            break;

        case eDATALINK_TSTATE_SEND_BUFFER:
            {   
                uint8_t ui8_sent = p_inst->txCallback(p_inst->txInfo.pui8_buf, p_inst->txInfo.ui8_bufLen);
                p_inst->txInfo.ui8_bufLen -= ui8_sent;
                p_inst->txInfo.pui8_buf += ui8_sent;

                if (p_inst->txInfo.ui8_bufLen == 0)
                    p_inst->tState = eDATALINK_TSTATE_SEND_ETX;
            }
            break;

        case eDATALINK_TSTATE_SEND_ETX:
            {
                uint8_t ui8_data = ETX;
                p_inst->txCallback(&ui8_data, 1);
                p_inst->tState = eDATALINK_TSTATE_READY;
            }
            break;
            
        default:
            break;
    }
}

//=============================================================================
void acknowledgeTx(DATALINK *p_inst)
{
    p_inst->tState = eDATALINK_TSTATE_IDLE;
}
