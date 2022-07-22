/**************************************************************************//**
 * \file VarAccess.c
 * \author Roman Holderried
 *
 * \brief Definitions for the VarAccess module.
 *
 * <b> History </b>
 * 	- 2022-01-18 - File creation
 *  - 2022-03-17 - Port to C (Originally from SerialProtocol)
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "Variables.h"
#include "SCIconfig.h"
#include "SCICommon.h"

/******************************************************************************
 * Global variables definitions
 *****************************************************************************/
extern const uint8_t ui8_byteLength[];

/******************************************************************************
 * Function definitions
 *****************************************************************************/

tSCI_ERROR initVarstruct(VAR_ACCESS* p_varAccess)
{
    uint16_t    ui16_currentEEVarAddress = ADDRESS_OFFET;
    uint8_t     ui8_incrementor = 0;
    uint8_t     ui8_actualEEIdx = 0;
    tSCI_ERROR  eError;

    for (uint8_t i = 0; i < SIZE_OF_VAR_STRUCT; i++)
    {
        if (p_varAccess->p_varStruct[i].vartype == eVARTYPE_EEPROM)
        {
            // Check if there is enough space in the address table
            if (ui8_actualEEIdx == MAX_NUMBER_OF_EEPROM_VARS)
                return eSCI_ERROR_EEPROM_PARTITION_TABLE_NOT_SUFFICIENT;

            p_varAccess->eepromPartitionTable[ui8_actualEEIdx].ui8_idx = i;
            p_varAccess->eepromPartitionTable[ui8_actualEEIdx].ui16_address = ui16_currentEEVarAddress;

            ui8_actualEEIdx++;
            //writeEEPROMwithValueFromVarStruct(p_varAccess, i + 1);
            eError = readEEPROMValueIntoVarStruct(p_varAccess, i + 1);

            if (eError != eSCI_ERROR_NONE)
                return eError;

            ui8_incrementor = ui8_byteLength[p_varAccess->p_varStruct[i].datatype] / EEPROM_ADDRESSTYPE;
            ui16_currentEEVarAddress += ui8_incrementor > 0 ? ui8_incrementor : 1;
        }
    }

    return eSCI_ERROR_NONE;
}

//=============================================================================
tSCI_ERROR readValFromVarStruct(VAR_ACCESS* p_varAccess, int16_t i16_varNum, float *pf_val)
{
    int16_t i16_varIdx = i16_varNum - 1;
    //bool b_success;
    union
    {
        uint8_t     ui8_val;
        int8_t      i8_val;
        uint16_t    ui16_val;
        int16_t     i16_val;
        uint32_t    ui32_val;
        int32_t     i32_val;
        float       f_val;
    }ret;

    ret.ui32_val = 0;

    if (i16_varNum > 0 && i16_varNum <= SIZE_OF_VAR_STRUCT)
    {

        switch (p_varAccess->p_varStruct[i16_varIdx].datatype)
        {
            case eDTYPE_UINT8:
                ret.ui8_val = *(uint8_t*)(p_varAccess->p_varStruct[i16_varIdx].val);
                #ifndef VALUE_MODE_HEX
                *pf_val = (float)ret.ui8_val;
                #endif
                break;
            case eDTYPE_INT8:
                ret.i8_val = *(int8_t*)(p_varAccess->p_varStruct[i16_varIdx].val);
                #ifndef VALUE_MODE_HEX
                *pf_val = (float)ret.i8_val;
                #endif
                break;
            case eDTYPE_UINT16:
                ret.ui16_val = *(uint16_t*)(p_varAccess->p_varStruct[i16_varIdx].val);
                #ifndef VALUE_MODE_HEX
                *pf_val = (float)ret.ui16_val;
                #endif
                break;
            case eDTYPE_INT16:
                ret.i16_val = *(int16_t*)(p_varAccess->p_varStruct[i16_varIdx].val);
                #ifndef VALUE_MODE_HEX
                *pf_val = (float)ret.i16_val;
                #endif
                break;
            case eDTYPE_UINT32:
                ret.ui32_val = *(uint32_t*)(p_varAccess->p_varStruct[i16_varIdx].val);
                #ifndef VALUE_MODE_HEX
                *pf_val = (float)ret.ui32_val;
                #endif
                break;
            case eDTYPE_INT32:
                ret.i32_val = *(int32_t*)(p_varAccess->p_varStruct[i16_varIdx].val);
                #ifndef VALUE_MODE_HEX
                *pf_val = (float)ret.i32_val;
                #endif
                break;
            case eDTYPE_F32:
                ret.f_val   = *(float*)(p_varAccess->p_varStruct[i16_varIdx].val);
                #ifndef VALUE_MODE_HEX
                *pf_val = ret.f_val;
                #endif
                break;

            default:
                return eSCI_ERROR_UNKNOWN_DATATYPE;

        }

        #ifdef VALUE_MODE_HEX
        *pf_val = ret.f_val;
        #endif

        return eSCI_ERROR_NONE;
    }
    else
        return eSCI_ERROR_VAR_NUMBER_INVALID;
}

//=============================================================================
tSCI_ERROR writeValToVarStruct(VAR_ACCESS* p_varAccess, int16_t i16_varNum, float f_val)
{
    int16_t i16_varIdx = i16_varNum - 1;
    //bool b_success;

    union
    {
        uint8_t     ui8_val;
        int8_t      i8_val;
        uint16_t    ui16_val;
        int16_t     i16_val;
        uint32_t    ui32_val;
        int32_t     i32_val;
        float       f_val;
    }in;

    in.f_val = f_val;

    if (i16_varNum > 0 && i16_varNum <= SIZE_OF_VAR_STRUCT)
    {

        switch (p_varAccess->p_varStruct[i16_varIdx].datatype)
        {
            case eDTYPE_UINT8:
                #ifdef VALUE_MODE_HEX
                *(uint8_t*)(p_varAccess->p_varStruct[i16_varIdx].val) = in.ui8_val;
                #else
                *(uint8_t*)(p_varAccess->p_varStruct[i16_varIdx].val) = (uint8_t)f_val;
                #endif
                break;
            case eDTYPE_INT8:
                #ifdef VALUE_MODE_HEX
                *(int8_t*)(p_varAccess->p_varStruct[i16_varIdx].val) = in.i8_val;
                #else
                *(int8_t*)(p_varAccess->p_varStruct[i16_varIdx].val) = (int8_t)f_val;
                #endif
                break;
            case eDTYPE_UINT16:
                #ifdef VALUE_MODE_HEX
                *(uint16_t*)(p_varAccess->p_varStruct[i16_varIdx].val) = in.ui16_val;
                #else
                *(uint16_t*)(p_varAccess->p_varStruct[i16_varIdx].val) = (uint16_t)f_val;
                #endif
                break;
            case eDTYPE_INT16:
                #ifdef VALUE_MODE_HEX
                *(int16_t*)(p_varAccess->p_varStruct[i16_varIdx].val) = in.i16_val;
                #else
                *(int16_t*)(p_varAccess->p_varStruct[i16_varIdx].val) = (int16_t)f_val;
                #endif
                break;
            case eDTYPE_UINT32:
                #ifdef VALUE_MODE_HEX
               *(uint32_t*)(p_varAccess->p_varStruct[i16_varIdx].val) = in.ui32_val;
               #else
               *(uint32_t*)(p_varAccess->p_varStruct[i16_varIdx].val) = (uint32_t)f_val;
               #endif
               break;
            case eDTYPE_INT32:
                #ifdef VALUE_MODE_HEX
                *(int32_t*)(p_varAccess->p_varStruct[i16_varIdx].val) = in.i32_val;
                #else
                *(int32_t*)(p_varAccess->p_varStruct[i16_varIdx].val) = (int32_t)f_val;
                #endif
                break;
            case eDTYPE_F32:
                *(float*)(p_varAccess->p_varStruct[i16_varIdx].val) = f_val;
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
tSCI_ERROR readEEPROMValueIntoVarStruct(VAR_ACCESS* p_varAccess, int16_t i16_varNum)
{
    bool        successIndicator = true;
    uint32_t    ui32_tmp = 0;
    uint8_t     ui8_numberOfIncs = 0;
    uint16_t    ui16_eepromAddress;

    union {
        uint8_t     ui8_val;
        int8_t      i8_val;
        uint16_t    ui16_val;
        int16_t     i16_val;
        uint32_t    ui32_val;
        int32_t     i32_val;
        float       f_val;
    } u_tmp;
    


    u_tmp.ui32_val = 0;
    if (p_varAccess->p_varStruct[i16_varNum - 1].vartype == eVARTYPE_EEPROM && p_varAccess->readEEPROM_cb != NULL)
    {
        // Determine how many EEPROM reads have to be accomplished
        ui8_numberOfIncs = ui8_byteLength[p_varAccess->p_varStruct[i16_varNum - 1].datatype]/EEPROM_ADDRESSTYPE;
        ui8_numberOfIncs = ui8_numberOfIncs > 0 ? ui8_numberOfIncs : 1;


        // Look for the partition table index of the eeprom var
        ui16_eepromAddress = getEEPROMAddress(p_varAccess, i16_varNum);

        if(ui16_eepromAddress == EEEPROM_ADDRESS_ILLEGAL)
            return eSCI_ERROR_EEPROM_ADDRESS_UNKNOWN;
;

        for (uint8_t i = 0; i < ui8_numberOfIncs; i++)
        {
            successIndicator &= p_varAccess->readEEPROM_cb(&ui32_tmp, ui16_eepromAddress + i);

            if (!successIndicator)
               return eSCI_ERROR_EEPROM_READOUT_FAILED;
            
            ui32_tmp <<= (i * EEPROM_ADDRESSTYPE * 8);
            u_tmp.ui32_val |= ui32_tmp;
            ui32_tmp = 0;
        }

        // Write the data structure with the read value
        switch(p_varAccess->p_varStruct[i16_varNum - 1].datatype)
        {
            case eDTYPE_UINT8:
                *(uint8_t*)(p_varAccess->p_varStruct[i16_varNum - 1].val) = u_tmp.ui8_val;
                break;
            case eDTYPE_INT8:
                *(int8_t*)(p_varAccess->p_varStruct[i16_varNum - 1].val) = u_tmp.i8_val;
                break;
            case eDTYPE_UINT16:
                *(uint16_t*)(p_varAccess->p_varStruct[i16_varNum - 1].val) = u_tmp.ui16_val;
                break;
            case eDTYPE_INT16:
                *(int16_t*)(p_varAccess->p_varStruct[i16_varNum - 1].val) = u_tmp.i16_val;
                break;
            case eDTYPE_UINT32:
                *(uint32_t*)(p_varAccess->p_varStruct[i16_varNum - 1].val) = u_tmp.ui32_val;
                break;
            case eDTYPE_INT32:
                *(int32_t*)(p_varAccess->p_varStruct[i16_varNum - 1].val) = u_tmp.i32_val;
                break;
            case eDTYPE_F32:
                *(float*)(p_varAccess->p_varStruct[i16_varNum - 1].val) = u_tmp.f_val;
                break;
            default:
                return eSCI_ERROR_UNKNOWN_DATATYPE;
        }
    }

    return eSCI_ERROR_NONE;
}

//=============================================================================
tSCI_ERROR writeEEPROMwithValueFromVarStruct(VAR_ACCESS* p_varAccess, int16_t i16_varNum)
{
    bool        successIndicator = true;
    uint32_t    ui32_mask = 0, ui32_tmp = 0;
    uint8_t     ui8_numberOfIncs = 0;
    uint16_t    ui16_eepromAddress;

    union {
        uint8_t     ui8_val;
        int8_t      i8_val;
        uint16_t    ui16_val;
        int16_t     i16_val;
        uint32_t    ui32_val;
        int32_t     i32_val;
        float       f_val;
    } u_tmp;
    
    u_tmp.ui32_val = 0;

    if (p_varAccess->p_varStruct[i16_varNum - 1].vartype == eVARTYPE_EEPROM && p_varAccess->writeEEPROM_cb != NULL)
    {
        // Look for the partition table index of the eeprom var
        ui16_eepromAddress = getEEPROMAddress(p_varAccess, i16_varNum);

        if(ui16_eepromAddress == EEEPROM_ADDRESS_ILLEGAL)
            return eSCI_ERROR_EEPROM_ADDRESS_UNKNOWN;

        // Read data from the data structure
        switch(p_varAccess->p_varStruct[i16_varNum - 1].datatype)
        {
            case eDTYPE_UINT8:
                u_tmp.ui8_val = *(uint8_t*)(p_varAccess->p_varStruct[i16_varNum - 1].val);
                break;
            case eDTYPE_INT8:
                u_tmp.i8_val = *(int8_t*)(p_varAccess->p_varStruct[i16_varNum - 1].val);
                break;
            case eDTYPE_UINT16:
                u_tmp.ui16_val = *(uint16_t*)(p_varAccess->p_varStruct[i16_varNum - 1].val);
                break;
            case eDTYPE_INT16:
                u_tmp.i16_val = *(int16_t*)(p_varAccess->p_varStruct[i16_varNum - 1].val);
                break;
            case eDTYPE_UINT32:
                u_tmp.ui32_val = *(uint32_t*)(p_varAccess->p_varStruct[i16_varNum - 1].val);
                break;
            case eDTYPE_INT32:
                u_tmp.i32_val = *(int32_t*)(p_varAccess->p_varStruct[i16_varNum - 1].val);
                break;
            case eDTYPE_F32:
                u_tmp.f_val = *(float*)(p_varAccess->p_varStruct[i16_varNum - 1].val);
                break;
            default:
                return eSCI_ERROR_UNKNOWN_DATATYPE;
        }

        // Determine how many EEPROM reads have to be accomplished
        ui8_numberOfIncs = ui8_byteLength[p_varAccess->p_varStruct[i16_varNum - 1].datatype]/EEPROM_ADDRESSTYPE;
        ui8_numberOfIncs = ui8_numberOfIncs > 0 ? ui8_numberOfIncs : 1;

        // Generate the bit mask
        for (uint8_t i = EEPROM_ADDRESSTYPE; i > 0; i--)
        {
            ui32_mask |= 0xFF << ((i - 1) * 8);
        }

        // Write EEPROM 
        for (uint8_t i = ui8_numberOfIncs; i > 0; i--)
        {
            ui32_tmp = u_tmp.ui32_val >> (i - 1) * EEPROM_ADDRESSTYPE * 8;
            ui32_tmp &= ui32_mask;

            successIndicator &= p_varAccess->writeEEPROM_cb(ui32_tmp, ui16_eepromAddress + (i - 1));

            if (!successIndicator)
                return eSCI_ERROR_EEPROM_WRITE_FAILED;
        }
    }

    return eSCI_ERROR_NONE;
}

//=============================================================================
uint16_t getEEPROMAddress(VAR_ACCESS* p_varAccess, int16_t i16_varNum)
{
    uint16_t ui16_address = EEEPROM_ADDRESS_ILLEGAL;
    uint8_t ui8_idx = 0;

    while (ui8_idx < MAX_NUMBER_OF_EEPROM_VARS)
    {
        if ((i16_varNum - 1) == p_varAccess->eepromPartitionTable[ui8_idx].ui8_idx)
        {
            ui16_address = p_varAccess->eepromPartitionTable[ui8_idx].ui16_address;
            break;
        }
        
        ui8_idx++;
    }

    return ui16_address;
}

//=============================================================================
tSCI_ERROR getVarPtr(VAR_ACCESS* p_varAccess, VAR** p_Var, int16_t i16_varNum)
{
    if ((i16_varNum > 0 && i16_varNum))    
    {
        *p_Var = &p_varAccess->p_varStruct[i16_varNum - 1];
        return eSCI_ERROR_NONE;
    }
    
    return eSCI_ERROR_VAR_NUMBER_INVALID;
}