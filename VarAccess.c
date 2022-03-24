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

/******************************************************************************
 * Global variables definitions
 *****************************************************************************/
const uint8_t ui8_byteLength[7] = {1,1,2,2,4,4,4};

/******************************************************************************
 * Function definitions
 *****************************************************************************/

bool initVarstruct(VAR_ACCESS* p_varAccess)
{
    uint16_t    ui16_currentEEVarAddress = ADDRESS_OFFET;
    uint8_t     ui8_incrementor = 0;
    uint8_t     ui8_actualEEIdx = 0;

    for (uint8_t i = 0; i < p_varAccess->ui8_varStructLength; i++)
    {
        if (p_varAccess->p_varStruct[i].vartype == eVARTYPE_EEPROM)
        {
            // Check if there is enough space in the address table
            if (ui8_actualEEIdx == MAX_NUMBER_OF_EEPROM_VARS)
                return false;

            p_varAccess->eepromPartitionTable[ui8_actualEEIdx].ui8_idx = i;
            p_varAccess->eepromPartitionTable[ui8_actualEEIdx].ui16_address = ui16_currentEEVarAddress;

            ui8_actualEEIdx++;

            readEEPROMValueIntoVarStruct(p_varAccess, i + 1);

            ui8_incrementor = ui8_byteLength[p_varAccess->p_varStruct[i].datatype] / EEPROM_ADDRESSTYPE;
            ui16_currentEEVarAddress += ui8_incrementor > 0 ? ui8_incrementor : 1;
        }
    }

    return true;
}

//=============================================================================
bool readValFromVarStruct(VAR_ACCESS* p_varAccess, int16_t i16_varNum, float *pf_val)
{
    int16_t i16_varIdx = i16_varNum - 1;
    bool b_success;

    if (i16_varNum > 0 && i16_varNum <= p_varAccess->ui8_varStructLength)
    {

        switch (p_varAccess->p_varStruct[i16_varIdx].datatype)
        {
            case eDTYPE_UINT8:
                *pf_val = (float)(*(uint8_t*)(p_varAccess->p_varStruct[i16_varIdx].val));
                break;
            case eDTYPE_INT8:
                *pf_val = (float)(*(int8_t*)(p_varAccess->p_varStruct[i16_varIdx].val));
                break;
            case eDTYPE_UINT16:
                *pf_val = (float)(*(uint16_t*)(p_varAccess->p_varStruct[i16_varIdx].val));
                break;
            case eDTYPE_INT16:
                *pf_val = (float)(*(int16_t*)(p_varAccess->p_varStruct[i16_varIdx].val));
                break;
            case eDTYPE_UINT32:
                *pf_val = (float)(*(uint32_t*)(p_varAccess->p_varStruct[i16_varIdx].val));
                break;
            case eDTYPE_INT32:
                *pf_val = (float)(*(int32_t*)(p_varAccess->p_varStruct[i16_varIdx].val));
                break;
            case eDTYPE_F32:
                *pf_val = *(float*)(p_varAccess->p_varStruct[i16_varIdx].val);
                break;
        }

        b_success = true;
    }

    return b_success;
}

//=============================================================================
bool writeValToVarStruct(VAR_ACCESS* p_varAccess, int16_t i16_varNum, float f_val)
{
    int16_t i16_varIdx = i16_varNum - 1;
    bool b_success;

    if (i16_varNum > 0 && i16_varNum <= p_varAccess->ui8_varStructLength)
    {

        switch (p_varAccess->p_varStruct[i16_varIdx].datatype)
        {
            case eDTYPE_UINT8:
                *(uint8_t*)(p_varAccess->p_varStruct[i16_varIdx].val) = (uint8_t)(f_val);
                break;
            case eDTYPE_INT8:
                *(int8_t*)(p_varAccess->p_varStruct[i16_varIdx].val) = (int8_t)(f_val);
                break;
            case eDTYPE_UINT16:
                *(uint16_t*)(p_varAccess->p_varStruct[i16_varIdx].val) = (uint16_t)(f_val);
                break;
            case eDTYPE_INT16:
                *(int16_t*)(p_varAccess->p_varStruct[i16_varIdx].val) = (int16_t)(f_val);
                break;
            case eDTYPE_UINT32:
               *(uint32_t*)(p_varAccess->p_varStruct[i16_varIdx].val) = (uint32_t)(f_val);
               break;
            case eDTYPE_INT32:
                *(int32_t*)(p_varAccess->p_varStruct[i16_varIdx].val) = (int32_t)(f_val);
                break;
            case eDTYPE_F32:
                *(float*)(p_varAccess->p_varStruct[i16_varIdx].val) = f_val;
                break;
        }

        b_success = true;
    }

    return b_success;
}

//=============================================================================
bool readEEPROMValueIntoVarStruct(VAR_ACCESS* p_varAccess, int16_t i16_varNum)
{
    bool        successIndicator = false;
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
        ui16_eepromAddress = getEEPROMAdress(p_varAccess, i16_varNum);

        successIndicator = ~(ui16_eepromAddress == EEEPROM_ADDRESS_ILLEGAL);

        if (!successIndicator)
            goto terminate;

        for (uint8_t i = 0; i < ui8_numberOfIncs; i++)
        {
            successIndicator &= readEEPROM_cb(&ui32_tmp, ui16_eepromAddress + i);

            if (!successIndicator)
                goto terminate;
            
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
        }
    }

    terminate: return successIndicator;
}

//=============================================================================
bool writeEEPROMwithValueFromVarStruct(VAR_ACCESS* p_varAccess, int16_t i16_varNum)
{
    bool        successIndicator = false;
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
        ui16_eepromAddress = getEEPROMAdress(p_varAccess, i16_varNum);

        successIndicator = ~(ui16_eepromAddress == EEEPROM_ADDRESS_ILLEGAL);

        if (!successIndicator)
            goto terminate;

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
        }

        // Determine how many EEPROM reads have to be accomplished
        ui8_numberOfIncs = ui8_byteLength[p_varAccess->p_varStruct[i16_varNum - 1].datatype]/EEPROM_ADDRESSTYPE;
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

            successIndicator &= writeEEPROM_cb(ui16_eepromAddress + (i - 1));

            if (!successIndicator)
                break;
        }
    }

    terminate: return successIndicator;
}

//=============================================================================
uint16_t getEEPROMAdress(VAR_ACCESS* p_varAccess, int16_t i16_varNum)
{
    uint16_t ui16_address = EEEPROM_ADDRESS_ILLEGAL;
    uint8_t ui8_idx = 0;

    while (ui8_idx < MAX_NUMBER_OF_EEPROM_VARS)
    {
        if ((i16_varNum - 1) == p_varAccess->eepromPartitionTable[ui8_idx].ui8_idx)
            ui16_address = p_varAccess->eepromPartitionTable[ui8_idx].ui16_address;
            break;
        
        ui8_idx++;
    }

    return ui16_address;
}