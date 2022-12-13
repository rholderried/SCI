/**************************************************************************//**
 * \file CommandStructure.h
 * \author Roman Holderried
 *
 * \brief Declares datatypes necessary for setting up the command structure.
 *
 * <b> History </b>
 * 	- 2022-01-18 - File creation
 *  - 2022-12-13 - Adapted code for unified master/slave repo structure.
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
#include "SCITransferCommon.h"

/******************************************************************************
 * Type definitions
 *****************************************************************************/
// typedef enum
// {
//     eCOMMAND_STATUS_SUCCESS             = 0,
//     eCOMMAND_STATUS_SUCCESS_DATA        = 1,
//     eCOMMAND_STATUS_SUCCESS_UPSTREAM    = 2,
//     eCOMMAND_STATUS_ERROR               = 3,
//     eCOMMAND_STATUS_UNKNOWN             = 4
// }COMMAND_CB_STATUS;

/** \brief Command structure member.
 * 
 * @param pf_valArray       pointer to an Array of parameters to be passed. nullptr if there are none.
 * @param ui8_valArrayLen   Length of the value array.
 * @param psData            Pointer to the transfer data structure.
 * @returns Command execution success indicator.
*/
#ifdef VALUE_MODE_HEX
typedef teREQUEST_ACKNOWLEDGE(*COMMAND_CB)(uint32_t* pui32_valArray, uint8_t ui8_valArrayLen, tsTRANSFER_DATA *psData);
#else
typedef teREQUEST_ACKNOWLEDGE(*COMMAND_CB)(float* pf_valArray, uint8_t ui8_valArrayLen, tsTRANSFER_DATA *psData);
#endif

#endif // _COMMANDSTRUCTURE_H_