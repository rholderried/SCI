/**************************************************************************//**
 * \file Buffer.cpp
 * \author Roman Holderried
 *
 * \brief Definitions for the Buffer module.
 *
 * <b> History </b>
 * 	- 2022-01-13 - File creation
 *****************************************************************************/

#include "Buffer.h"


/******************************************************************************
 * Class method definitions
 *****************************************************************************/

Buffer::Buffer(uint8_t ui8_size, uint8_t *pui8_buf)
{
    pui8_bufPtr     = pui8_buf;
    i16_bufIdx      = -1;
    ui8_bufLen      = ui8_size;
    ui8_bufSpace    = ui8_size;
    b_ovfl          = false;
}

//=============================================================================
void Buffer::putElem(uint8_t ui8_data)
{
    // Put the data into the buffer only when it is not going to be overflowed
    if (ui8_bufSpace > 0)
    {
        ui8_bufSpace--;
        i16_bufIdx++;
        pui8_bufPtr[i16_bufIdx] = ui8_data;
    }
    else
        b_ovfl = true;
}

//=============================================================================
uint8_t Buffer::readBuf(uint8_t **pui8_target)
{
    uint8_t size = i16_bufIdx + 1;

    *pui8_target = pui8_bufPtr;

    //flushBuf();

    return size;
}

//=============================================================================
void Buffer::flushBuf (void)
{
    i16_bufIdx      = -1;
    ui8_bufSpace    = ui8_bufLen;
    b_ovfl          = false;
} 

//=============================================================================
bool Buffer::getNextFreeBufSpace(uint8_t **pui8_target)
{
    bool success = false;

    if (ui8_bufSpace > 0)
    {
        *pui8_target = &pui8_bufPtr[i16_bufIdx + 1];
        success = true;
    }

    return success;
}

//=============================================================================
bool Buffer::increaseBufIdx(uint8_t ui8_size)
{
    bool success = false;

    if ((i16_bufIdx + ui8_size) < ui8_bufLen)
    {
        i16_bufIdx      += ui8_size;
        ui8_bufSpace    -= ui8_size;
        success         = true;
    }

    return success;

}

//=============================================================================
int16_t Buffer::getActualIdx(void)
{
    return i16_bufIdx;
}