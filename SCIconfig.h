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
 * Defines
 *****************************************************************************/
#define RX_BUFFER_LENGTH    64
#define TX_BUFFER_LENGTH    64

#define SIZE_OF_VAR_STRUCT  10
#define SIZE_OF_CMD_STRUCT  10
#define MAX_NUMBER_OF_EEPROM_VARS 10


#endif // _SCICONFIG_H_