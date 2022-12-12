/**************************************************************************//**
 * \file SCICommon.h
 * \author Roman Holderried
 *
 * \brief Common defines and type definitions for the SCI.
 *
 * <b> History </b>
 * 	- 2022-11-17 - File copied from SCI
 *****************************************************************************/

#ifndef _SCICOMMON_H_
#define _SCICOMMON_H_
/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include <stdbool.h>
/******************************************************************************
 * defines
 *****************************************************************************/
#define SCI

#define SCI_VERSION_MAJOR    0
#define SCI_VERSION_MINOR    6
#define SCI_REVISION         0

#define EEPROM_BYTE_ADDRESSABLE      1
#define EEPROM_WORD_ADDRESSABLE      2
#define EEPROM_LONG_ADDRESSABLE      4

#define UNKNOWN_IDENTIFIER      '#'
#define GETVAR_IDENTIFIER       '?'
#define SETVAR_IDENTIFIER       '!'
#define COMMAND_IDENTIFIER      ':'
#define UPSTREAM_IDENTIFIER     '>'
#define DOWNSTREAM_IDENTIFIER   '<'
/******************************************************************************
 * Type definitions
 *****************************************************************************/

/** \brief SCI Master errors */
typedef enum
{
    eSCI_ERROR_NONE = 0,
    eSCI_ERROR_VAR_NUMBER_INVALID,
    eSCI_ERROR_UNKNOWN_DATATYPE,
    eSCI_ERROR_REQUEST_IDENTIFIER_NOT_FOUND,
    eSCI_ERROR_NUMBER_CONVERSION_FAILED,
    eSCI_ERROR_ACKNOWLEDGE_UNKNOWN,
    eSCI_ERROR_PARAMETER_CONVERSION_FAILED,
    eSCI_ERROR_EXPECTED_DATALENGTH_NOT_MET,
    eSCI_ERROR_MESSAGE_EXCEEDS_TX_BUFFER_SIZE,
    eSCI_ERROR_FEATURE_NOT_IMPLEMENTED
}teSCI_MASTER_ERROR;

/** \brief SCI Slave errors */
typedef enum
{
    eSCI_ERROR_NONE = 0,
    eSCI_ERROR_EEPROM_PARTITION_TABLE_NOT_SUFFICIENT,
    eSCI_ERROR_VAR_NUMBER_INVALID,
    eSCI_ERROR_UNKNOWN_DATATYPE,
    eSCI_ERROR_EEPROM_ADDRESS_UNKNOWN,
    eSCI_ERROR_EEPROM_READOUT_FAILED,
    eSCI_ERROR_EEPROM_WRITE_FAILED,
    eSCI_ERROR_REQUEST_IDENTIFIER_NOT_FOUND,
    eSCI_ERROR_VARIABLE_NUMBER_CONVERSION_FAILED,
    eSCI_ERROR_REQUEST_VALUE_CONVERSION_FAILED,
    eSCI_ERROR_REQUEST_UNKNOWN,
    eSCI_ERROR_UPSTREAM_NOT_INITIATED
}teSCI_SLAVE_ERROR;

/** @brief SCI version data structure */
typedef struct
{
    uint8_t ui8VersionMajor;
    uint8_t ui8VersionMinor;
    uint8_t ui8Revision;
}tsSCI_VERSION;

#define tsSCI_VERSION_VALUE {SCI_VERSION_MAJOR, SCI_VERSION_MINOR, SCI_REVISION}

typedef enum
{
    ePROTOCOL_ERROR         = -1,
    ePROTOCOL_IDLE          = 0,
    ePROTOCOL_SENDING       = 1,
    ePROTOCOL_EVALUATING    = 2,
    ePROTOCOL_RECEIVING     = 3
}tePROTOCOL_STATE;

#endif //_SCICOMMON_H_