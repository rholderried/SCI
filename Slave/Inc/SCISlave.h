/**************************************************************************//**
 * \file SCISlave.h
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
 *  - 2022-12-11 - Adapted code for unified master/slave repo structure.
 *****************************************************************************/

#ifndef _SCI_SLAVE_H_
#define _SCI_SLAVE_H_

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "SCISlaveTransfer.h"
#include "SCIVariables.h"
#include "SCIVarAccess.h"
#include "Buffer.h"
#include "CommandStucture.h"
#include "SCIDataLink.h"
#include "SCIconfig.h"

/******************************************************************************
 * Defines
 *****************************************************************************/


/******************************************************************************
 * Enum Type definitions
 *****************************************************************************/

/******************************************************************************
 * Structure Type definitions
 *****************************************************************************/

typedef struct
{
    tsSCI_VERSION       sVersion;
    tePROTOCOL_STATE    e_state;    /*!< Actual protocol state. */

    uint8_t             ui8RxBuffer[RX_PACKET_LENGTH];   /*!< RX buffer space. */ 
    uint8_t             ui8TxBuffer[TX_PACKET_LENGTH];   /*!< TX buffer space. */ 

    tsFIFO_BUF          sRxFIFO;  /*!< RX buffer management. */ 
    tsFIFO_BUF          sTxFIFO;  /*!< TX buffer management. */

    tsDATALINK              sDatalink;
    tsSCI_TRANSFER_SLAVE    sSciTransfer;   /*!< Commands variable structure. */
    tsVAR_ACCESS            sVarAccess;      /*!< Variable structure access. */
}tsSCI_SLAVE;

#define tsSCI_SLAVE_DEFAULTS {  {SCI_VERSION_MAJOR, SCI_VERSION_MINOR, SCI_REVISION},\
                                ePROTOCOL_IDLE,\
                                {0},{0},\
                                tsFIFO_BUF_DEFAULTS,\
                                tsFIFO_BUF_DEFAULTS,\
                                tsDATALINK_DEFAULTS,\
                                tsSCI_TRANSFER_SLAVE_DEFAULTS,\
                                tsVAR_ACCESS_DEFAULTS}

typedef struct
{
    WRITEEEPROM_CB cbWriteEEPROM;             /*!< Callback for writing data into the EEPROM. */
    READEEPROM_CB cbReadEEPROM;               /*!< Callback for reading data from the EEPROM. */
    BLOCKING_TX_CB cbTransmitBlocking;        /*!< Callback for the data transmission driver. Blocking. */
    NONBLOCKING_TX_CB cbTransmitNonBlocking;  /*!< Callback for the data transmission driver. Blocking. */
    GET_BUSY_STATE_CB cbGetTxBusyState;       /*!< Callback for polling the busy state of the transmitter. */
}tsSCI_SLAVE_CALLBACKS;

#define SCI_CALLBACKS_DEFAULT {NULL}



/******************************************************************************
 * Function definitions
 *****************************************************************************/
/** \brief Returns the version struct of SCI.*/
tsSCI_VERSION SCISlaveGetVersion(void);

/** \brief Initialize protocol functionality.*/
teSCI_SLAVE_ERROR SCISlaveInit(tsSCI_SLAVE_CALLBACKS sCallbacks, const tsSCIVAR *pVarStruct, const COMMAND_CB *pCmdStruct);

/** \brief Protocol state machine
 *
 * This function must be called in a cyclic manner for proper operation
 * of the serial protocol.
 */
void SCISlaveStatemachine (void);

/** \brief Receive method.
 *
 * @param ui8Data  Received data byte to be processed within the proocol.
 */
void SCISlaveReceiveData (uint8_t ui8Data);

/** \brief Get a single variable pointer from the variable structure.
 *
 * @param i16VarNum    Variable number of the desired variable.
 * @param pVar         Pointer address to be set to the var struct variable.
 */
teSCI_SLAVE_ERROR SCISlaveGetVarFromStruct(int16_t i16VarNum, tsSCIVAR* pVar);

#ifdef __cplusplus
}
#endif

#endif //_SCI_SLAVE_H_