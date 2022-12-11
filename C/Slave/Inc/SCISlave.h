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
#include "SCICommands.h"
#include "Variables.h"
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

typedef enum
{
    ePROTOCOL_ERROR         = -1,
    ePROTOCOL_IDLE          = 0,
    ePROTOCOL_RECEIVING     = 1,
    ePROTOCOL_EVALUATING    = 2,
    ePROTOCOL_SENDING       = 3,
}PROTOCOL_STATE;

typedef enum
{
    eDEBUG_ACTIVATION_NONE,
    eDEBUG_ACTIVATION_S1,
    eDEBUG_ACTIVATION_S2,
    eDEBUG_ACTIVATION_FINAL
}DEBUG_ACTIVATION_STATE;

/******************************************************************************
 * Structure Type definitions
 *****************************************************************************/


typedef struct
{
    tsSCI_VERSION   sVersion;
    PROTOCOL_STATE  e_state;    /*!< Actual protocol state. */

    uint8_t ui8RxBuffer[RX_PACKET_LENGTH];   /*!< RX buffer space. */ 
    uint8_t ui8TxBuffer[TX_PACKET_LENGTH];   /*!< TX buffer space. */ 

    tsFIFO_BUF      sRxFIFO;  /*!< RX buffer management. */ 
    tsFIFO_BUF      sTxFIFO;  /*!< TX buffer management. */

    tsDATALINK      sDatalink;
    SCI_COMMANDS    sciCommands;   /*!< Commands variable structure. */
    VAR_ACCESS      varAccess;     /*!< Variable structure access. */

    // TX_CB       txCallback = nullptr;   /*!< Transmission callback function. */ 
}tsSCI_SLAVE;

#define tsSCI_SLAVE_DEFAULTS {  {SCI_VERSION_MAJOR, SCI_VERSION_MINOR, SCI_REVISION},\
                                ePROTOCOL_IDLE,\
                                {0},{0},\
                                tsFIFO_BUF_DEFAULTS,\
                                tsFIFO_BUF_DEFAULTS,\
                                tsDATALINK_DEFAULTS,\
                                SCI_COMMANDS_DEFAULT,\
                                VAR_ACCESS_DEFAULT}

typedef struct
{
    WRITEEEPROM_CB writeEEPROMCallback;             /*!< Callback for writing data into the EEPROM. */
    READEEPROM_CB readEEPROMCallback;               /*!< Callback for reading data from the EEPROM. */
    BLOCKING_TX_CB transmitBlockingCallback;        /*!< Callback for the data transmission driver. Blocking. */
    NONBLOCKING_TX_CB transmitNonBlockingCallback;  /*!< Callback for the data transmission driver. Blocking. */
    GET_BUSY_STATE_CB getTxBusyStateCallback;       /*!< Callback for polling the busy state of the transmitter. */
}SCI_CALLBACKS;

#define SCI_CALLBACKS_DEFAULT {NULL}



/******************************************************************************
 * Function definitions
 *****************************************************************************/
/** \brief Returns the version struct of SCI.*/
tsSCI_VERSION SCI_GetVersion(void);

/** \brief Initialize protocol functionality.*/
teSCI_SLAVE_ERROR SCI_init(SCI_CALLBACKS callbacks, const VAR *p_varStruct, const COMMAND_CB *p_cmdStruct);

/** \brief Protocol state machine
 *
 * This function must be called in a cyclic manner for proper operation
 * of the serial protocol.
 */
void SCI_statemachine   (void);

/** \brief Receive method.
 *
 * @param ui8_data  Received data byte to be processed within the proocol.
 */
void SCI_receiveData    (uint8_t ui8_data);

/** \brief Get a single variable pointer from the variable structure.
 *
 * @param i16_varNum    Variable number of the desired variable.
 * @param p_Var         Pointer address to be set to the var struct variable.
 */
teSCI_SLAVE_ERROR SCI_GetVarFromStruct(int16_t i16_varNum, VAR* p_Var);
#endif //_SCI_SLAVE_H_