#include <gtest/gtest.h>

extern "C" {
#include "SCISlave.h"
#include "SCIMaster.h"
#include "MasterTestCallbacks.h"
}

extern "C" {
    extern tsSCIVAR varStruct[];
    extern COMMAND_CB cmdStruct;
    extern tsSCI_SLAVE_CALLBACKS sSlaveTestCbs;
}

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

class RoundTripTest : public ::testing::Test {
protected:
    void SetUp() override {
        g_callbackFired = false;
        g_receivedValue = 0;
        g_receivedAck   = eREQUEST_ACK_STATUS_UNKNOWN;

        SCISlaveInit(sSlaveTestCbs, varStruct, &cmdStruct);

        tsSCI_MASTER_CALLBACKS sMasterCbs = tsSCI_MASTER_CALLBACKS_DEFAULTS;
        sMasterCbs.GetVarExternalCB     = OnGetVarResponse;
        sMasterCbs.BlockingTxExternalCB = MasterTxCbBlocking;
        SCIMasterInit(sMasterCbs);
    }
};

// Pumps both statemachines in lockstep until the Master returns to IDLE
// (transaction complete) or the iteration budget runs out.
static bool PumpUntilIdle(int maxIterations = 500) {
    for (int i = 0; i < maxIterations; i++) {
        SCIMasterSM();
        SCISlaveStatemachine();
        if (SCIGetProtocolState() == ePROTOCOL_IDLE && i > 0)
            return true;
    }
    return false;
}

TEST_F(RoundTripTest, GetVarUI8RoundTrip) {
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
