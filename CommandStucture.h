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

/******************************************************************************
 * Type definitions
 *****************************************************************************/
/** \brief Command structure member.
 * 
 * @param pf_valArray       pointer to an Array of parameters to be passed. nullptr if there are none.
 * @param ui8_valArrayLen   Length of the value array.
 * @returns Command execution success indicator.
*/
typedef bool(*COMMAND_CB)(float* pf_valArray, uint8_t ui8_valArrayLen);

#endif // _COMMANDSTRUCTURE_H_