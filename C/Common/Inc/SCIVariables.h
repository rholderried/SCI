/**************************************************************************//**
 * \file SCIVariables.h
 * \author Roman Holderried
 *
 * \brief SCI Variable declarations and type definitions.
 * 
 * <b> History </b>
 * 	- 2022-12-12 - File creation
 *****************************************************************************/

#ifndef _SCIVARIABLES_H_
#define _SCIVARIABLES_H_

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "SCIconfig.h"
#include "SCICommon.h"

/******************************************************************************
 * defines
 *****************************************************************************/

/******************************************************************************
 * Type definitions
 *****************************************************************************/

/** \brief Reflects the storage type of the linked variable.*/
typedef enum
{
    eVARTYPE_NONE       = 0,    /*!< Unknown storage type. Shouldn't be used.*/
    eVARTYPE_EEPROM     = 1,    /*!< EEPROM variable. Command execution will call EEPROM write/read functions on setVar/getVar.*/
    eVARTYPE_RAM        = 2     /*!< RAM variable. Just the linked variable will be accessed, no EEPROM read/write.*/
}teVARTYPE;

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
}teDTYPE;

#endif //_SCIVARIABLES_H_