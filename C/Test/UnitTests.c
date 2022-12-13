#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unity.h>
#include "SCISlave.h"
#include "SCIMaster.h"

/******************************************************************************
 * Defines
 *****************************************************************************/
#define NUMBER_OF_LOOPS 100

/******************************************************************************
 * External Globals
 *****************************************************************************/
extern tsSCIVAR varStruct;
extern COMMAND_CB cmdStruct;
extern tsSCI_SLAVE_CALLBACKS sSlaveTestCbs;
extern char cTxMsgBuf[];
extern char cRxMsgBuf[];

void setUp(void)
{}

void tearDown(void)
{}

void test_SCISlavePollVar1 (void)
{
    uint8_t ui8Msg[] = {0x02, '1', '?', 0x03};
    uint8_t ui8AnsExp[]= {0x02, '1', '?', 'F', '5' ,0x03};
    uint8_t ui8MsgIdx = 0;
    uint8_t j = 0;

    for(uint8_t i = 0; i < NUMBER_OF_LOOPS; i++)
    {

        if (j < sizeof(ui8Msg))
        {
            SCISlaveReceiveData(ui8Msg[j]);
            j++;
        }

        SCISlaveStatemachine();
    }
    TEST_ASSERT_EQUAL_CHAR_ARRAY(ui8AnsExp,cTxMsgBuf, sizeof(ui8AnsExp));
}

int main (void)
{
    UNITY_BEGIN();

    SCISlaveInit(sSlaveTestCbs, &varStruct, &cmdStruct);

    RUN_TEST(test_SCISlavePollVar1);

    
    return UNITY_END();
}