#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "SCIVariables.h"
#include "SCIconfig.h"
#include "CommandStucture.h"
#include "Helpers.h"

float testVar = 2.356;
uint8_t ui8_test = 245;
uint16_t ui16_test = 34534;
uint32_t i32_test = -87344381;
float   f_test = 2.4533;

// EEPROM-backed variable (address 6). Exercises the EEPROM read/write path
// in VarAccess.c via SlaveReadEEROM/SlaveWriteEEROM (TestCallbacks.c).
uint16_t ui16_eeprom_test = 1000;

// RAM variable with an action procedure attached (address 7). Used to
// assert the action procedure fires exactly once per successful SetVar.
uint8_t ui8_ap_test = 0;
uint32_t g_apCallCount = 0;

static void IncrementApCounter(void)
{
    g_apCallCount++;
}

tsSCIVAR varStruct[] = {{&testVar, eVARTYPE_RAM, eDTYPE_F32, NULL},                // Number 1
                        {&f_test, eVARTYPE_RAM, eDTYPE_F32, NULL},                 // Number 2
                        {&ui8_test, eVARTYPE_RAM, eDTYPE_UINT8, NULL},             // Number 3
                        {&ui16_test, eVARTYPE_RAM, eDTYPE_UINT16, NULL},           // Number 4
                        {&i32_test, eVARTYPE_RAM, eDTYPE_INT32, NULL},             // Number 5
                        {&ui16_eeprom_test, eVARTYPE_EEPROM, eDTYPE_UINT16, NULL}, // Number 6
                        {&ui8_ap_test, eVARTYPE_RAM, eDTYPE_UINT8, IncrementApCounter}};   // Number 7

uint8_t ui8_testBuffer[20] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 };
uint32_t ui32_testBuffer[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

// Large (> TX_PACKET_LENGTH) upstream payload used to force multi-packet
// upstream chunking in the round-trip tests. Filled deterministically
// (byte i holds i % 256) so tests can verify exact reassembled content.
#define LARGE_UPSTREAM_BUFFER_SIZE 300
static uint8_t ui8_largeUpstreamBuffer[LARGE_UPSTREAM_BUFFER_SIZE];

// Captures the arguments a command callback last received, for test assertions.
uint32_t g_lastCmdArgs[MAX_NUM_REQUEST_VALUES];
uint8_t  g_lastCmdArgCount = 0;

#ifdef VALUE_MODE_HEX

// Command 1: triggers a small (single-packet) upstream transfer of the
// well-known 20-byte ui8_testBuffer.
teREQUEST_ACKNOWLEDGE testCmdUpstream (uint32_t* pui32_valArray, uint8_t ui8_valArrayLen, tsTRANSFER_DATA *psData)
{
    psData->pui8UpStreamBuf = ui8_testBuffer;
    psData->ui32DatLen      = sizeof(ui8_testBuffer);
    // Statically allocated buffer -> not dynamic, don't let the transfer
    // control free() it.
    psData->ui8InfoFlagBits.upstreamBufDynamic = 0;

    return eREQUEST_ACK_STATUS_SUCCESS_UPSTREAM;
}

// Command 2: returns a small set of known numeric values inline (DATA
// response), no upstream/stream-mode switch involved.
teREQUEST_ACKNOWLEDGE testCmdData (uint32_t* pui32_valArray, uint8_t ui8_valArrayLen, tsTRANSFER_DATA *psData)
{
    psData->puRespVals[0].ui32_hex = 0x11111111;
    psData->puRespVals[1].ui32_hex = 0x22222222;
    psData->puRespVals[2].ui32_hex = 0x33333333;
    psData->ui32DatLen = 3;

    return eREQUEST_ACK_STATUS_SUCCESS_DATA;
}

// Command 3: plain success, no result data at all.
teREQUEST_ACKNOWLEDGE testCmdPlain (uint32_t* pui32_valArray, uint8_t ui8_valArrayLen, tsTRANSFER_DATA *psData)
{
    psData->ui32DatLen = 0;

    return eREQUEST_ACK_STATUS_SUCCESS;
}

// Command 4: triggers a large (> TX_PACKET_LENGTH) upstream transfer to
// force the multi-packet chunking path on both Master and Slave.
teREQUEST_ACKNOWLEDGE testCmdUpstreamLarge (uint32_t* pui32_valArray, uint8_t ui8_valArrayLen, tsTRANSFER_DATA *psData)
{
    for (uint16_t i = 0; i < LARGE_UPSTREAM_BUFFER_SIZE; i++)
        ui8_largeUpstreamBuffer[i] = (uint8_t)(i % 256);

    psData->pui8UpStreamBuf = ui8_largeUpstreamBuffer;
    psData->ui32DatLen      = LARGE_UPSTREAM_BUFFER_SIZE;
    psData->ui8InfoFlagBits.upstreamBufDynamic = 0;

    return eREQUEST_ACK_STATUS_SUCCESS_UPSTREAM;
}

// Command 5: records the incoming argument array so a round-trip test can
// assert the Slave actually received the values the Master sent.
teREQUEST_ACKNOWLEDGE testCmdWithArgs (uint32_t* pui32_valArray, uint8_t ui8_valArrayLen, tsTRANSFER_DATA *psData)
{
    uint8_t ui8Cnt = ui8_valArrayLen < MAX_NUM_REQUEST_VALUES ? ui8_valArrayLen : MAX_NUM_REQUEST_VALUES;

    for (uint8_t i = 0; i < ui8Cnt; i++)
        g_lastCmdArgs[i] = pui32_valArray[i];

    g_lastCmdArgCount = ui8Cnt;
    psData->ui32DatLen = 0;

    return eREQUEST_ACK_STATUS_SUCCESS;
}

#else
COMMAND_CB_STATUS testCmdUpstream (float* pf_valArray, uint8_t ui8_valArrayLen, PROCESS_INFO *p_info)
{
    float * f_buf = ui8_testBuffer;
    for (uint8_t i = 0; i < 5; i++)
        f_buf[i] = (float)i + 0.5;

    p_info->pui8_buf = ui8_testBuffer;
    p_info->ui32_datLen = 20;
    p_info->eDataFormat = e_testDtypes;
    p_info->ui16_dataFormatLen= 1;
    return eCOMMAND_STATUS_SUCCESS_UPSTREAM;
}
#endif

#ifdef VALUE_MODE_HEX
COMMAND_CB cmdStruct[SIZE_OF_CMD_STRUCT] = { testCmdUpstream,
                                              testCmdData,
                                              testCmdPlain,
                                              testCmdUpstreamLarge,
                                              testCmdWithArgs };
#else
COMMAND_CB cmdStruct[SIZE_OF_CMD_STRUCT] = { testCmdUpstream };
#endif
