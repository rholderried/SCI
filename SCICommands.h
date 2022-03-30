/**************************************************************************//**
 * \file SCICommands.h
 * \author Roman Holderried
 *
 * \brief Command evaluation and variable structure access.
 *
 * <b> History </b>
 * 	- 2022-01-13 - File creation
 *  - 2022-03-17 - Port to C (Originally from SerialProtocol)
 *****************************************************************************/
#ifndef _COMMANDS_H_
#define _COMMANDS_H_

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "Variables.h"
#include "CommandStucture.h"

/******************************************************************************
 * Defines
 *****************************************************************************/
#define MAX_NUM_COMMAND_VALUES 10

/******************************************************************************
 * Type definitions
 *****************************************************************************/

/** \brief Command type enumeration.*/
typedef enum 
{
    eCOMMAND_TYPE_NONE          = 0,
    eCOMMAND_TYPE_GETVAR        = 1,
    eCOMMAND_TYPE_SETVAR        = 2,
    eCOMMAND_TYPE_COMMAND       = 3,
    eCOMMAND_TYPE_UPSTREAM      = 4,
    eCOMMAND_TYPE_DOWNSTREAM    = 5
}COMMAND_TYPE;


/** \brief Command structure declaration.*/
typedef struct
{
    int16_t         i16_num;            /*!< ID Number.*/
    uint8_t         ui8_valArrLen;      /*!< Length of the value Array.*/
    union 
    {
        float           f_float;
        uint32_t        ui32_hex;                         
    }valArr[MAX_NUM_COMMAND_VALUES];    /*!< Pointer to the value array.*/
    COMMAND_TYPE    e_cmdType;          /*!< Command Type.*/
}COMMAND;

#define COMMAND_DEFAULT         {0, 0, {{.ui32_hex = 0}}, eCOMMAND_TYPE_NONE}

/** \brief Response structure declaration.*/
typedef struct
{
    bool                b_valid;    /*!< Flags if response is valid and can be sent.*/
    int16_t             i16_num;    /*!< ID Number. (Reflects Command ID number).*/
    union 
    {
        float           f_float;    
        uint32_t        ui32_hex;   
    }val;                           /*!< Response value.*/
    COMMAND_TYPE        e_cmdType;  /*!< Response type inherited from Command type.*/
    COMMAND_CB_STATUS   e_cmdStatus;/*!< Status returned by the command callback.*/
    PROCESS_INFO        info;       /*!< Additional command processing info.*/
}RESPONSE;

#define RESPONSE_DEFAULT         {false, 0, {.ui32_hex = 0}, eCOMMAND_TYPE_NONE, eCOMMAND_STATUS_UNKNOWN, PROCESS_INFO_DEFAULT}

typedef struct
{
    union
    {
        struct
        {
            uint8_t firstPacketNotSent  : 1;
            uint8_t ongoing             : 1;
            uint8_t upstream            : 1;
            uint8_t reserved            : 5;
        }ui8_controlBits;
        uint8_t ui8_controlByte;
    };
    uint16_t    ui16_typeIdx;
    uint32_t    ui32_byteIdx;
    RESPONSE    rsp;
}RESPONSECONTROL;

#define RESPONSECONTROL_DEFAULT {{.ui8_controlByte = 0}, 0, 0, RESPONSE_DEFAULT}

typedef struct
{
    RESPONSECONTROL responseControl;

    COMMAND_CB *p_cmdCBStruct;      /*!< Command callback structure.*/
}SCI_COMMANDS;

#define SCI_COMMANDS_DEFAULT    {RESPONSECONTROL_DEFAULT, NULL}

/******************************************************************************
 * Function declarations
 *****************************************************************************/
/** \brief Executes the incoming command.
 *
 * @param cmd               Holds the command information from the parsed command.
 * @returns Response structure.
 */
RESPONSE executeCmd(SCI_COMMANDS *p_sciCommands, VAR_ACCESS *p_varAccess, COMMAND cmd); 

uint8_t fillBufferWithValues(SCI_COMMANDS *p_inst, uint8_t * p_buf, uint8_t ui8_maxSize);

void clearResponseControl(SCI_COMMANDS *p_inst);

#endif //_COMMANDS_H_