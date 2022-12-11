/**************************************************************************//**
 * \file SCICommon.h
 * \author Roman Holderried
 *
 * \brief Common defines and type definitions for the SCI.
 *
 * <b> History </b>
 * 	- 2022-11-17 - File copied from SCI
 *****************************************************************************/

#ifndef SCICOMMON_H_
#define SCICOMMON_H_
/******************************************************************************
 * Includes
 *****************************************************************************/

/******************************************************************************
 * defines
 *****************************************************************************/
#define SCI
#define SCI_VERSION_MAJOR    0
#define SCI_VERSION_MINOR    6
#define SCI_REVISION         0

#define GETVAR_IDENTIFIER       '?'
#define SETVAR_IDENTIFIER       '!'
#define COMMAND_IDENTIFIER      ':'
#define UPSTREAM_IDENTIFIER     '>'
#define DOWNSTREAM_IDENTIFIER   '<'

#define EEPROM_BYTE_ADDRESSABLE      1
#define EEPROM_WORD_ADDRESSABLE      2
#define EEPROM_LONG_ADDRESSABLE      4

/******************************************************************************
 * Type definitions
 *****************************************************************************/

/** \brief SCI Master errors */
typedef enum
{
    eSCI_ERROR_NONE = 0,
    eSCI_ERROR_VAR_NUMBER_INVALID,
    eSCI_ERROR_UNKNOWN_DATATYPE,
    eSCI_ERROR_COMMAND_IDENTIFIER_NOT_FOUND,
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
    eSCI_ERROR_COMMAND_IDENTIFIER_NOT_FOUND,
    eSCI_ERROR_VARIABLE_NUMBER_CONVERSION_FAILED,
    eSCI_ERROR_COMMAND_VALUE_CONVERSION_FAILED,
    eSCI_ERROR_COMMAND_UNKNOWN,
    eSCI_ERROR_UPSTREAM_NOT_INITIATED
}tSCI_SLAVE_ERROR;


/** \brief Request acknowledge enumeration */
typedef enum
{
    eREQUEST_ACK_STATUS_SUCCESS             = 0,
    eREQUEST_ACK_STATUS_SUCCESS_DATA        = 1,
    eREQUEST_ACK_STATUS_SUCCESS_UPSTREAM    = 2,
    eREQUEST_ACK_STATUS_ERROR               = 3,
    eREQUEST_ACK_STATUS_UNKNOWN             = 4
}teREQUEST_ACKNOWLEDGE;

/** \brief Return value of the Transfer callbacks*/
typedef enum
{
    eTRANSFER_ACK_SUCCESS = 0,
    eTRANSFER_ACK_REPEAT_REQUEST,
    eTRANSFER_ACK_ABORT
}teTRANSFER_ACK;

/** @brief SCI version data structure */
typedef struct
{
    uint8_t ui8VersionMajor;
    uint8_t ui8VersionMinor;
    uint8_t ui8Revision;
}tSCI_VERSION;




#endif //SCICOMMON_H_