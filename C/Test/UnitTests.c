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
{
    SCISlaveInit(sSlaveTestCbs, &varStruct, &cmdStruct);
}

void tearDown(void)
{}

void test_SCISlavePollVarUI8 (void)
{
    uint8_t ui8Msg[] = {0x02, '3', '?', 0x03};
    uint8_t ui8AnsExp[]= {0x02, '3', '?', 'A', 'C', 'K', ';', 'F', '5' ,0x03};
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

void test_SCISlavePollVarUI16 (void)
{
    uint8_t ui8Msg[] = {0x02, '4', '?', 0x03};
    uint8_t ui8AnsExp[]= {0x02, '4', '?', 'A', 'C', 'K', ';', '8', '6', 'E', '6' ,0x03};
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

void test_SCISlavePollVarI32 (void)
{
    uint8_t ui8Msg[] = {0x02, '5', '?', 0x03};
    uint8_t ui8AnsExp[]= {0x02, '5', '?', 'A', 'C', 'K', ';', 'F', 'A', 'C', 'B', '3', 'B', '0', '3', 0x03};
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

void test_SCISlavePollVarF32 (void)
{
    uint8_t ui8Msg[] = {0x02, '1', '?', 0x03};
    uint8_t ui8AnsExp[]= {0x02, '1', '?', 'A', 'C', 'K', ';', '4', '0', '1', '6', 'C', '8', 'B', '4', 0x03};
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

    RUN_TEST(test_SCISlavePollVarUI8);
    RUN_TEST(test_SCISlavePollVarUI16);
    RUN_TEST(test_SCISlavePollVarI32);
    RUN_TEST(test_SCISlavePollVarF32);

    
    return UNITY_END();
}