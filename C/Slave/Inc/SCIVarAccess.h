/**************************************************************************//**
 * \file SCIVarAccess.h
 * \author Roman Holderried
 *
 * \brief Variable access declarations and definitions.
 * 
 * <b> History </b>
 * 	- 2022-01-14 - File creation
 *  - 2022-03-17 - Port to C (Originally from SerialProtocol)
 *  - 2022-12-12 - Adapted code for unified master/slave repo structure.
 *****************************************************************************/
#ifndef _SCIVARACCESS_H_
#define _SCIVARACCESS_H_

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "SCIconfig.h"
#include "SCICommon.h"
#include "SCIVariables.h"

/******************************************************************************
 * defines
 *****************************************************************************/
#define EEPROM_ADDRESSTYPE_DEFAULT    EEPROM_BYTE_ADDRESSABLE
#ifndef EEPROM_ADDRESSTYPE
#define EEPROM_ADDRESSTYPE EEPROM_ADDRESSTYPE_DEFAULT
#endif

#define ADDRESS_OFFSET_DEFAULT  0
#ifndef ADDRESS_OFFET
#define ADDRESS_OFFET ADDRESS_OFFSET_DEFAULT
#endif

#define EEEPROM_ADDRESS_ILLEGAL 0xFFFF

/******************************************************************************
 * Type definitions
 *****************************************************************************/

/** \brief EEPROM write user callback.*/
typedef bool(*WRITEEEPROM_CB)(uint32_t ui32Val, uint16_t ui16Address);
/** \brief EEPROM read user callback.*/
typedef bool(*READEEPROM_CB)(uint32_t *ui32Val, uint16_t ui16Address);
/** \brief Action procedure declaration for a setVar operation.*/
typedef void(*ACTION_PROCEDURE)(void);

typedef struct
{
    uint8_t     ui8Idx;
    uint16_t    ui16Address;
}tsEEPROM_PARTITION_INFO;

#define tsEEPROM_PARTITION_INFO_DEFAULTS {0,0}

/** \brief Variable struct member declaration.*/
typedef struct
{
    void        *pVal;       /*!< Pointer to the RAM variable the structure item links to.*/
    teVARTYPE   eVartype;    /*!< Storage type of the variable (RAM or EEPROM).*/
    teDTYPE     eDatatype;   /*!< Datatype of the linked variable.*/

    ACTION_PROCEDURE ap;
}tsSCIVAR;

#define VAR_DEFAULT {NULL, eVARTYPE_NONE, eDTYPE_UINT8, NULL}

typedef struct
{
    const tsSCIVAR  *pVarStruct;    /*!< Remembers the address of the variable structure.*/

    WRITEEEPROM_CB  cbWriteEEPROM;  /*!< Gets called in case of a EEPROM variable has been writen by command.*/
    READEEPROM_CB   cbReadEEPROM;   /*!< Gets called in case of a EEPROM variable has been read by command.*/

    tsEEPROM_PARTITION_INFO eepromPartitionTable[MAX_NUMBER_OF_EEPROM_VARS];
}tsVAR_ACCESS;

#define tsVAR_ACCESS_DEFAULTS  {NULL, NULL, NULL, {tsEEPROM_PARTITION_INFO_DEFAULTS}}

/******************************************************************************
 * Function declarations
 *****************************************************************************/
/** \brief Initializes the variable structure.
     * 
     * This function sets up the "partition table" for EEPROM accesses and reads writes the
     * values currently stored in the EEPROM to the variable structure.
     *
     * @param   pVarAccess module data pointer
     * @returns Success indicator.
     */
teSCI_SLAVE_ERROR InitVarstruct(tsVAR_ACCESS* pVarAccess);

/** \brief Performs a variable read operation through the variable structure.
 *
 * @param   pVarAccess  module data pointer
 * @param   i16VarNum   Variable number (deduced from ID number) to access.
 * @param * pfVal       Address to the variable to which the value gets written.
 * @returns Success indicator.
 */
teSCI_SLAVE_ERROR ReadValFromVarStruct(tsVAR_ACCESS* pVarAccess, int16_t i16VarNum, float *pfVal);

/** \brief Performs a variable write operation through the variable structure.
 *
 * @param   pVarAccess  module data pointer
 * @param i16VarNum     Variable number (deduced from ID number) to access.
 * @param fVal          Value to write.
 * @returns Success indicator.
 */
teSCI_SLAVE_ERROR WriteValToVarStruct(tsVAR_ACCESS* pVarAccess, int16_t i16VarNum, float fVal);

/** \brief Reads a value from the EEPROM into the Variable structure.
 *
 * @param   pVarAccess  module data pointer
 * @param   i16VarNum   Variable structure number.
 * @returns Success indicator.
 */
teSCI_SLAVE_ERROR ReadEEPROMValueIntoVarStruct(tsVAR_ACCESS* pVarAccess, int16_t i16VarNum);

/** \brief Writes the EEPROM by the value read out from the variable structure.
 *
 * @param   pVarAccess  module data pointer
 * @param   i16VarNum   Variable structure number.
 * @returns Success indicator.
 */
teSCI_SLAVE_ERROR WriteEEPROMwithValueFromVarStruct(tsVAR_ACCESS* pVarAccess, int16_t i16VarNum);


uint16_t GetEEPROMAddress(tsVAR_ACCESS* pVarAccess, int16_t i16VarNum);

teSCI_SLAVE_ERROR GetVar(tsVAR_ACCESS* pVarAccess, tsSCIVAR* pVar, int16_t i16VarNum);

/******************************************************************************
 * Global variable declaration
 *****************************************************************************/
#endif //_SCIVARACCESS_H_