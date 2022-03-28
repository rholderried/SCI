/**************************************************************************//**
 * \file CommandStructure.h
 * \author Roman Holderried
 *
 * \brief Declares datatypes necessary for setting up the command structure.
 *
 * <b> History </b>
 * 	- 2022-01-18 - File creation
 *****************************************************************************/

#ifndef _COMMANDSTRUCTURE_H_
#define _COMMANDSTRUCTURE_H_

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "SCIconfig.h"
#include "Variables.h"

/******************************************************************************
 * Type definitions
 *****************************************************************************/
typedef enum
{
    eCOMMAND_STATUS_SUCCESS         = 0,
    eCOMMAND_STATUS_DATA_BYTES      = 1,
    eCOMMAND_STATUS_DATA_VALUES     = 2,
    eCOMMAND_STATUS_ERROR           = 3,
    eCOMMAND_STATUS_UNKNOWN         = 4
}COMMAND_CB_STATUS;

typedef struct
{
    uint8_t  *pui8_buf;
    uint32_t ui32_datLen;
    DTYPE    *eDataFormat;
    uint16_t ui16_dataFormatLen;
    uint16_t ui16_error;
}PROCESS_INFO;

#define PROCESS_INFO_DEFAULT {NULL, 0, NULL, 0, 0}

/** \brief Command structure member.
 * 
 * @param pf_valArray       pointer to an Array of parameters to be passed. nullptr if there are none.
 * @param ui8_valArrayLen   Length of the value array.
 * @returns Command execution success indicator.
*/
#ifdef VALUE_MODE_HEX
typedef COMMAND_CB_STATUS(*COMMAND_CB)(uint32_t* pui32_valArray, uint8_t ui8_valArrayLen, PROCESS_INFO *p_info);
#else
typedef COMMAND_CB_STATUS(*COMMAND_CB)(float* pf_valArray, uint8_t ui8_valArrayLen, PROCESS_INFO *p_info);
#endif

#endif // _COMMANDSTRUCTURE_H_