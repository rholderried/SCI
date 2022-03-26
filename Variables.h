/**************************************************************************//**
 * \file Variables.h
 * \author Roman Holderried
 *
 * \brief Declarations for the variable structure.
 * 
 * Variale definitions take place in an external, user defined source file.
 * The variable structure must be based on the types defined by this header.
 *
 * <b> History </b>
 * 	- 2022-01-14 - File creation
 *  - 2022-03-17 - Port to C (Originally from SerialProtocol)
 *****************************************************************************/
#ifndef _VARIABLES_H_
#define _VARIABLES_H_

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include "SCIconfig.h"


/******************************************************************************
 * defines
 *****************************************************************************/
#define EEPROM_BYTE_ADRESSABLE      1
#define EEPROM_WORD_ADRESSABLE      2
#define EEPROM_LONG_ADRESSABLE      4

#define EEPROM_ADDRESSTYPE_DEFAULT    EEPROM_BYTE_ADRESSABLE

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

/** \brief Reflects the storage type of the linked variable.*/
typedef enum
{
    eVARTYPE_NONE,      /*!< Unknown storage type. Shouldn't be used.*/
    eVARTYPE_EEPROM,    /*!< EEPROM variable. Command execution will call EEPROM write/read functions on setVar/getVar.*/
    eVARTYPE_RAM        /*!< RAM variable. Just the linked variable will be accessed, no EEPROM read/write.*/
}TYPE;

/** \brief Reflects the data type of the linked variable.*/
typedef enum
{
    eDTYPE_UINT8  = 0,
    eDTYPE_INT8   = 1,
    eDTYPE_UINT16 = 2,
    eDTYPE_INT16  = 3,
    eDTYPE_UINT32 = 4,
    eDTYPE_INT32  = 5,
    eDTYPE_F32    = 6
}DTYPE;

/** \brief EEPROM write user callback.*/
typedef bool(*WRITEEEPROM_CB)(uint32_t ui32_val, uint16_t ui16_address);
/** \brief EEPROM read user callback.*/
typedef bool(*READEEPROM_CB)(uint32_t *ui32_val, uint16_t ui16_address);
/** \brief Action procedure declaration for a setVar operation.*/
typedef void(*ACTION_PROCEDURE)(void);

typedef struct
{
    uint8_t     ui8_idx;
    uint16_t    ui16_address;
}EEPROM_PARTITION_INFO;

#define EEPROM_PARTITION_INFO_DEFAULT {0,0}

/** \brief Variable struct member declaration.*/
typedef struct
{
    void    *val;       /*!< Pointer to the RAM variable the structure item links to.*/
    TYPE    vartype;    /*!< Storage type of the variable (RAM or EEPROM).*/
    DTYPE   datatype;   /*!< Datatype of the linked variable.*/

    ACTION_PROCEDURE ap;
}VAR;

#define VAR_DEFAULT {NULL, eVARTYPE_NONE, eDTYPE_UINT8, NULL}

typedef struct
{
    VAR     *p_varStruct;           /*!< Remembers the address of the variable structure.*/

    WRITEEEPROM_CB  writeEEPROM_cb;  /*!< Gets called in case of a EEPROM variable has been writen by command.*/
    READEEPROM_CB   readEEPROM_cb;   /*!< Gets called in case of a EEPROM variable has been read by command.*/

    EEPROM_PARTITION_INFO eepromPartitionTable[MAX_NUMBER_OF_EEPROM_VARS];
}VAR_ACCESS;

#define VAR_ACCESS_DEFAULT  {NULL, NULL, NULL, {EEPROM_PARTITION_INFO_DEFAULT}}

/******************************************************************************
 * Function declarations
 *****************************************************************************/
/** \brief Initializes the variable structure.
     * 
     * This function sets up the "partition table" for EEPROM accesses and reads writes the
     * values currently stored in the EEPROM to the variable structure.
     *
     * @returns Success indicator.
     */
bool initVarstruct(VAR_ACCESS* p_varAccess);

/** \brief Performs a variable read operation through the variable structure.
 *
 * @param i16_varNum    Variable number (deduced from ID number) to access.
 * @param *pf_val       Address to the variable to which the value gets written.
 * @returns Success indicator.
 */
bool readValFromVarStruct(VAR_ACCESS* p_varAccess, int16_t i16_varNum, float *pf_val);

/** \brief Performs a variable write operation through the variable structure.
 *
 * @param i16_varNum    Variable number (deduced from ID number) to access.
 * @param f_val         Value to write.
 * @returns Success indicator.
 */
bool writeValToVarStruct(VAR_ACCESS* p_varAccess, int16_t i16_varNum, float f_val);

/** \brief Reads a value from the EEPROM into the Variable structure.
 *
 * @param i16_varNum    Variable structure number.
 * @returns Success indicator.
 */
bool readEEPROMValueIntoVarStruct(VAR_ACCESS* p_varAccess, int16_t i16_varNum);

/** \brief Writes the EEPROM by the value read out from the variable structure.
 *
 * @param i16_varNum    Variable structure number.
 * @returns Success indicator.
 */
bool writeEEPROMwithValueFromVarStruct(VAR_ACCESS* p_varAccess, int16_t i16_varNum);


uint16_t getEEPROMAdress(VAR_ACCESS* p_varAccess, int16_t i16_varNum);

/******************************************************************************
 * Global variable declaration
 *****************************************************************************/
#endif //_VARIABLES_H_