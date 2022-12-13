/**************************************************************************//**
 * \file VarAccess.c
 * \author Roman Holderried
 *
 * \brief Definitions for the VarAccess module.
 *
 * <b> History </b>
 * 	- 2022-01-18 - File creation
 *  - 2022-03-17 - Port to C (Originally from SerialProtocol)
 *  - 2022-12-13 - Adapted code for unified master/slave repo structure.
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "SCIVarAccess.h"
#include "SCIconfig.h"
#include "SCICommon.h"

/******************************************************************************
 * Global variables definitions
 *****************************************************************************/
extern const uint8_t ui8_byteLength[];

/******************************************************************************
 * Function definitions
 *****************************************************************************/

teSCI_SLAVE_ERROR InitVarstruct(tsVAR_ACCESS* pVarAccess)
{
    uint16_t    ui16_currentEEVarAddress = ADDRESS_OFFET;
    uint8_t     ui8_incrementor = 0;
    uint8_t     ui8_actualEEIdx = 0;
    teSCI_SLAVE_ERROR  eError = eSCI_ERROR_NONE;

    for (uint8_t i = 0; i < SIZE_OF_VAR_STRUCT; i++)
    {
        if (pVarAccess->pVarStruct[i].eVartype == eVARTYPE_EEPROM)
        {
            // Check if there is enough space in the address table
            if (ui8_actualEEIdx == MAX_NUMBER_OF_EEPROM_VARS)
                return eSCI_ERROR_EEPROM_PARTITION_TABLE_NOT_SUFFICIENT;

            pVarAccess->eepromPartitionTable[ui8_actualEEIdx].ui8Idx = i;
            pVarAccess->eepromPartitionTable[ui8_actualEEIdx].ui16Address = ui16_currentEEVarAddress;

            ui8_actualEEIdx++;
            //WriteEEPROMwithValueFromVarStruct(pVarAccess, i + 1);
            eError = ReadEEPROMValueIntoVarStruct(pVarAccess, i + 1);

            // We ignore EEPROM read errors and keep the default value of the RAM variable
            // To enable write Access, we establish the EEPROM partition table anyways.
            // if (eError != eSCI_ERROR_NONE)
            //     return eError;

            ui8_incrementor = ui8_byteLength[pVarAccess->pVarStruct[i].eDatatype] / EEPROM_ADDRESSTYPE;
            ui16_currentEEVarAddress += ui8_incrementor > 0 ? ui8_incrementor : 1;
        }
    }

    return eError;
}

//=============================================================================
teSCI_SLAVE_ERROR ReadValFromVarStruct(tsVAR_ACCESS* pVarAccess, int16_t i16VarNum, float *pfVal)
{
    int16_t i16_varIdx = i16VarNum - 1;
    //bool b_success;
    union
    {
        uint8_t     ui8Val;
        int8_t      i8_val;
        uint16_t    ui16Val;
        int16_t     i16Val;
        uint32_t    ui32Val;
        int32_t     i32Val;
        float       fVal;
    }ret;

    ret.ui32Val = 0;

    if (i16VarNum > 0 && i16VarNum <= SIZE_OF_VAR_STRUCT)
    {

        switch (pVarAccess->pVarStruct[i16_varIdx].eDatatype)
        {
            case eDTYPE_UINT8:
                ret.ui8Val = *(uint8_t*)(pVarAccess->pVarStruct[i16_varIdx].pVal);
                #ifndef VALUE_MODE_HEX
                *pfVal = (float)ret.ui8Val;
                #endif
                break;
            case eDTYPE_INT8:
                ret.i8_val = *(int8_t*)(pVarAccess->pVarStruct[i16_varIdx].pVal);
                #ifndef VALUE_MODE_HEX
                *pfVal = (float)ret.i8_val;
                #endif
                break;
            case eDTYPE_UINT16:
                ret.ui16Val = *(uint16_t*)(pVarAccess->pVarStruct[i16_varIdx].pVal);
                #ifndef VALUE_MODE_HEX
                *pfVal = (float)ret.ui16Val;
                #endif
                break;
            case eDTYPE_INT16:
                ret.i16Val = *(int16_t*)(pVarAccess->pVarStruct[i16_varIdx].pVal);
                #ifndef VALUE_MODE_HEX
                *pfVal = (float)ret.i16Val;
                #endif
                break;
            case eDTYPE_UINT32:
                ret.ui32Val = *(uint32_t*)(pVarAccess->pVarStruct[i16_varIdx].pVal);
                #ifndef VALUE_MODE_HEX
                *pfVal = (float)ret.ui32Val;
                #endif
                break;
            case eDTYPE_INT32:
                ret.i32Val = *(int32_t*)(pVarAccess->pVarStruct[i16_varIdx].pVal);
                #ifndef VALUE_MODE_HEX
                *pfVal = (float)ret.i32Val;
                #endif
                break;
            case eDTYPE_F32:
                ret.fVal   = *(float*)(pVarAccess->pVarStruct[i16_varIdx].pVal);
                #ifndef VALUE_MODE_HEX
                *pfVal = ret.fVal;
                #endif
                break;

            default:
                return eSCI_ERROR_UNKNOWN_DATATYPE;

        }

        #ifdef VALUE_MODE_HEX
        *pfVal = ret.fVal;
        #endif

        return eSCI_ERROR_NONE;
    }
    else
        return eSCI_ERROR_VAR_NUMBER_INVALID;
}

//=============================================================================
teSCI_SLAVE_ERROR WriteValToVarStruct(tsVAR_ACCESS* pVarAccess, int16_t i16VarNum, float fVal)
{
    int16_t i16_varIdx = i16VarNum - 1;
    //bool b_success;

    union
    {
        uint8_t     ui8Val;
        int8_t      i8_val;
        uint16_t    ui16Val;
        int16_t     i16Val;
        uint32_t    ui32Val;
        int32_t     i32Val;
        float       fVal;
    }in;

    in.fVal = fVal;

    if (i16VarNum > 0 && i16VarNum <= SIZE_OF_VAR_STRUCT)
    {

        switch (pVarAccess->pVarStruct[i16_varIdx].eDatatype)
        {
            case eDTYPE_UINT8:
                #ifdef VALUE_MODE_HEX
                *(uint8_t*)(pVarAccess->pVarStruct[i16_varIdx].pVal) = in.ui8Val;
                #else
                *(uint8_t*)(pVarAccess->pVarStruct[i16_varIdx].pVal) = (uint8_t)fVal;
                #endif
                break;
            case eDTYPE_INT8:
                #ifdef VALUE_MODE_HEX
                *(int8_t*)(pVarAccess->pVarStruct[i16_varIdx].pVal) = in.i8_val;
                #else
                *(int8_t*)(pVarAccess->pVarStruct[i16_varIdx].pVal) = (int8_t)fVal;
                #endif
                break;
            case eDTYPE_UINT16:
                #ifdef VALUE_MODE_HEX
                *(uint16_t*)(pVarAccess->pVarStruct[i16_varIdx].pVal) = in.ui16Val;
                #else
                *(uint16_t*)(pVarAccess->pVarStruct[i16_varIdx].pVal) = (uint16_t)fVal;
                #endif
                break;
            case eDTYPE_INT16:
                #ifdef VALUE_MODE_HEX
                *(int16_t*)(pVarAccess->pVarStruct[i16_varIdx].pVal) = in.i16Val;
                #else
                *(int16_t*)(pVarAccess->pVarStruct[i16_varIdx].pVal) = (int16_t)fVal;
                #endif
                break;
            case eDTYPE_UINT32:
                #ifdef VALUE_MODE_HEX
               *(uint32_t*)(pVarAccess->pVarStruct[i16_varIdx].pVal) = in.ui32Val;
               #else
               *(uint32_t*)(pVarAccess->pVarStruct[i16_varIdx].pVal) = (uint32_t)fVal;
               #endif
               break;
            case eDTYPE_INT32:
                #ifdef VALUE_MODE_HEX
                *(int32_t*)(pVarAccess->pVarStruct[i16_varIdx].pVal) = in.i32Val;
                #else
                *(int32_t*)(pVarAccess->pVarStruct[i16_varIdx].pVal) = (int32_t)fVal;
                #endif
                break;
            case eDTYPE_F32:
                *(float*)(pVarAccess->pVarStruct[i16_varIdx].pVal) = fVal;
                break;

            default:
                return eSCI_ERROR_UNKNOWN_DATATYPE;
        }

        return eSCI_ERROR_NONE;
    }
    else
        return eSCI_ERROR_VAR_NUMBER_INVALID;
}

//=============================================================================
teSCI_SLAVE_ERROR ReadEEPROMValueIntoVarStruct(tsVAR_ACCESS* pVarAccess, int16_t i16VarNum)
{
    bool        successIndicator = true;
    uint32_t    ui32_tmp = 0;
    uint8_t     ui8_numberOfIncs = 0;
    uint16_t    ui16_eepromAddress;

    union {
        uint8_t     ui8Val;
        int8_t      i8_val;
        uint16_t    ui16Val;
        int16_t     i16Val;
        uint32_t    ui32Val;
        int32_t     i32Val;
        float       fVal;
    } u_tmp;
    


    u_tmp.ui32Val = 0;
    if (pVarAccess->pVarStruct[i16VarNum - 1].eVartype == eVARTYPE_EEPROM && pVarAccess->cbReadEEPROM != NULL)
    {
        // Determine how many EEPROM reads have to be accomplished
        ui8_numberOfIncs = ui8_byteLength[pVarAccess->pVarStruct[i16VarNum - 1].eDatatype]/EEPROM_ADDRESSTYPE;
        ui8_numberOfIncs = ui8_numberOfIncs > 0 ? ui8_numberOfIncs : 1;


        // Look for the partition table index of the eeprom var
        ui16_eepromAddress = GetEEPROMAddress(pVarAccess, i16VarNum);

        if(ui16_eepromAddress == EEEPROM_ADDRESS_ILLEGAL)
            return eSCI_ERROR_EEPROM_ADDRESS_UNKNOWN;
;

        for (uint8_t i = 0; i < ui8_numberOfIncs; i++)
        {
            successIndicator &= pVarAccess->cbReadEEPROM(&ui32_tmp, ui16_eepromAddress + i);

            if (!successIndicator)
               return eSCI_ERROR_EEPROM_READOUT_FAILED;
            
            ui32_tmp <<= (i * EEPROM_ADDRESSTYPE * 8);
            u_tmp.ui32Val |= ui32_tmp;
            ui32_tmp = 0;
        }

        // Write the data structure with the read value
        switch(pVarAccess->pVarStruct[i16VarNum - 1].eDatatype)
        {
            case eDTYPE_UINT8:
                *(uint8_t*)(pVarAccess->pVarStruct[i16VarNum - 1].pVal) = u_tmp.ui8Val;
                break;
            case eDTYPE_INT8:
                *(int8_t*)(pVarAccess->pVarStruct[i16VarNum - 1].pVal) = u_tmp.i8_val;
                break;
            case eDTYPE_UINT16:
                *(uint16_t*)(pVarAccess->pVarStruct[i16VarNum - 1].pVal) = u_tmp.ui16Val;
                break;
            case eDTYPE_INT16:
                *(int16_t*)(pVarAccess->pVarStruct[i16VarNum - 1].pVal) = u_tmp.i16Val;
                break;
            case eDTYPE_UINT32:
                *(uint32_t*)(pVarAccess->pVarStruct[i16VarNum - 1].pVal) = u_tmp.ui32Val;
                break;
            case eDTYPE_INT32:
                *(int32_t*)(pVarAccess->pVarStruct[i16VarNum - 1].pVal) = u_tmp.i32Val;
                break;
            case eDTYPE_F32:
                *(float*)(pVarAccess->pVarStruct[i16VarNum - 1].pVal) = u_tmp.fVal;
                break;
            default:
                return eSCI_ERROR_UNKNOWN_DATATYPE;
        }
    }

    return eSCI_ERROR_NONE;
}

//=============================================================================
teSCI_SLAVE_ERROR WriteEEPROMwithValueFromVarStruct(tsVAR_ACCESS* pVarAccess, int16_t i16VarNum)
{
    bool        successIndicator = true;
    uint32_t    ui32_mask = 0, ui32_tmp = 0;
    uint8_t     ui8_numberOfIncs = 0;
    uint16_t    ui16_eepromAddress;

    union {
        uint8_t     ui8Val;
        int8_t      i8_val;
        uint16_t    ui16Val;
        int16_t     i16Val;
        uint32_t    ui32Val;
        int32_t     i32Val;
        float       fVal;
    } u_tmp;
    
    u_tmp.ui32Val = 0;

    if (pVarAccess->pVarStruct[i16VarNum - 1].eVartype == eVARTYPE_EEPROM && pVarAccess->cbWriteEEPROM != NULL)
    {
        // Look for the partition table index of the eeprom var
        ui16_eepromAddress = GetEEPROMAddress(pVarAccess, i16VarNum);

        if(ui16_eepromAddress == EEEPROM_ADDRESS_ILLEGAL)
            return eSCI_ERROR_EEPROM_ADDRESS_UNKNOWN;

        // Read data from the data structure
        switch(pVarAccess->pVarStruct[i16VarNum - 1].eDatatype)
        {
            case eDTYPE_UINT8:
                u_tmp.ui8Val = *(uint8_t*)(pVarAccess->pVarStruct[i16VarNum - 1].pVal);
                break;
            case eDTYPE_INT8:
                u_tmp.i8_val = *(int8_t*)(pVarAccess->pVarStruct[i16VarNum - 1].pVal);
                break;
            case eDTYPE_UINT16:
                u_tmp.ui16Val = *(uint16_t*)(pVarAccess->pVarStruct[i16VarNum - 1].pVal);
                break;
            case eDTYPE_INT16:
                u_tmp.i16Val = *(int16_t*)(pVarAccess->pVarStruct[i16VarNum - 1].pVal);
                break;
            case eDTYPE_UINT32:
                u_tmp.ui32Val = *(uint32_t*)(pVarAccess->pVarStruct[i16VarNum - 1].pVal);
                break;
            case eDTYPE_INT32:
                u_tmp.i32Val = *(int32_t*)(pVarAccess->pVarStruct[i16VarNum - 1].pVal);
                break;
            case eDTYPE_F32:
                u_tmp.fVal = *(float*)(pVarAccess->pVarStruct[i16VarNum - 1].pVal);
                break;
            default:
                return eSCI_ERROR_UNKNOWN_DATATYPE;
        }

        // Determine how many EEPROM reads have to be accomplished
        ui8_numberOfIncs = ui8_byteLength[pVarAccess->pVarStruct[i16VarNum - 1].eDatatype]/EEPROM_ADDRESSTYPE;
        ui8_numberOfIncs = ui8_numberOfIncs > 0 ? ui8_numberOfIncs : 1;

        // Generate the bit mask
        for (uint8_t i = EEPROM_ADDRESSTYPE; i > 0; i--)
        {
            ui32_mask |= 0xFF << ((i - 1) * 8);
        }

        // Write EEPROM 
        for (uint8_t i = ui8_numberOfIncs; i > 0; i--)
        {
            ui32_tmp = u_tmp.ui32Val >> (i - 1) * EEPROM_ADDRESSTYPE * 8;
            ui32_tmp &= ui32_mask;

            successIndicator &= pVarAccess->cbWriteEEPROM(ui32_tmp, ui16_eepromAddress + (i - 1));

            if (!successIndicator)
                return eSCI_ERROR_EEPROM_WRITE_FAILED;
        }
    }

    return eSCI_ERROR_NONE;
}

//=============================================================================
uint16_t GetEEPROMAddress(tsVAR_ACCESS* pVarAccess, int16_t i16VarNum)
{
    uint16_t ui16Address = EEEPROM_ADDRESS_ILLEGAL;
    uint8_t ui8Idx = 0;

    while (ui8Idx < MAX_NUMBER_OF_EEPROM_VARS)
    {
        if ((i16VarNum - 1) == pVarAccess->eepromPartitionTable[ui8Idx].ui8Idx)
        {
            ui16Address = pVarAccess->eepromPartitionTable[ui8Idx].ui16Address;
            break;
        }
        
        ui8Idx++;
    }

    return ui16Address;
}

//=============================================================================
teSCI_SLAVE_ERROR GetVar(tsVAR_ACCESS* pVarAccess, tsSCIVAR* pVar, int16_t i16VarNum)
{
    if ((i16VarNum > 0 && i16VarNum))    
    {
        *pVar = pVarAccess->pVarStruct[i16VarNum - 1];
        return eSCI_ERROR_NONE;
    }
    
    return eSCI_ERROR_VAR_NUMBER_INVALID;
}