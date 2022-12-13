/**************************************************************************//**
 * \file SCISlaveDataframe.h
 * \author Roman Holderried
 *
 * \brief SCI Dataframe parser declarations and definitions.
 *
 * <b> History </b>
 * 	- 2022-12-11 - File creation
 *  - 2022-12-13 - Adapted code for unified master/slave repo structure.s
 *****************************************************************************/

#ifndef _SCISLAVEDATAFRAME_H_
#define _SCISLAVEDATAFRAME_H_

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include <stdbool.h>

#include "SCICommon.h"

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
teSCI_SLAVE_ERROR SCISlaveRequestParser(uint8_t* pui8Buf, uint8_t ui8StringSize, tsREQUEST *psReq);

/** \brief Builds the response string.
 * 
 * Takes the output from the command evaluation (RESPONSE type) and generates
 * an output message string from the data.
 * 
 * @param *pui8_buf     Pointer to the buffer where the string is going to be stored.
 * @param response      Structure holding the response information.
 * @returns size of the generated message string.
 */
uint8_t SCISlaveResponseBuilder(uint8_t *pui8Buf, bool bFirstPacketNotSent, bool bOngoing, uint32_t *pui32DataIdx, tsRESPONSE *psRsp);

uint8_t _SCIFillBufferWithValues(uint8_t * pui8Buf, uint8_t ui8MaxSize, uint32_t *pui32DataIdx, tsRESPONSE *psRsp);


#endif //_SCISLAVEDATAFRAME_H_