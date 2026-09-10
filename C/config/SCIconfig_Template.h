/**************************************************************************//**
 * \file SCIconfig_Template.h
 * \author Roman Holderried
 *
 * \brief TEMPLATE SCI configuration — not compiled by this repo's build.
 *
 * SCI is designed to be platform independent: every file under Common/,
 * Master/, and Slave/ does `#include "SCIconfig.h"` unqualified, and
 * expects the *integrating project* to provide that header via its own
 * include path — this repo does not ship one canonical config.
 *
 * To use SCI in your own project:
 *   1. Copy this file into your project, e.g. `myproject/sci_config/SCIconfig.h`
 *      (the filename must be exactly `SCIconfig.h`, only the directory differs).
 *   2. Adjust the values below for your target/application (buffer sizes,
 *      var-struct size, EEPROM layout, transfer mode, etc.).
 *   3. Point your build's include path at that directory. With this repo's
 *      CMake build, that's `-DSCI_CONFIG_DIR=<dir containing your SCIconfig.h>`
 *      (see C/CMakeLists.txt).
 *
 * The C/Test GoogleTest suite in this repo has its own copy at
 * C/Test/config/SCIconfig.h, wired in automatically when BUILD_TESTS=ON.
 * Do not edit that file to experiment with values — copy this template
 * instead and point SCI_CONFIG_DIR at your copy.
 *
 * <b> History </b>
 * 	- 2022-01-13 - File creation (as SCIconfig.h)
 *  - 2022-03-17 - Port to C (Originally from SerialProtocol)
 *  - 2022-12-13 - Adapted code for unified master/slave repo structure.
 *  - 2026-09-10 - Repurposed as a template; the GoogleTest suite now owns
 *                 its own copy at C/Test/config/SCIconfig.h.
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

#define SIZE_OF_VAR_STRUCT  5
#define SIZE_OF_CMD_STRUCT  2
#define MAX_NUMBER_OF_EEPROM_VARS 10

// Mode configuration
#define SEND_MODE_BYTE_BY_BYTE
#define VALUE_MODE_HEX

// EEPROM configuration
#define EEPROM_ADDRESSTYPE  EEPROM_WORD_ADDRESSABLE
#define ADDRESS_OFFET       0

// SCI error offset (SCI currently defines 11 errors)
#define SCI_ERROR_OFFSET    0x100

// Number of request and response values
#define MAX_NUM_REQUEST_VALUES  10
#define MAX_NUM_RESPONSE_VALUES 10

#endif // _SCICONFIG_H_
