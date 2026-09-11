/**************************************************************************//**
 * \file SCIconfig.h
 * \author Roman Holderried
 *
 * \brief SCI configuration owned by the C/Test GoogleTest suite.
 *
 * This header configures buffer sizes, var/command struct sizes, and other
 * compile-time SCI parameters used ONLY by the C/Test GoogleTest suite
 * (VariablesAndCommands.c, TestCallbacks.c, gtest/*.cpp). It is NOT a
 * generic library default — SCI is platform independent and every
 * integration is expected to supply its own SCIconfig.h. See
 * C/config/SCIconfig_Template.h for a documented starting point to copy
 * into your own project.
 *
 * <b> History </b>
 * 	- 2022-01-13 - File creation (as C/config/SCIconfig.h)
 *  - 2022-03-17 - Port to C (Originally from SerialProtocol)
 *  - 2022-12-13 - Adapted code for unified master/slave repo structure.
 *  - 2026-09-10 - Split off as C/Test/config/SCIconfig.h, the GoogleTest
 *                 suite's own configuration; C/config/SCIconfig.h became a
 *                 template (see C/config/SCIconfig_Template.h).
 *  - 2026-09-11 - Added MAX_NUMBER_OF_PARAMETER_DIGITS (now a required
 *                 SCIconfig.h parameter, see SCITransferCommon.h); updated
 *                 the SCI_ERROR_OFFSET comment's error count (11 -> 12)
 *                 for the two new error codes added to SCICommon.h.
 *****************************************************************************/

#ifndef _SCICONFIG_H_
#define _SCICONFIG_H_

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "SCICommon.h"

/******************************************************************************
 * Defines
 *****************************************************************************/
#define RX_PACKET_LENGTH    128
#define TX_PACKET_LENGTH    128

#define SIZE_OF_VAR_STRUCT  7
#define SIZE_OF_CMD_STRUCT  5
#define MAX_NUMBER_OF_EEPROM_VARS 10

// Mode configuration
#define SEND_MODE_BYTE_BY_BYTE
#define VALUE_MODE_HEX

// EEPROM configuration
#define EEPROM_ADDRESSTYPE  EEPROM_WORD_ADDRESSABLE
#define ADDRESS_OFFET       0

// SCI error offset (SCI currently defines 12 errors)
#define SCI_ERROR_OFFSET    0x100

// Number of request and response values
#define MAX_NUM_REQUEST_VALUES  10
#define MAX_NUM_RESPONSE_VALUES 10

// Maximum characters of a single wire-format parameter value (see
// MAX_NUMBER_OF_PARAMETER_DIGITS in SCITransferCommon.h). VALUE_MODE_HEX
// values are hex-encoded uint32_t, so 8 covers the full range.
#define MAX_NUMBER_OF_PARAMETER_DIGITS 8

#endif // _SCICONFIG_H_
