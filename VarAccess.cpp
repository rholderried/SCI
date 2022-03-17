/**************************************************************************//**
 * \file VarAccess.cpp
 * \author Roman Holderried
 *
 * \brief Definitions for the VarAccess module.
 *
 * <b> History </b>
 * 	- 2022-01-18 - File creation
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "Variables.h"

/******************************************************************************
 * Global variables definitions
 *****************************************************************************/
const uint8_t ui8_byteLength[7] = {1,1,2,2,4,4,4};


/******************************************************************************
 * Class method definitions
 *****************************************************************************/
VarAccess::VarAccess() {}

//=============================================================================
bool VarAccess::initVarstruct()
{
    uint16_t    ui16_currentEEVarAddress = ADDRESS_OFFET;
    uint8_t     ui8_incrementor = 0;

    for (uint8_t i = 0; i < ui8_varStructLength; i++)
    {
        if (p_varStruct[i].vartype == eVARTYPE_EEPROM)
        {
            p_varStruct[i].runtime.ui8_byteLength = ui8_byteLength[p_varStruct[i].datatype];
            p_varStruct[i].runtime.ui16_eeAddress = ui16_currentEEVarAddress;

            readEEPROMValueIntoVarStruct(i + 1);

            ui8_incrementor = p_varStruct[i].runtime.ui8_byteLength / EEPROM_ADDRESSTYPE;
            ui16_currentEEVarAddress += ui8_incrementor > 0 ? ui8_incrementor : 1;
        }
    }

    return true;
}

//=============================================================================
bool VarAccess::readValFromVarStruct(int16_t i16_varNum, float *pf_val)
{
    int16_t i16_varIdx = i16_varNum - 1;
    bool b_success;

    if (i16_varNum > 0 && i16_varNum <= ui8_varStructLength)
    {

        switch (p_varStruct[i16_varIdx].datatype)
        {
            case eDTYPE_UINT8:
                *pf_val = static_cast<float>(*reinterpret_cast<uint8_t*>(p_varStruct[i16_varIdx].val));
                break;
            case eDTYPE_INT8:
                *pf_val = static_cast<float>(*reinterpret_cast<int8_t*>(p_varStruct[i16_varIdx].val));
                break;
            case eDTYPE_UINT16:
                *pf_val = static_cast<float>(*reinterpret_cast<uint16_t*>(p_varStruct[i16_varIdx].val));
                break;
            case eDTYPE_INT16:
                *pf_val = static_cast<float>(*reinterpret_cast<int16_t*>(p_varStruct[i16_varIdx].val));
                break;
            case eDTYPE_UINT32:
                *pf_val = static_cast<float>(*reinterpret_cast<uint32_t*>(p_varStruct[i16_varIdx].val));
                break;
            case eDTYPE_INT32:
                *pf_val = static_cast<float>(*reinterpret_cast<int32_t*>(p_varStruct[i16_varIdx].val));
                break;
            case eDTYPE_F32:
                *pf_val = *reinterpret_cast<float*>(p_varStruct[i16_varIdx].val);
                break;
        }

        b_success = true;
    }

    return b_success;
}

//=============================================================================
bool VarAccess::writeValToVarStruct(int16_t i16_varNum, float f_val)
{
    int16_t i16_varIdx = i16_varNum - 1;
    bool b_success;

    if (i16_varNum > 0 && i16_varNum <= ui8_varStructLength)
    {

        switch (p_varStruct[i16_varIdx].datatype)
        {
            case eDTYPE_UINT8:
                *reinterpret_cast<uint8_t*>(p_varStruct[i16_varIdx].val) = static_cast<uint8_t>(f_val);
                break;
            case eDTYPE_INT8:
                *reinterpret_cast<int8_t*>(p_varStruct[i16_varIdx].val) = static_cast<int8_t>(f_val);
                break;
            case eDTYPE_UINT16:
                *reinterpret_cast<uint16_t*>(p_varStruct[i16_varIdx].val) = static_cast<uint16_t>(f_val);
                break;
            case eDTYPE_INT16:
                *reinterpret_cast<int16_t*>(p_varStruct[i16_varIdx].val) = static_cast<int16_t>(f_val);
                break;
            case eDTYPE_UINT32:
               *reinterpret_cast<uint32_t*>(p_varStruct[i16_varIdx].val) = static_cast<uint32_t>(f_val);
               break;
            case eDTYPE_INT32:
                *reinterpret_cast<int32_t*>(p_varStruct[i16_varIdx].val) = static_cast<int32_t>(f_val);
                break;
            case eDTYPE_F32:
                *reinterpret_cast<float*>(p_varStruct[i16_varIdx].val) = f_val;
                break;
        }

        b_success = true;
    }

    return b_success;
}

//=============================================================================
bool VarAccess::readEEPROMValueIntoVarStruct(int16_t i16_varNum)
{
    bool        successIndicator = false;
    uint32_t    ui32_tmp = 0;
    uint8_t     ui8_numberOfIncs = 0;

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

    if (p_varStruct[i16_varNum - 1].runtime.ui8_byteLength > 0 && p_varStruct[i16_varNum - 1].vartype == eVARTYPE_EEPROM && readEEPROM_cb != nullptr)
    {
        // Determine how many EEPROM reads have to be accomplished
        ui8_numberOfIncs = p_varStruct[i16_varNum - 1].runtime.ui8_byteLength/EEPROM_ADDRESSTYPE;
        ui8_numberOfIncs = ui8_numberOfIncs > 0 ? ui8_numberOfIncs : 1;

        successIndicator = true;

        for (uint8_t i = 0; i < ui8_numberOfIncs; i++)
        {
            successIndicator &= readEEPROM_cb(&ui32_tmp, p_varStruct[i16_varNum - 1].runtime.ui16_eeAddress + i);

            if (!successIndicator)
                goto terminate;
            
            ui32_tmp <<= (i * EEPROM_ADDRESSTYPE * 8);
            u_tmp.ui32_val |= ui32_tmp;
            ui32_tmp = 0;
        }

        // Write the data structure with the read value
        switch(p_varStruct[i16_varNum - 1].datatype)
        {
            case eDTYPE_UINT8:
                *reinterpret_cast<uint8_t*>(p_varStruct[i16_varNum - 1].val) = u_tmp.ui8_val;
                break;
            case eDTYPE_INT8:
                *reinterpret_cast<int8_t*>(p_varStruct[i16_varNum - 1].val) = u_tmp.i8_val;
                break;
            case eDTYPE_UINT16:
                *reinterpret_cast<uint16_t*>(p_varStruct[i16_varNum - 1].val) = u_tmp.ui16_val;
                break;
            case eDTYPE_INT16:
                *reinterpret_cast<int16_t*>(p_varStruct[i16_varNum - 1].val) = u_tmp.i16_val;
                break;
            case eDTYPE_UINT32:
                *reinterpret_cast<uint32_t*>(p_varStruct[i16_varNum - 1].val) = u_tmp.ui32_val;
                break;
            case eDTYPE_INT32:
                *reinterpret_cast<int32_t*>(p_varStruct[i16_varNum - 1].val) = u_tmp.i32_val;
                break;
            case eDTYPE_F32:
                *reinterpret_cast<float*>(p_varStruct[i16_varNum - 1].val) = u_tmp.f_val;
                break;
        }
    }

    terminate: return successIndicator;
}

//=============================================================================
bool VarAccess::writeEEPROMwithValueFromVarStruct(int16_t i16_varNum)
{
    bool        successIndicator = false;
    uint32_t    ui32_mask = 0, ui32_tmp = 0;
    uint8_t     ui8_numberOfIncs = 0;

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

    if (p_varStruct[i16_varNum - 1].runtime.ui8_byteLength > 0 && p_varStruct[i16_varNum - 1].vartype == eVARTYPE_EEPROM && writeEEPROM_cb != nullptr)
    {
        // Read data from the data structure
        switch(p_varStruct[i16_varNum - 1].datatype)
        {
            case eDTYPE_UINT8:
                u_tmp.ui8_val = *reinterpret_cast<uint8_t*>(p_varStruct[i16_varNum - 1].val);
                break;
            case eDTYPE_INT8:
                u_tmp.i8_val = *reinterpret_cast<int8_t*>(p_varStruct[i16_varNum - 1].val);
                break;
            case eDTYPE_UINT16:
                u_tmp.ui16_val = *reinterpret_cast<uint16_t*>(p_varStruct[i16_varNum - 1].val);
                break;
            case eDTYPE_INT16:
                u_tmp.i16_val = *reinterpret_cast<int16_t*>(p_varStruct[i16_varNum - 1].val);
                break;
            case eDTYPE_UINT32:
                u_tmp.ui32_val = *reinterpret_cast<uint32_t*>(p_varStruct[i16_varNum - 1].val);
                break;
            case eDTYPE_INT32:
                u_tmp.i32_val = *reinterpret_cast<int32_t*>(p_varStruct[i16_varNum - 1].val);
                break;
            case eDTYPE_F32:
                u_tmp.f_val = *reinterpret_cast<float*>(p_varStruct[i16_varNum - 1].val);
                break;
        }

        // Determine how many EEPROM reads have to be accomplished
        ui8_numberOfIncs = p_varStruct[i16_varNum - 1].runtime.ui8_byteLength/EEPROM_ADDRESSTYPE;
        ui8_numberOfIncs = ui8_numberOfIncs > 0 ? ui8_numberOfIncs : 1;

        // Generate the bit mask
        for (uint8_t i = EEPROM_ADDRESSTYPE; i > 0; i--)
        {
            ui32_mask |= 0xFF << ((i - 1) * 8);
        }

        successIndicator = true;

        // Write EEPROM 
        for (uint8_t i = ui8_numberOfIncs; i > 0; i--)
        {
            ui32_tmp = u_tmp.ui32_val >> (i - 1) * EEPROM_ADDRESSTYPE * 8;
            ui32_tmp &= ui32_mask;

            successIndicator &= writeEEPROM_cb(ui32_tmp, p_varStruct[i16_varNum - 1].runtime.ui16_eeAddress + (i - 1));

            if (!successIndicator)
                break;
        }
    }

    return successIndicator;
}