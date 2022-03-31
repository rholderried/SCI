/**************************************************************************//**
 * \file Helpers.h
 * \author Roman Holderried
 *
 * \brief Helper functions that can be generically used.
 *
 * <b> History </b>
 * 	- 2022-01-14 - File creation
 *****************************************************************************/
#ifndef _HELPERS_H_
#define _HELPERS_H_

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include <stdbool.h>

/******************************************************************************
 * Defines
 *****************************************************************************/
#define FTOA_MAX_AFTERPOINT 5
/******************************************************************************
 * Function declarations
 *****************************************************************************/

/** \brief Float to ASCII string conversion.
 *
 * This function features an automatic value range detection, a user defined
 * afterpoint length definition, as well es an optional value rounding functionality.
 * Trailing zeros are detected and ignored.
 * 
 * @param   *pui8_resBuf    Pointer to the buffer which will be holding the result.
 * @param   val             Float value to be converted.
 * @param   b_round         Value gets rounded according to the after point digits or not.
 * @returns Output string size in bytes.
 */
uint8_t ftoa (uint8_t *pui8_resBuf, float val, bool b_round);

bool strToHex (uint8_t *pui8_strBuf, uint32_t *pui32_val);

int8_t hexToStr (uint8_t *pui8_strBuf, uint32_t *pui32_val, uint8_t ui8_maxDataNibbles, bool shrinkZeros);

int8_t hexToStr2 (uint8_t *pui8_strBuf, uint8_t *pui8_val, uint8_t ui8_byteCount, bool shrinkZeros);


#endif // _HELPERS_H_