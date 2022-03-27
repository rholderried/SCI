/**************************************************************************//**
 * \file SCICommands.c
 * \author Roman Holderried
 *
 * \brief Definitions for the SCI commands module.
 *
 * <b> History </b>
 * 	- 2022-01-13 - File creation
 *  - 2022-03-17 - Port to C (Originally from SerialProtocol)
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include <stddef.h>
#include "SCICommands.h"
#include "Variables.h"
#include "SCIconfig.h"

/******************************************************************************
 * Function definitions
 *****************************************************************************/

//=============================================================================
RESPONSE executeCmd(SCI_COMMANDS *p_sciCommands, VAR_ACCESS *p_varAccess, COMMAND cmd)
{
    RESPONSE rsp = RESPONSE_DEFAULT;

    switch (cmd.e_cmdType)
    {

        case eCOMMAND_TYPE_GETVAR:
            {
                float f_val = 0.0;

                // If there is no readEEPROM callback or this is no EEPROM var, simply skip this step
                if (p_varAccess->p_varStruct[cmd.i16_num - 1].vartype == eVARTYPE_EEPROM)
                    // If conditions are met, EEPROM read must be successful.
                    if (!readEEPROMValueIntoVarStruct(p_varAccess, cmd.i16_num))
                        goto terminate;

                if (!readValFromVarStruct(p_varAccess, cmd.i16_num, &f_val))
                    goto terminate;
                
                rsp.i16_num     = cmd.i16_num;
                rsp.val.f_float = f_val;
                rsp.e_cmdType   = cmd.e_cmdType;
                rsp.b_valid     = true;
                
            }
            break;

        case eCOMMAND_TYPE_SETVAR:
            {
                float f_formerVal, newVal = 0.0;
                //bool writeSuccessful = false;
                bool readSuccessful = readValFromVarStruct(p_varAccess, cmd.i16_num, &f_formerVal);

                if (!readSuccessful)
                    goto terminate;

                // Read back actual value and write new one (write will only happen if read was successful)
                if (!writeValToVarStruct(p_varAccess, cmd.i16_num, cmd.valArr[0].f_float))
                    goto terminate;

                // If the varStruct write operation was successful, trigger an EEPROM write (if callback present and variable is of type eVARTYPE_EEPROM)
                if (p_varAccess->p_varStruct[cmd.i16_num - 1].vartype == eVARTYPE_EEPROM)
                {
                    // If conditions are met, write must be successful.
                    if (!writeEEPROMwithValueFromVarStruct(p_varAccess, cmd.i16_num))
                    {
                        // If the EEPROM write was not successful, write back the old value to the var struct to keep it in sync with the EEPROM.
                        writeValToVarStruct(p_varAccess, cmd.i16_num, f_formerVal);
                        goto terminate;
                    }
                }

                // Everything was successful -> We call the action procedure
                if (p_varAccess->p_varStruct[cmd.i16_num - 1].ap != NULL)
                    p_varAccess->p_varStruct[cmd.i16_num - 1].ap();

                readValFromVarStruct(p_varAccess, cmd.i16_num, &newVal);

                // If everything happens to be allright, create the response
                rsp.i16_num     = cmd.i16_num;
                rsp.val.f_float = newVal;
                rsp.e_cmdType   = cmd.e_cmdType;
                rsp.b_valid     = true;
            }
            break;
        
        case eCOMMAND_TYPE_COMMAND:
            {
                bool cmdSuccess = false;

                // Check if a command structure has been passed
                if (p_sciCommands->p_cmdCBStruct != NULL && cmd.i16_num > 0 && cmd.i16_num <= SIZE_OF_CMD_STRUCT)
                {
                    // TODO: Support for passing values to the command function
                    #ifdef VALUE_MODE_HEX
                    cmdSuccess = p_sciCommands->p_cmdCBStruct[cmd.i16_num - 1](&cmd.valArr[0].ui32_hex,cmd.ui8_valArrLen);
                    #else
                    cmdSuccess = p_sciCommands->p_cmdCBStruct[cmd.i16_num - 1](&cmd.valArr[0].f_float,cmd.ui8_valArrLen);
                    #endif
                }

                // Response is getting sent independently of command success
                rsp.i16_num         = cmd.i16_num;
                rsp.e_cmdType       = cmd.e_cmdType;
                rsp.val.ui32_hex    = cmdSuccess ? 0 : 1;
                rsp.b_valid         = true;
            }
            break;

        default:
            break;
    }

    terminate: return rsp;
}