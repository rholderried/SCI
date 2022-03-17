/**************************************************************************//**
 * \file Buffer.h
 * \author Roman Holderried
 *
 * \brief Functions for controlling data traffic from and into a memory space.
 * 
 * Needs an externally defined buffer space (array), to which the address must
 * be passed to the constructor.
 *
 * <b> History </b>
 * 	- 2022-01-13 - File creation
 *  - 2022-03-17 - Port to C (Originally from SerialProtocol)
 *****************************************************************************/

#ifndef _BUFFER_H_
#define _BUFFER_H_

/******************************************************************************
 * Includes
 *****************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/******************************************************************************
 * Type definitions
 *****************************************************************************/
    
typedef struct
{
    uint8_t     *pui8_bufPtr;   /*!< Pointer to the external buffer. */
    int16_t     i16_bufIdx;     /*!< Actual buffer index (Last written index). */
    uint8_t     ui8_bufLen;     /*!< Length of the buffer array. */
    uint8_t     ui8_bufSpace;   /*!< Actual remaining buffer space. */
    bool        b_ovfl;         /*!< Overflow indicator. */
}FIFO_BUF;

#define FIFO_BUF_DEFAULT {NULL, 0, 0, 0, false}

/******************************************************************************
 * Function declaration
 *****************************************************************************/
/** \brief Puts one byte into the buffer
 *
 * @param data Data byte
 */
void putElem(FIFO_BUF* p_inst, uint8_t ui8_data);

/** \brief Buffer read operation
 *
 * This routine receives the address of a pointer variable, which gets moved
 * to the start of the buffer.
 * 
 * @param   **pui8_target Pointer address.
 * @returns Size of the stored data in bytes.
 */
uint8_t readBuf (FIFO_BUF* p_inst, uint8_t **pui8_target);

/** \brief Empties the buffer
 *
 * Sets the buffer index to -1 (start value) and the actual buffer Space
 * to the buffer length. The buffer contents hence are "invalidated".
 */
void flushBuf (FIFO_BUF* p_inst);

/** \brief Sets the input pointer to the next free buffer address
 * 
 * @param   **pui8_target Pointer address.
 * @returns True if there was free buffer space available, false otherwise
 */
bool getNextFreeBufSpace(FIFO_BUF* p_inst, uint8_t **pui8_target);

/** \brief Increases the buffer index.
 *
 * @param   ui8_size counts about which to increase the index.
 * @returns True if the operation was successful, false otherwise.
 */
bool increaseBufIdx(FIFO_BUF* p_inst, uint8_t ui8_size);

/** \brief Returns the actual (last written) buffer index.
 *
 * @returns actual buffer index.
 */
int16_t getActualIdx(FIFO_BUF* p_inst);

#endif