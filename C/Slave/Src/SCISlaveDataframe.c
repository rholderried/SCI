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
#include "SCISlaveDataframe.h"
// #include "SCITransfer.h"
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
