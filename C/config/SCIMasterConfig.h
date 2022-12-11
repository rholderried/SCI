/**************************************************************************//**
 * \file SCIMasterConfig.h
 * \author Roman Holderried
 *
 * \brief Configuration file for the SCI Master.
 * 
 * <b> History </b>
 * 	- 2022-11-17 - File creation 
 *****************************************************************************/

#ifndef _SCIMASTERCONFIG_H_
#define _SCIMASTERCONFIG_H_

/******************************************************************************
 * Includes
 *****************************************************************************/

/******************************************************************************
 * Defines
 *****************************************************************************/
#define RX_PACKET_LENGTH    128
#define TX_PACKET_LENGTH    128

// Mode configuration
#define SEND_MODE_BYTE_BY_BYTE
#define VALUE_MODE_HEX

#endif // _SCIMASTERCONFIG_H_