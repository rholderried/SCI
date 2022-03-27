#include <stdio.h>
#include "SCI.h"
#include "Helpers.h"
#include "Variables.h"


extern VAR varStruct[];
extern COMMAND_CB cmdStruct[];
uint8_t EESimulationBuffer[20];

bool testWriteEE (uint32_t ui32_val, uint16_t ui16_address)
{
    uint8_t val = ui32_val & 0xFF;
    EESimulationBuffer[ui16_address] = val;
    return true;
}

bool testReadEE (uint32_t *ui32_val, uint16_t ui16_address)
{
    uint8_t val = EESimulationBuffer[ui16_address];
    *ui32_val = val;
    return true;
}


bool testTxCb(uint8_t* p_buf, uint8_t size)
{
    p_buf[size] = '\0';
    printf((const char*)p_buf);

    return true;
}

int main (void)
{
    printf("Hello World!");
    return 0;
}