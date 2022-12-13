/**************************************************************************//**
 * \file NativeTestControl.c
 * \author Roman Holderried
 *
 * \brief Controls the data transfer between Master and slave.
 *
 * <b> History </b>
 * 	- 2022-12-13 - File creation
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "SCISlave.h"
#include "SCIMaster.h"

/******************************************************************************
 * Exported Globals
 *****************************************************************************/
struct
{
    struct
    {
        uint8_t ui8Idx;
    }sTriggerSlave;
    struct
    {
        uint8_t ui8Idx;
    }sTriggerMaster;
}sTestControl = {{0},{0}};

void TriggerMaster(uint8_t* pBuf, uint8_t ui8Size)
{
    for(uint8_t i = 0; i < ui8Size; i++)
        SCIMasterReceiveData(&pBuf[i], ui8Size);
}
