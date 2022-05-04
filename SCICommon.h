/**************************************************************************//**
 * \file SCICommon.h
 * \author Roman Holderried
 *
 * \brief Common defines and type definitions for the SCI.
 *
 * <b> History </b>
 * 	- 2022-05-02 - File creation
 *****************************************************************************/

#ifndef SCICOMMON_H_
#define SCICOMMON_H_
/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "SCIconfig.h"

/******************************************************************************
 * defines
 *****************************************************************************/
#define EEPROM_BYTE_ADDRESSABLE      1
#define EEPROM_WORD_ADDRESSABLE      2
#define EEPROM_LONG_ADDRESSABLE      4

/******************************************************************************
 * Type definitions
 *****************************************************************************/
typedef enum
{
    eSCI_ERROR_NONE = 0,
    eSCI_ERROR_EEPROM_PARTITION_TABLE_NOT_SUFFICIENT,
    eSCI_ERROR_VAR_NUMBER_INVALID,
    eSCI_ERROR_UNKNOWN_DATATYPE,
    eSCI_ERROR_EEPROM_ADDRESS_UNKNOWN,
    eSCI_ERROR_EEPROM_READOUT_FAILED,
    eSCI_ERROR_EEPROM_WRITE_FAILED,
    eSCI_ERROR_COMMAND_IDENTIFIER_NOT_FOUND,
    eSCI_ERROR_VARIABLE_NUMBER_CONVERSION_FAILED,
    eSCI_ERROR_COMMAND_VALUE_CONVERSION_FAILED,
    eSCI_ERROR_COMMAND_UNKNOWN,
    eSCI_ERROR_UPSTREAM_NOT_INITIATED

}tSCI_ERROR;



#endif //SCICOMMON_H_