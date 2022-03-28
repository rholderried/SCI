#include <stdint.h>
#include <stddef.h>
#include "Variables.h"
#include "SCIconfig.h"
#include "CommandStucture.h"

float testVar = 2.356;
uint8_t ui8_test = 245;
uint16_t ui16_test = 34534;
uint32_t i32_test = -87344381;
float   f_test = 2.4533;

VAR varStruct[] = { {&testVar, eVARTYPE_RAM, eDTYPE_F32,NULL},           // Number 1
                    {&f_test, eVARTYPE_RAM, eDTYPE_F32,NULL},         // Number 2
                    {&ui8_test, eVARTYPE_RAM, eDTYPE_UINT8,NULL},     // Number 3
                    {&ui16_test, eVARTYPE_RAM, eDTYPE_UINT16,NULL},   // Number 4
                    {&i32_test, eVARTYPE_RAM, eDTYPE_INT32,NULL}};    // Number 5

uint8_t ui8_testBuffer[20] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 } ;


#ifdef VALUE_MODE_HEX
COMMAND_CB_STATUS testCmd (uint32_t* pui32_valArray, uint8_t ui8_valArrayLen, PROCESS_INFO *p_info)
{
    p_info->pui8_buf = ui8_testBuffer;
    p_info->ui32_datLen = 20;
    return eCOMMAND_STATUS_DATA_BYTES;
}
#else
bool testCmd (float* pf_valArray, uint8_t ui8_valArrayLen)
{
    return true;
}
#endif

COMMAND_CB cmdStruct = {testCmd};
