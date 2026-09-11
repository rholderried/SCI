#include "RoundTripFixture.h"
#include <cstring>

extern "C" {
    extern uint32_t g_lastCmdArgs[];
    extern uint8_t  g_lastCmdArgCount;
}

namespace {

// Captures what the Master's Command callback received, for assertion.
static bool                  g_cmdCallbackFired = false;
static teREQUEST_ACKNOWLEDGE g_cmdReceivedAck   = eREQUEST_ACK_STATUS_UNKNOWN;
static int16_t               g_cmdReceivedNum   = 0;
static uint8_t               g_cmdReceivedDataCnt = 0;
static uint32_t              g_cmdReceivedData[MAX_NUM_RESPONSE_VALUES];
static uint16_t              g_cmdReceivedErrNum = 0;

// Captures what the Master's Upstream callback received, for assertion.
static bool     g_upstreamCallbackFired = false;
static int16_t  g_upstreamReceivedNum   = 0;
static uint32_t g_upstreamReceivedByteCnt = 0;
static uint8_t  g_upstreamReceivedData[512];

static teTRANSFER_ACK OnCommandResponse(teREQUEST_ACKNOWLEDGE eAck, int16_t i16Num, uint32_t *pui32Data, uint8_t ui8DataCnt, uint16_t ui16ErrNum)
{
    g_cmdCallbackFired   = true;
    g_cmdReceivedAck     = eAck;
    g_cmdReceivedNum     = i16Num;
    g_cmdReceivedErrNum  = ui16ErrNum;

    uint8_t ui8CopyCnt = ui8DataCnt;
    if (ui8CopyCnt > MAX_NUM_RESPONSE_VALUES)
        ui8CopyCnt = MAX_NUM_RESPONSE_VALUES;
    g_cmdReceivedDataCnt = ui8CopyCnt;

    if (pui32Data != NULL && ui8CopyCnt > 0)
        memcpy(g_cmdReceivedData, pui32Data, ui8CopyCnt * sizeof(uint32_t));

    return eTRANSFER_ACK_SUCCESS;
}

static teTRANSFER_ACK OnUpstreamResponse(int16_t i16Num, uint8_t *pui8Data, uint32_t ui32ByteCnt)
{
    g_upstreamCallbackFired   = true;
    g_upstreamReceivedNum     = i16Num;

    uint32_t ui32CopyCnt = ui32ByteCnt;
    if (ui32CopyCnt > sizeof(g_upstreamReceivedData))
        ui32CopyCnt = sizeof(g_upstreamReceivedData);
    g_upstreamReceivedByteCnt = ui32CopyCnt;

    if (pui8Data != NULL && ui32CopyCnt > 0)
        memcpy(g_upstreamReceivedData, pui8Data, ui32CopyCnt);

    return eTRANSFER_ACK_SUCCESS;
}

class RoundTripCommandTest : public RoundTripTest {
protected:
    void SetUp() override {
        RoundTripTest::SetUp();

        g_cmdCallbackFired   = false;
        g_cmdReceivedAck     = eREQUEST_ACK_STATUS_UNKNOWN;
        g_cmdReceivedNum     = 0;
        g_cmdReceivedDataCnt = 0;
        g_cmdReceivedErrNum  = 0;
        memset(g_cmdReceivedData, 0, sizeof(g_cmdReceivedData));

        g_upstreamCallbackFired   = false;
        g_upstreamReceivedNum     = 0;
        g_upstreamReceivedByteCnt = 0;
        memset(g_upstreamReceivedData, 0, sizeof(g_upstreamReceivedData));

        g_lastCmdArgCount = 0;
        memset(g_lastCmdArgs, 0, MAX_NUM_REQUEST_VALUES * sizeof(uint32_t));

        tsSCI_MASTER_CALLBACKS sMasterCbs = tsSCI_MASTER_CALLBACKS_DEFAULTS;
        sMasterCbs.CommandExternalCB   = OnCommandResponse;
        sMasterCbs.UpstreamExternalCB  = OnUpstreamResponse;
        sMasterCbs.BlockingTxExternalCB = MasterTxCbBlocking;
        SCIMasterInit(sMasterCbs);
    }
};

// Command 3 (testCmdPlain): plain SUCCESS ack, zero result data.
TEST_F(RoundTripCommandTest, CommandPlainSuccessRoundTrip) {
    SCIRequestCommand(3, NULL, 0);

    bool finished = PumpUntilIdle();

    ASSERT_TRUE(finished) << "Master never returned to IDLE - transaction did not complete";
    ASSERT_TRUE(g_cmdCallbackFired) << "Command callback was never invoked";
    EXPECT_EQ(eREQUEST_ACK_STATUS_SUCCESS, g_cmdReceivedAck);
    EXPECT_EQ(3, g_cmdReceivedNum);
    EXPECT_EQ(0u, g_cmdReceivedDataCnt);
}

// Command 2 (testCmdData): SUCCESS_DATA ack carrying 3 known uint32 values.
TEST_F(RoundTripCommandTest, CommandDataRoundTrip) {
    SCIRequestCommand(2, NULL, 0);

    bool finished = PumpUntilIdle();

    ASSERT_TRUE(finished) << "Master never returned to IDLE - transaction did not complete";
    ASSERT_TRUE(g_cmdCallbackFired) << "Command callback was never invoked";
    // Per SCIMasterTransfer.c's eREQUEST_TYPE_COMMAND handling, the
    // SUCCESS_DATA ack is passed straight through to CommandExternalCB
    // once all expected data has been received (no ack translation).
    EXPECT_EQ(eREQUEST_ACK_STATUS_SUCCESS_DATA, g_cmdReceivedAck);
    EXPECT_EQ(2, g_cmdReceivedNum);
    ASSERT_EQ(3u, g_cmdReceivedDataCnt);
    EXPECT_EQ(0x11111111u, g_cmdReceivedData[0]);
    EXPECT_EQ(0x22222222u, g_cmdReceivedData[1]);
    EXPECT_EQ(0x33333333u, g_cmdReceivedData[2]);
}

// Command 5 (testCmdWithArgs): asserts the Slave actually receives the
// argument values the Master sends, not just that the round trip completes.
TEST_F(RoundTripCommandTest, CommandWithArgsRoundTrip) {
    tuREQUESTVALUE arr[2];
    arr[0].ui32_hex = 0xAAAA;
    arr[1].ui32_hex = 0xBBBB;

    SCIRequestCommand(5, arr, 2);

    bool finished = PumpUntilIdle();

    ASSERT_TRUE(finished) << "Master never returned to IDLE - transaction did not complete";
    ASSERT_TRUE(g_cmdCallbackFired) << "Command callback was never invoked";
    EXPECT_EQ(eREQUEST_ACK_STATUS_SUCCESS, g_cmdReceivedAck);
    EXPECT_EQ(5, g_cmdReceivedNum);
    EXPECT_EQ(0u, g_cmdReceivedDataCnt);

    ASSERT_EQ(2u, g_lastCmdArgCount) << "Slave did not receive the expected argument count";
    EXPECT_EQ(0xAAAAu, g_lastCmdArgs[0]);
    EXPECT_EQ(0xBBBBu, g_lastCmdArgs[1]);
}

// Command 1 (testCmdUpstream): SUCCESS_UPSTREAM triggers an automatic
// chained Upstream request. Single 20-byte packet, fits within one
// TX_PACKET_LENGTH/RX_PACKET_LENGTH (128), so the default PumpUntilIdle
// budget suffices.
TEST_F(RoundTripCommandTest, CommandUpstreamRoundTrip) {
    SCIRequestCommand(1, NULL, 0);

    bool finished = PumpUntilIdle();

    ASSERT_TRUE(finished) << "Master never returned to IDLE - transaction did not complete";
    ASSERT_TRUE(g_upstreamCallbackFired) << "Upstream callback was never invoked";
    EXPECT_EQ(1, g_upstreamReceivedNum);
    ASSERT_EQ(20u, g_upstreamReceivedByteCnt);
    for (uint32_t i = 0; i < 20; i++) {
        EXPECT_EQ((uint8_t)(i + 1), g_upstreamReceivedData[i]) << "Mismatch at byte index " << i;
    }
}

// Command 4 (testCmdUpstreamLarge): 300-byte upstream payload, deliberately
// larger than TX_PACKET_LENGTH/RX_PACKET_LENGTH (128), forcing multi-packet
// chunking on both Master and Slave. Needs a larger iteration budget.
TEST_F(RoundTripCommandTest, CommandUpstreamLargeRoundTripChunked) {
    SCIRequestCommand(4, NULL, 0);

    bool finished = PumpUntilIdle(2000);

    ASSERT_TRUE(finished) << "Master never returned to IDLE - transaction did not complete (chunked upstream)";
    ASSERT_TRUE(g_upstreamCallbackFired) << "Upstream callback was never invoked";
    EXPECT_EQ(4, g_upstreamReceivedNum);
    ASSERT_EQ(300u, g_upstreamReceivedByteCnt);

    int firstMismatchIdx = -1;
    for (uint32_t i = 0; i < 300; i++) {
        uint8_t expected = (uint8_t)(i % 256);
        if (g_upstreamReceivedData[i] != expected && firstMismatchIdx < 0)
            firstMismatchIdx = (int)i;
        EXPECT_EQ(expected, g_upstreamReceivedData[i]) << "Mismatch at byte index " << i;
    }
    EXPECT_EQ(-1, firstMismatchIdx) << "First mismatching byte index: " << firstMismatchIdx;
}

}  // namespace
