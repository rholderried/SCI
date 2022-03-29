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
DTYPE e_testDtypes[]={eDTYPE_F32};


#ifdef VALUE_MODE_HEX
COMMAND_CB_STATUS testCmd (uint32_t* pui32_valArray, uint8_t ui8_valArrayLen, PROCESS_INFO *p_info)
{
    uint16_t * pui16_buf = ui8_testBuffer;
    for (uint8_t i = 0; i < 10; i++)
        pui16_buf[i] = i;

    p_info->pui8_buf = ui8_testBuffer;
    p_info->ui32_datLen = 10;
    p_info->eDataFormat = e_testDtypes;
    p_info->ui16_dataFormatLen= 1;
    return eCOMMAND_STATUS_DATA_VALUES;
}
#else
COMMAND_CB_STATUS testCmd (float* pf_valArray, uint8_t ui8_valArrayLen, PROCESS_INFO *p_info)
{
    float * f_buf = ui8_testBuffer;
    for (uint8_t i = 0; i < 5; i++)
        f_buf[i] = (float)i + 0.5;

    p_info->pui8_buf = ui8_testBuffer;
    p_info->ui32_datLen = 5;
    p_info->eDataFormat = e_testDtypes;
    p_info->ui16_dataFormatLen= 1;
    return eCOMMAND_STATUS_DATA_BYTES;
}
#endif

COMMAND_CB cmdStruct = {testCmd};
