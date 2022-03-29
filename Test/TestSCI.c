#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Variables.h"
#include "CommandStucture.h"
#include "SCI.h"

extern VAR varStruct [];
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


void dummyTxCb(uint8_t * pui8_buf, uint8_t ui8_size)
{
    char* buf = (char*)malloc(ui8_size + 1);
    memcpy(buf, pui8_buf, ui8_size);
    buf[ui8_size]='\0';

    printf(buf);
    free(buf);
}

void TestSCI_generateReads(void)
{
    // uint8_t ui8_msg[] = {2,'3','!','D','4',3};
    uint8_t ui8_msg1[] = {2,'1',':','D','4',3};
    // uint8_t ui8_msg1[] = {2,'1','>',3};
    //uint8_t ui8_msg[] = {2,'7','?','D','4',3};
    // uint8_t ui8_msg[] = {2,'7','D','4',3};

    uint8_t ui8_loopCount = 100;
    
    {
        SCI_CALLBACKS cbs = {NULL, NULL, dummyTxCb, NULL, NULL};
        SCI_init(cbs, varStruct, cmdStruct);
    }
    
    // Dummy receive
    for(uint8_t i = 0; i < sizeof(ui8_msg1); i++)
    {
        SCI_receiveData(ui8_msg1[i]);
    }

    for(uint8_t i = 0; i < ui8_loopCount; i++)
    {
        SCI_statemachine();
    }

    // Dummy receive
    for(uint8_t i = 0; i < sizeof(ui8_msg1); i++)
    {
        SCI_receiveData(ui8_msg1[i]);
    }

    for(uint8_t i = 0; i < ui8_loopCount; i++)
    {
        SCI_statemachine();
    }

    // Dummy receive
    for(uint8_t i = 0; i < sizeof(ui8_msg1); i++)
    {
        SCI_receiveData(ui8_msg1[i]);
    }

    for(uint8_t i = 0; i < ui8_loopCount; i++)
    {
        SCI_statemachine();
    }
    // Dummy receive
    for(uint8_t i = 0; i < sizeof(ui8_msg1); i++)
    {
        SCI_receiveData(ui8_msg1[i]);
    }

    for(uint8_t i = 0; i < ui8_loopCount; i++)
    {
        SCI_statemachine();
    }

    // Dummy receive
    for(uint8_t i = 0; i < sizeof(ui8_msg1); i++)
    {
        SCI_receiveData(ui8_msg1[i]);
    }

    for(uint8_t i = 0; i < ui8_loopCount; i++)
    {
        SCI_statemachine();
    }
    // Dummy receive
    for(uint8_t i = 0; i < sizeof(ui8_msg1); i++)
    {
        SCI_receiveData(ui8_msg1[i]);
    }

    for(uint8_t i = 0; i < ui8_loopCount; i++)
    {
        SCI_statemachine();
    }
    // Dummy receive
    for(uint8_t i = 0; i < sizeof(ui8_msg1); i++)
    {
        SCI_receiveData(ui8_msg1[i]);
    }

    for(uint8_t i = 0; i < ui8_loopCount; i++)
    {
        SCI_statemachine();
    }
    // Dummy receive
    for(uint8_t i = 0; i < sizeof(ui8_msg1); i++)
    {
        SCI_receiveData(ui8_msg1[i]);
    }

    for(uint8_t i = 0; i < ui8_loopCount; i++)
    {
        SCI_statemachine();
    }
    // Dummy receive
    for(uint8_t i = 0; i < sizeof(ui8_msg1); i++)
    {
        SCI_receiveData(ui8_msg1[i]);
    }

    for(uint8_t i = 0; i < ui8_loopCount; i++)
    {
        SCI_statemachine();
    }
    // Dummy receive
    for(uint8_t i = 0; i < sizeof(ui8_msg1); i++)
    {
        SCI_receiveData(ui8_msg1[i]);
    }

    for(uint8_t i = 0; i < ui8_loopCount; i++)
    {
        SCI_statemachine();
    }
    // Dummy receive
    for(uint8_t i = 0; i < sizeof(ui8_msg1); i++)
    {
        SCI_receiveData(ui8_msg1[i]);
    }

    for(uint8_t i = 0; i < ui8_loopCount; i++)
    {
        SCI_statemachine();
    }
    // Dummy receive
    for(uint8_t i = 0; i < sizeof(ui8_msg1); i++)
    {
        SCI_receiveData(ui8_msg1[i]);
    }

    for(uint8_t i = 0; i < ui8_loopCount; i++)
    {
        SCI_statemachine();
    }
}