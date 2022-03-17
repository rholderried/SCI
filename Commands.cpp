/**************************************************************************//**
 * \file Commands.cpp
 * \author Roman Holderried
 *
 * \brief Definitions for the SerialCommands module.
 *
 * <b> History </b>
 * 	- 2022-01-13 - File creation
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include "Commands.h"
#include "Variables.h"

/******************************************************************************
 * Class method definitions
 *****************************************************************************/
SerialCommands::SerialCommands(void){}

//=============================================================================
RESPONSE SerialCommands::executeCmd(COMMAND cmd)
{
    RESPONSE rsp = RESPONSE_DEFAULT;

    switch (cmd.e_cmdType)
    {

        case eCOMMAND_TYPE_GETVAR:
            {
                float f_val = 0.0;

                // If there is no readEEPROM callback or this is no EEPROM var, simply skip this step
                if (varAccess.p_varStruct[cmd.i16_num - 1].vartype == eVARTYPE_EEPROM)
                    // If conditions are met, EEPROM read must be successful.
                    if (!varAccess.readEEPROMValueIntoVarStruct(cmd.i16_num))
                        goto terminate;

                if (!varAccess.readValFromVarStruct(cmd.i16_num, &f_val))
                    goto terminate;
                
                rsp.i16_num     = cmd.i16_num;
                rsp.f_val       = f_val;
                rsp.e_cmdType   = cmd.e_cmdType;
                rsp.b_valid     = true;
                
            }
            break;

        case eCOMMAND_TYPE_SETVAR:
            {
                float f_formerVal, newVal = 0.0;
                bool writeSuccessful = false;
                bool readSuccessful = varAccess.readValFromVarStruct(cmd.i16_num, &f_formerVal);

                if (!readSuccessful)
                    goto terminate;

                // Read back actual value and write new one (write will only happen if read was successful)
                if (!varAccess.writeValToVarStruct(cmd.i16_num, cmd.f_valArr[0]))
                    goto terminate;

                // If the varStruct write operation was successful, trigger an EEPROM write (if callback present and variable is of type eVARTYPE_EEPROM)
                if (varAccess.p_varStruct[cmd.i16_num - 1].vartype == eVARTYPE_EEPROM)
                {
                    // If conditions are met, write must be successful.
                    if (!varAccess.writeEEPROMwithValueFromVarStruct(cmd.i16_num))
                    {
                        // If the EEPROM write was not successful, write back the old value to the var struct to keep it in sync with the EEPROM.
                        varAccess.writeValToVarStruct(cmd.i16_num, f_formerVal);
                        goto terminate;
                    }
                }

                // Everything was successful -> We call the action procedure
                if (varAccess.p_varStruct[cmd.i16_num - 1].ap != nullptr)
                    varAccess.p_varStruct[cmd.i16_num - 1].ap();

                varAccess.readValFromVarStruct(cmd.i16_num, &newVal);

                // If everything happens to be allright, create the response
                rsp.i16_num     = cmd.i16_num;
                rsp.f_val       = newVal;
                rsp.e_cmdType   = cmd.e_cmdType;
                rsp.b_valid     = true;
            }
            break;
        
        case eCOMMAND_TYPE_COMMAND:
            {
                bool cmdSuccess = false;

                // Check if a command structure has been passed
                if (p_cmdCBStruct != nullptr && cmd.i16_num > 0 && cmd.i16_num <= ui8_cmdCBStructLength)
                {
                    // TODO: Support for passing values to the command function
                    cmdSuccess = p_cmdCBStruct[cmd.i16_num - 1](cmd.f_valArr,cmd.ui8_valArrLen);
                }

                // Response is getting sent independently of command success
                rsp.i16_num = cmd.i16_num;
                rsp.e_cmdType = cmd.e_cmdType;
                rsp.f_val = cmdSuccess ? 0 : 1;
                rsp.b_valid = true;
                
            }
            break;

        default:
            break;
    }

    terminate: return rsp;
}