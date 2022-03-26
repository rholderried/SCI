/**************************************************************************//**
 * \file SCI.h
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
 *****************************************************************************/

#ifndef _SCI_H_
#define _SCI_H_

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

#define GETVAR_IDENTIFIER   '?'
#define SETVAR_IDENTIFIER   '!'
#define COMMAND_IDENTIFIER  ':'

/******************************************************************************
 * Type definitions
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
 * Type definitions
 *****************************************************************************/
typedef struct
{
    PROTOCOL_STATE  e_state;    /*!< Actual protocol state. */

    uint8_t rxBuffer[RX_BUFFER_LENGTH];   /*!< RX buffer space. */ 
    uint8_t txBuffer[TX_BUFFER_LENGTH];   /*!< TX buffer space. */ 

    FIFO_BUF rxFIFO;  /*!< RX buffer management. */ 
    FIFO_BUF txFIFO;  /*!< TX buffer management. */

    DATALINK     datalink;
    SCI_COMMANDS sciCommands;   /*!< Commands variable structure. */
    VAR_ACCESS   varAccess;     /*!< Variable structure access. */

    // TX_CB       txCallback = nullptr;   /*!< Transmission callback function. */ 
}SCI;

#define SCI_DEFAULT {   ePROTOCOL_IDLE,\
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



/******************************************************************************
 * Function definitions
 *****************************************************************************/
/** \brief Initialize protocol functionality.
 */
void SCI_init(SCI_CALLBACKS callbacks, VAR *p_varStruct, COMMAND_CB *p_cmdStruct);

/** \brief Take and save the function pointers to the user-defined callbacks.
 *
 * @param transmit_cb       Transmission callback function pointer.
 * @param readEEPROM_cb     EEPROM read callback function pointer.
 * @param writeEEPROM_cb    EEPROM write callback function pointer.
 */
// void setupCallbacks(TX_CB transmit_cb, READEEPROM_CB readEEPROM_cb, WRITEEEPROM_CB writeEEPROM_cb);

/** \brief Store the variable structure address and length.
 *
 * @param *p_varStruct      pointer to the variable structure.
 * @param ui8_structLen     Length of the variable structure.
 */
// void setupVariableStructure(VAR *p_varStruct, uint8_t ui8_structLen);

/** \brief Store the command structure address and length.
 *
 * @param *p_cmdStruct      pointer to the command structure.
 * @param ui8_structLen     Length of the variable structure.
 */
// void setupCommandStructure(COMMAND_CB *p_cmdStruct, uint8_t ui8_structLen);

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

/** \brief Parses incoming message strings.
 *
 * @param *pui8_buf         Pointer to the buffer that holds the message.
 * @param ui8_stringSize    Length of the message string.
 * @returns COMMAND structure defining the command type and number
 */
COMMAND commandParser(uint8_t *pui8_buf, uint8_t ui8_stringSize);

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
#endif //_SCI_H_