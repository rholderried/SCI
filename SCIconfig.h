/**************************************************************************//**
 * \file SCIconfig.h
 * \author Roman Holderried
 *
 * \brief Request - Response protocol functionality.
 * 
 * This serial protocol has been initially written for the MMX heater controller
 * module. It provides data read/write access and a command interface to the 
 * application.
 *
 * <b> History </b>
 * 	- 2022-01-13 - File creation 
 *  - 2022-03-17 - Port to C (Originally from SerialProtocol)
 *****************************************************************************/

#ifndef _SCICONFIG_H_
#define _SCICONFIG_H_

/******************************************************************************
 * Defines - Not to be changed!
 *****************************************************************************/
#define EEPROM_BYTE_ADDRESSABLE      1
#define EEPROM_WORD_ADDRESSABLE      2
#define EEPROM_LONG_ADDRESSABLE      4

/******************************************************************************
 * Defines
 *****************************************************************************/
#define RX_PACKET_LENGTH    128
#define TX_PACKET_LENGTH    128

#define SIZE_OF_VAR_STRUCT  5
#define SIZE_OF_CMD_STRUCT  2
#define MAX_NUMBER_OF_EEPROM_VARS 10

// Mode configuration
#define SEND_MODE_BYTE_BY_BYTE
#define VALUE_MODE_HEX

// EEPROM configuration
#define EEPROM_ADDRESSTYPE  EEPROM_WORD_ADDRESSABLE
#define ADDRESS_OFFET       0

// Send parameters
#define MAX_COUNT_DATA_FORMATS  10


#endif // _SCICONFIG_H_