/**************************************************************************//**
 * \file Buffer.cpp
 * \author Roman Holderried
 *
 * \brief Definitions for the Buffer module.
 *
 * <b> History </b>
 * 	- 2022-11-17 - Copy from SCI
 *  - 2022-12-13 - Adapted code for unified master/slave repo structure.
 *****************************************************************************/

#include "Buffer.h"

/******************************************************************************
 * Function definitions
 *****************************************************************************/
void fifoBufInit(tsFIFO_BUF* p_inst, uint8_t *pui8_buf, uint8_t ui8_bufLen)
{
    p_inst->b_ovfl = false;
    p_inst->ui8_bufLen = ui8_bufLen;
    p_inst->ui8_bufSpace = ui8_bufLen;
    p_inst->pui8_bufPtr = pui8_buf;
}

//=============================================================================
void putElem(tsFIFO_BUF* p_inst, uint8_t ui8_data)
{
    // Put the data into the buffer only when it is not going to be overflowed
    if (p_inst->ui8_bufSpace > 0)
    {
        p_inst->ui8_bufSpace--;
        p_inst->i16_bufIdx++;
        p_inst->pui8_bufPtr[p_inst->i16_bufIdx] = ui8_data;
    }
    else
        p_inst->b_ovfl = true;
}

//=============================================================================
uint8_t readBuf(tsFIFO_BUF* p_inst, uint8_t **pui8_target)
{
    uint8_t size = p_inst->i16_bufIdx + 1;

    *pui8_target = p_inst->pui8_bufPtr;

    //flushBuf();

    return size;
}

//=============================================================================
void flushBuf (tsFIFO_BUF* p_inst)
{
    p_inst->i16_bufIdx      = -1;
    p_inst->ui8_bufSpace    = p_inst->ui8_bufLen;
    p_inst->b_ovfl          = false;
} 

//=============================================================================
bool getNextFreeBufSpace(tsFIFO_BUF* p_inst, uint8_t **pui8_target)
{
    bool success = false;

    if (p_inst->ui8_bufSpace > 0)
    {
        *pui8_target = &p_inst->pui8_bufPtr[p_inst->i16_bufIdx + 1];
        success = true;
    }

    return success;
}

//=============================================================================
bool increaseBufIdx(tsFIFO_BUF* p_inst, uint8_t ui8_size)
{
    bool success = false;

    if ((p_inst->i16_bufIdx + ui8_size) < p_inst->ui8_bufLen)
    {
        p_inst->i16_bufIdx      += ui8_size;
        p_inst->ui8_bufSpace    -= ui8_size;
        success         = true;
    }

    return success;

}

//=============================================================================
int16_t getActualIdx(tsFIFO_BUF* p_inst)
{
    return p_inst->i16_bufIdx;
}