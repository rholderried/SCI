/**************************************************************************//**
 * \file DataLink.h
 * \author Roman Holderried
 *
 * \brief Data link layer functionality for the SCI protocol.
 *
 * <b> History </b>
 * 	- 2022-03-18 - File creation
 *****************************************************************************/
#ifndef _DATALINK_H_
#define _DATALINK_H_

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include <stdbool.h>

/******************************************************************************
 * Defines
 *****************************************************************************/
#define STX 0x02
#define ETX 0x03
/******************************************************************************
 * Type definitions
 *****************************************************************************/
typedef enum
{
    DATALINK_RSTATE_IDLE,
    DATALINK_RSTATE_BUSY,
    DATALINK_RSTATE_PENDING
}DATALINK_RECEIVE_STATE;

typedef enum
{
    DATALINK_TSTATE_IDLE,
    DATALINK_TSTATE_BUSY
}DATALINK_TRANSMIT_STATE;

typedef struct
{
    DATALINK_RECEIVE_STATE rState;
    DATALINK_TRANSMIT_STATE tState;

}DATALINK;

/******************************************************************************
 * Function declarations
 *****************************************************************************/
void receive(DATALINK *p_inst, uint8_t ui8_data);
bool transmit(DATALINK *p_inst, uint8_t *pui8_buf, uint8_t ui8_bufLen);


#endif // _DATALINK_H_