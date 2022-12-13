/**************************************************************************//**
 * \file SCISlaveTransfer.h
 * \author Roman Holderried
 *
 * \brief Transfer evaluation for the SCI Slave module.
 *
 * <b> History </b>
 * 	- 2022-01-13 - File creation
 *  - 2022-03-17 - Port to C (Originally from SerialProtocol)
 *  - 2022-12-11 - Adapted code for unified master/slave repo structure.
 *****************************************************************************/
#ifndef _SCISLAVETRANSFER_H_
#define _SCISLAVETRANSFER_H_

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "SCIVariables.h"
#include "SCIVarAccess.h"
#include "CommandStucture.h"
#include "SCITransferCommon.h"
#include "SCICommon.h"

/******************************************************************************
 * Defines
 *****************************************************************************/
// #define MAX_NUM_COMMAND_VALUES 10

/******************************************************************************
 * Type definitions
 *****************************************************************************/

// /** \brief Command type enumeration.*/
// typedef enum 
// {
//     eCOMMAND_TYPE_NONE          = 0,
//     eCOMMAND_TYPE_GETVAR        = 1,
//     eCOMMAND_TYPE_SETVAR        = 2,
//     eCOMMAND_TYPE_COMMAND       = 3,
//     eCOMMAND_TYPE_UPSTREAM      = 4,
//     eCOMMAND_TYPE_DOWNSTREAM    = 5
// }COMMAND_TYPE;


/** \brief Command structure declaration.*/
// typedef struct
// {
//     int16_t         i16_num;            /*!< ID Number.*/
//     uint8_t         ui8_valArrLen;      /*!< Length of the value Array.*/
//     union 
//     {
//         float           f_float;
//         uint32_t        ui32_hex;                         
//     }valArr[MAX_NUM_COMMAND_VALUES];    /*!< Pointer to the value array.*/
//     COMMAND_TYPE    e_cmdType;          /*!< Command Type.*/
// }COMMAND;

// #define COMMAND_DEFAULT         {0, 0, {{.ui32_hex = 0}}, eCOMMAND_TYPE_NONE}

/** \brief Response structure declaration.*/
// typedef struct
// {
//     // bool                b_valid;    /*!< Flags if response is valid and can be sent.*/
//     int16_t             i16_num;    /*!< ID Number. (Reflects Command ID number).*/
//     union 
//     {
//         float           f_float;    
//         uint32_t        ui32_hex;   
//     }val;                           /*!< Response value.*/
//     COMMAND_TYPE        e_cmdType;  /*!< Response type inherited from Command type.*/
//     COMMAND_CB_STATUS   e_cmdStatus;/*!< Status returned by the command callback.*/
//     PROCESS_INFO        info;       /*!< Additional command processing info.*/
// }RESPONSE;

// #define RESPONSE_DEFAULT         {/*false,*/ 0, {.ui32_hex = 0}, eCOMMAND_TYPE_NONE, eCOMMAND_STATUS_UNKNOWN, PROCESS_INFO_DEFAULT}

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
        }ui8ControlBits;
        
        uint8_t ui8ControlByte;
    };
    uint32_t    ui32DataIdx;
    tsRESPONSE  sRsp;
}tsRESPONSECONTROL;

#define tsRESPONSECONTROL_DEFAULTS {{.ui8ControlByte = 0}, 0, tsRESPONSE_DEFAULTS}

typedef struct
{
    tsRESPONSECONTROL sResponseControl;     

    const COMMAND_CB *pCmdCBStruct;         /*!< Command callback structure.*/

}tsSCI_TRANSFER_SLAVE;

#define tsSCI_TRANSFER_SLAVE_DEFAULTS    {tsRESPONSECONTROL_DEFAULTS, NULL}

/******************************************************************************
 * Function declarations
 *****************************************************************************/
void SCISlaveTransferInitiateResponse (tsSCI_TRANSFER_SLAVE *psTransfer, int16_t i16Num, teREQUEST_TYPE eReqType);

void SCISlaveTransferSetError (tsSCI_TRANSFER_SLAVE *psTransfer, uint16_t ui16Error);

/** \brief Executes the incoming request.
 *
 * @param cmd               Holds the command information from the parsed command.
 * @returns Response structure.
 */
teSCI_SLAVE_ERROR SCISlaveTransferProcessRequest(tsSCI_TRANSFER_SLAVE *psTransfer, tsVAR_ACCESS *pVarAccess, tsREQUEST sReq);

void SCISlaveTransferClearResponseControl(tsSCI_TRANSFER_SLAVE *psTransfer);

#endif //_SCISLAVETRANSFER_H_