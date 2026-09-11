#include "RoundTripFixture.h"

namespace {

// Captures what the Master's GetVar callback received, for assertion.
static bool         g_callbackFired = false;
static uint32_t     g_receivedValue = 0;
static teREQUEST_ACKNOWLEDGE g_receivedAck = eREQUEST_ACK_STATUS_UNKNOWN;

static teTRANSFER_ACK OnGetVarResponse(teREQUEST_ACKNOWLEDGE eAck, int16_t i16Num, uint32_t ui32Data, uint16_t ui16ErrNum)
{
    (void)i16Num;
    (void)ui16ErrNum;
    g_callbackFired = true;
    g_receivedAck   = eAck;
    g_receivedValue = ui32Data;
    return eTRANSFER_ACK_SUCCESS;
}

class RoundTripGetVarTest : public RoundTripTest {
protected:
    void SetUp() override {
        RoundTripTest::SetUp();

        g_callbackFired = false;
        g_receivedValue = 0;
        g_receivedAck   = eREQUEST_ACK_STATUS_UNKNOWN;

        tsSCI_MASTER_CALLBACKS sMasterCbs = tsSCI_MASTER_CALLBACKS_DEFAULTS;
        sMasterCbs.GetVarExternalCB     = OnGetVarResponse;
        sMasterCbs.BlockingTxExternalCB = MasterTxCbBlocking;
        SCIMasterInit(sMasterCbs);
    }
};

TEST_F(RoundTripGetVarTest, GetVarUI8RoundTrip) {
    // Same variable/protocol-number pairing as SlaveTest.PollVarUI8:
    // address 3 == ui8_test == 245 (0xF5).
    SCIRequestGetVar(3);

    bool finished = PumpUntilIdle();

    ASSERT_TRUE(finished) << "Master never returned to IDLE - transaction did not complete";
    ASSERT_TRUE(g_callbackFired) << "GetVar callback was never invoked";
    EXPECT_EQ(eREQUEST_ACK_STATUS_SUCCESS, g_receivedAck);
    EXPECT_EQ(0xF5u, g_receivedValue);
}

}  // namespace
