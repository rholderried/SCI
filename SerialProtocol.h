/**************************************************************************//**
 * \file SerialProtocol.h
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
 *****************************************************************************/

#ifndef _SERIALPROTOCOL_H_
#define _SERIALPROTOCOL_H_

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "Commands.h"
#include "Variables.h"
#include "Buffer.h"
#include "CommandStucture.h"

/******************************************************************************
 * Defines
 *****************************************************************************/
#define TRANSMIT_BUFFER_LENGTH  64
// #define NUMBER_OF_CONTROL_BYTES 2
#define TXRX_BUFFER_LENGTH      TRANSMIT_BUFFER_LENGTH

#define STX 0x02
#define ETX 0x03

#define GETVAR_IDENTIFIER   '?'
#define SETVAR_IDENTIFIER   '!'
#define COMMAND_IDENTIFIER  ':'

#define DEBUG_FUNCTIONS

/******************************************************************************
 * Type definitions
 *****************************************************************************/
typedef bool(*TX_CB)(uint8_t*, uint8_t);
typedef void(*DBG_FCN_CB)(void);

typedef enum
{
    ePROTOCOL_IDLE,
    ePROTOCOL_RECEIVING,
    ePROTOCOL_EVALUATING,
    ePROTOCOL_SENDING,
    ePROTOCOL_DEBUG
}PROTOCOL_STATE;

#define PROTOCOL_STATE_DEFAULT ePROTOCOL_IDLE

typedef enum
{
    eDEBUG_ACTIVATION_NONE,
    eDEBUG_ACTIVATION_S1,
    eDEBUG_ACTIVATION_S2,
    eDEBUG_ACTIVATION_FINAL
}DEBUG_ACTIVATION_STATE;


/******************************************************************************
 * Class declarations
 *****************************************************************************/
class SerialProtocol
{
    public:

    struct {
        PROTOCOL_STATE  e_state;    /*!< Actual protocol state. */
        bool            b_error;    /*!< Error flag. */
        bool            b_sent;     /*!< Data sent flag. */ 

        #ifdef DEBUG_FUNCTIONS
        DEBUG_ACTIVATION_STATE e_dbgActState;       /*!< State of the received symbols for debug function activation. */   
        DBG_FCN_CB  debugFcnArray[10] = {nullptr};  /*!< Function pointer array to the debug command functions. */
        #endif
    } control;

    uint8_t txRxBuffer[TXRX_BUFFER_LENGTH] = {0};   /*!< Combined TX/RX buffer. */ 


    Buffer rxBuffer = Buffer(TXRX_BUFFER_LENGTH, this->txRxBuffer); /*!< RX buffer handler. */ 
    Buffer txBuffer = Buffer(TXRX_BUFFER_LENGTH, this->txRxBuffer); /*!< TX buffer handler. */ 

    TX_CB       txCallback = nullptr;   /*!< Transmission callback function. */ 

    // Public methods

    /** \brief SerialProtocol c'tor
     *
     * @param transmitCallback  Callback for transmitting data
     */
    SerialProtocol();

    /** \brief Take and save the function pointers to the user-defined callbacks.
     *
     * @param transmit_cb       Transmission callback function pointer.
     * @param readEEPROM_cb     EEPROM read callback function pointer.
     * @param writeEEPROM_cb    EEPROM write callback function pointer.
     */
    void setupCallbacks(TX_CB transmit_cb, READEEPROM_CB readEEPROM_cb, WRITEEEPROM_CB writeEEPROM_cb);

    /** \brief Store the variable structure address and length.
     *
     * @param *p_varStruct      pointer to the variable structure.
     * @param ui8_structLen     Length of the variable structure.
     */
    void setupVariableStructure(VAR *p_varStruct, uint8_t ui8_structLen);

    /** \brief Store the command structure address and length.
     *
     * @param *p_cmdStruct      pointer to the command structure.
     * @param ui8_structLen     Length of the variable structure.
     */
    void setupCommandStructure(COMMAND_CB *p_cmdStruct, uint8_t ui8_structLen);

    /** \brief Protocol state machine
     *
     * This function must be called in a cyclic manner for proper operation
     * of the serial protocol.
     */
    void statemachine   (void);

    /** \brief Receive method.
     *
     * @param ui8_data  Received data byte to be processed within the proocol.
     */
    void receive        (uint8_t ui8_data);

    private:
    
    SerialCommands cmdModule = SerialCommands();    /*!< Command execution module. */

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

};

#endif //_SERIALPROTOCOL_H_