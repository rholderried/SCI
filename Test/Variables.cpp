#include <stdint.h>
#include "Variables.h"
#include "CommandStucture.h"

float testVar = 2.356;
uint8_t ui8_test = 245;
uint16_t ui16_test = 34534;
uint32_t i32_test = -87344381;
float   f_test = 2.4533;

VAR varStruct[] = { {&testVar, eVARTYPE_RAM, eDTYPE_F32,{0}},           // Number 1
                    {&f_test, eVARTYPE_EEPROM, eDTYPE_F32,{0}},         // Number 2
                    {&ui8_test, eVARTYPE_EEPROM, eDTYPE_UINT8,{0}},     // Number 3
                    {&ui16_test, eVARTYPE_EEPROM, eDTYPE_UINT16,{0}},     // Number 4
                    {&i32_test, eVARTYPE_EEPROM, eDTYPE_INT32,{0}}};    // Number 5


bool testCmd (float* pf_valArray, uint8_t ui8_valArrayLen)
{
    return true;
}

COMMAND_CB cmdStruct = {testCmd};
