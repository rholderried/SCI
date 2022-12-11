/**************************************************************************//**
 * \file SCISlaveDataframe.h
 * \author Roman Holderried
 *
 * \brief SCI Dataframe parser declarations and definitions.
 *
 * <b> History </b>
 * 	- 2022-12-11 - File creation
 *****************************************************************************/

#ifndef _SCISLAVEDATAFRAME_H_
#define _SCISLAVEDATAFRAME_H_

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include <stdbool.h>

#include "SCICommon.h"
#include "SCITransfer.h"

/******************************************************************************
 * Defines
 *****************************************************************************/

/******************************************************************************
 * Type definitions
 *****************************************************************************/

/******************************************************************************
 * Function declarations
 *****************************************************************************/
/** \brief Parses incoming request strings on the SCI slave.
 *
 * @param *pui8_buf         Pointer to the buffer that holds the message.
 * @param ui8_stringSize    Length of the message string.
 * @returns COMMAND structure defining the command type and number
 */
teSCI_SLAVE_ERROR SCISlaveRequestParser(uint8_t* pui8_buf, uint8_t ui8_stringSize, COMMAND *pCmd);

/** \brief Builds the response string.
 * 
 * Takes the output from the command evaluation (RESPONSE type) and generates
 * an output message string from the data.
 * 
 * @param *pui8_buf     Pointer to the buffer where the string is going to be stored.
 * @param response      Structure holding the response information.
 * @returns size of the generated message string.
 */
uint8_t SCISlaveResponseBuilder(uint8_t *pui8_buf, RESPONSE response);


#endif //_SCISLAVEDATAFRAME_H_