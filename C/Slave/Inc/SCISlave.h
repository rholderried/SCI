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
#include "DataLink.h"
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
    tSCI_VERSION    sVersion;
    PROTOCOL_STATE  e_state;    /*!< Actual protocol state. */

    uint8_t rxBuffer[RX_PACKET_LENGTH];   /*!< RX buffer space. */ 
    uint8_t txBuffer[TX_PACKET_LENGTH];   /*!< TX buffer space. */ 

    FIFO_BUF rxFIFO;  /*!< RX buffer management. */ 
    FIFO_BUF txFIFO;  /*!< TX buffer management. */

    DATALINK     datalink;
    SCI_COMMANDS sciCommands;   /*!< Commands variable structure. */
    VAR_ACCESS   varAccess;     /*!< Variable structure access. */

    // TX_CB       txCallback = nullptr;   /*!< Transmission callback function. */ 
}tSCI;

#define SCI_DEFAULT {   {SCI_VERSION_MAJOR, SCI_VERSION_MINOR, SCI_REVISION},\
                        ePROTOCOL_IDLE,\
                        {0},{0},\
                        FIFO_BUF_DEFAULT,\
                        FIFO_BUF_DEFAULT,\
                        DATALINK_DEFAULT,\
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
tSCI_VERSION SCI_GetVersion(void);

/** \brief Initialize protocol functionality.*/
tSCI_ERROR SCI_init(SCI_CALLBACKS callbacks, const VAR *p_varStruct, const COMMAND_CB *p_cmdStruct);

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
tSCI_ERROR SCI_GetVarFromStruct(int16_t i16_varNum, VAR* p_Var);

/** \brief Parses incoming message strings.
 *
 * @param *pui8_buf         Pointer to the buffer that holds the message.
 * @param ui8_stringSize    Length of the message string.
 * @returns COMMAND structure defining the command type and number
 */
tSCI_ERROR commandParser(uint8_t* pui8_buf, uint8_t ui8_stringSize, COMMAND *pCmd);

/** \brief Builds the response message string.
 * 
 * Takes the output from the command evaluation (RESPONSE type) and generates
 * an output message string from the data.
 * 
 * @param *pui8_buf     Pointer to the buffer where the string is going to be stored.
 * @param response      Structure holding the response information.
 * @returns size of the generated message string.
 */
uint8_t responseBuilder(uint8_t *pui8_buf, RESPONSE response);
#endif //_SCI_SLAVE_H_