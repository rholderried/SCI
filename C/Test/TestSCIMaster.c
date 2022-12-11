#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SCIMaster.h"
#include "TestSCIMaster.h"


uint8_t TestSetVarCB(uint8_t ui8Ack, int16_t i16Num, uint16_t ui16ErrNum)
{
    printf("SetVar Transfer finished!\n");
    printf("Acknowledge: %d, Number %d, Error %d\n", ui8Ack, i16Num, ui16ErrNum);

    return 0;
}

uint8_t TestGetVarCB(uint8_t ui8Ack, int16_t i16Num, uint32_t ui32Data, uint16_t ui16ErrNum)
{
    printf("GetVar Transfer finished!\n");
    printf("Acknowledge: %d, Number %d, Error %d\n\n", ui8Ack, i16Num, ui16ErrNum);
    printf("Result:\n");
    printf("%\n", ui32Data);

    return 0;
}

uint8_t TestCommandCB(uint8_t ui8Ack, int16_t i16Num, uint32_t *pui32Data, uint8_t ui8DataCnt, uint16_t ui16ErrNum)
{
    printf("Command Transfer finished!\n");
    printf("Acknowledge: %d, Number %d, Error %d\n\n", ui8Ack, i16Num, ui16ErrNum);
    printf("Results:\n");

    for (uint8_t i = 0; i < ui8DataCnt;i++)
        printf("R%d: %x\n",(i+1), *pui32Data++);

    return 0;
}

uint8_t TestUpstreamCB(int16_t i16Num, uint8_t *pui8Data, uint32_t ui32DataCnt)
{
    printf("Upstream Transfer finished!\n");
    printf("Number %d\n\n", i16Num);
    printf("Result:\n");

    for (uint32_t i = 0; i < ui32DataCnt;i++)
        printf("%x", *pui8Data++);

    return 0;
}

void dummySlave(void)
{
    static uint8_t ui8Sched = 0;
    // uint8_t ui8_msg1[] = {2,'1','?','A','C','K',';','4',3};
    //uint8_t ui8_msg1[] = {2,'F','F',':','D','A','T',';','2',';','F','F',',','3',3};
    uint8_t ui8_msg1[] = {2,'F','F',':','U','P','S',';','2','0','0',3};
    uint8_t ui8UpsMsg[TX_PACKET_LENGTH + 2];
    
    if (ui8Sched == 0)
    {
        SCIReceive(ui8_msg1, sizeof(ui8_msg1));
        ui8Sched++;
    }
    else
    {
        ui8UpsMsg[0] = 2;

        for (uint8_t i = 0; i < TX_PACKET_LENGTH; i++)
        {
            ui8UpsMsg[i + 1] = ui8Sched;
        }
        ui8UpsMsg[TX_PACKET_LENGTH + 1] = 3;

        ui8Sched++;
        SCIReceive(ui8UpsMsg, sizeof(ui8UpsMsg));
    }
}

void dummyTxCb(uint8_t * pui8_buf, uint8_t ui8_size)
{
    char* buf = (char*)malloc(ui8_size + 1);
    memcpy(buf, pui8_buf, ui8_size);
    buf[ui8_size]='\0';

    // print("\n");
    // print("Tx Message:\n");
    printf(buf);
    // print("\n\n");
    // print("Calling Slave...\n");
    // dummySlave(buf, ui8_size);

    free(buf);
}



void main(void)
{
    tsSCI_MASTER_CALLBACKS sCbs = { .BlockingTxExternalCB = dummyTxCb, 
                                    .CommandExternalCB = TestCommandCB,
                                    .GetVarExternalCB = TestGetVarCB,
                                    .SetVarExternalCB = TestSetVarCB,
                                    .UpstreamExternalCB = TestUpstreamCB};

    tuREQUESTVALUE uVal[3] = {{.ui32_hex = 3}, {.f_float = 2.0}, {.ui32_hex = 255}};
    // Init Master
    SCIMasterInit(sCbs);

    // Get variable 1
    //SCIRequestGetVar(1);
    SCIRequestCommand(255, uVal, 3);

    for (uint16_t i = 0; i<256; i++)
        SCIMasterSM();
    
    dummySlave();

    for (uint8_t i = 0; i<255; i++)
        SCIMasterSM();
    
    dummySlave();

    for (uint8_t i = 0; i<255; i++)
        SCIMasterSM();
    
    dummySlave();

    for (uint8_t i = 0; i<255; i++)
        SCIMasterSM();
    
    dummySlave();

    for (uint8_t i = 0; i<255; i++)
        SCIMasterSM();
    
    dummySlave();

    for (uint8_t i = 0; i<255; i++)
        SCIMasterSM();
    
    dummySlave();

    for (uint8_t i = 0; i<255; i++)
        SCIMasterSM();
}