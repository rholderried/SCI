/**************************************************************************//**
 * \file RoundTripSetVarTests.cpp
 * \author Roman Holderried
 *
 * \brief GoogleTest round-trip tests for the SetVar request type.
 *
 * Exercises SCIRequestSetVar() against the shared Slave fixture
 * (VariablesAndCommands.c / TestCallbacks.c), covering every datatype
 * present in varStruct, the EEPROM-backed variable's round trip and write
 * failure rollback path, and the action-procedure trigger.
 *
 * <b> History </b>
 * \t- 2026-09-10 - File creation (Phase 1 - SetVar round trips)
 *****************************************************************************/
#include "RoundTripFixture.h"

extern "C" {
    extern float    testVar;
    extern float    f_test;
    extern uint8_t  ui8_test;
    extern uint16_t ui16_test;
    extern uint32_t i32_test;
    extern uint16_t ui16_eeprom_test;
    extern uint8_t  ui8_ap_test;
    extern uint32_t g_apCallCount;
    extern uint16_t ui16EEPROMWordAddressable[];
    extern bool     g_forceEEPROMWriteFailure;
}

namespace {

// Captures what the Master's SetVar callback received, for assertion.
static bool                   g_callbackFired = false;
static teREQUEST_ACKNOWLEDGE  g_receivedAck   = eREQUEST_ACK_STATUS_UNKNOWN;
static int16_t                g_receivedNum   = 0;
static uint16_t               g_receivedErr   = 0;

static teTRANSFER_ACK OnSetVarResponse(teREQUEST_ACKNOWLEDGE eAck, int16_t i16Num, uint16_t ui16ErrNum)
{
    g_callbackFired = true;
    g_receivedAck   = eAck;
    g_receivedNum   = i16Num;
    g_receivedErr   = ui16ErrNum;
    return eTRANSFER_ACK_SUCCESS;
}

class RoundTripSetVarTest : public RoundTripTest {
protected:
    void SetUp() override {
        RoundTripTest::SetUp();

        g_callbackFired = false;
        g_receivedAck   = eREQUEST_ACK_STATUS_UNKNOWN;
        g_receivedNum   = 0;
        g_receivedErr   = 0;
        g_forceEEPROMWriteFailure = false;

        tsSCI_MASTER_CALLBACKS sMasterCbs = tsSCI_MASTER_CALLBACKS_DEFAULTS;
        sMasterCbs.SetVarExternalCB    = OnSetVarResponse;
        sMasterCbs.BlockingTxExternalCB = MasterTxCbBlocking;
        SCIMasterInit(sMasterCbs);
    }

    void TearDown() override {
        g_forceEEPROMWriteFailure = false;
        RoundTripTest::TearDown();
    }
};

TEST_F(RoundTripSetVarTest, SetVarUI8RoundTrip) {
    tuREQUESTVALUE uVal;
    uVal.ui32_hex = 0x11;

    SCIRequestSetVar(3, uVal);

    bool finished = PumpUntilIdle();

    ASSERT_TRUE(finished) << "Master never returned to IDLE - transaction did not complete";
    ASSERT_TRUE(g_callbackFired) << "SetVar callback was never invoked";
    EXPECT_EQ(eREQUEST_ACK_STATUS_SUCCESS, g_receivedAck);
    EXPECT_EQ(3, g_receivedNum);
    EXPECT_EQ(0x11u, ui8_test);
}

TEST_F(RoundTripSetVarTest, SetVarUI16RoundTrip) {
    tuREQUESTVALUE uVal;
    uVal.ui32_hex = 0xBEEF;

    SCIRequestSetVar(4, uVal);

    bool finished = PumpUntilIdle();

    ASSERT_TRUE(finished) << "Master never returned to IDLE - transaction did not complete";
    ASSERT_TRUE(g_callbackFired) << "SetVar callback was never invoked";
    EXPECT_EQ(eREQUEST_ACK_STATUS_SUCCESS, g_receivedAck);
    EXPECT_EQ(4, g_receivedNum);
    EXPECT_EQ(0xBEEFu, ui16_test);
}

TEST_F(RoundTripSetVarTest, SetVarI32RoundTrip) {
    tuREQUESTVALUE uVal;
    uVal.ui32_hex = 0xDEADBEEFu;

    SCIRequestSetVar(5, uVal);

    bool finished = PumpUntilIdle();

    ASSERT_TRUE(finished) << "Master never returned to IDLE - transaction did not complete";
    ASSERT_TRUE(g_callbackFired) << "SetVar callback was never invoked";
    EXPECT_EQ(eREQUEST_ACK_STATUS_SUCCESS, g_receivedAck);
    EXPECT_EQ(5, g_receivedNum);
    EXPECT_EQ(0xDEADBEEFu, i32_test);
}

TEST_F(RoundTripSetVarTest, SetVarF32RoundTrip) {
    union { float f; uint32_t u; } conv;
    conv.f = 3.14159f;

    tuREQUESTVALUE uVal;
    uVal.ui32_hex = conv.u;

    SCIRequestSetVar(1, uVal);

    bool finished = PumpUntilIdle();

    ASSERT_TRUE(finished) << "Master never returned to IDLE - transaction did not complete";
    ASSERT_TRUE(g_callbackFired) << "SetVar callback was never invoked";
    EXPECT_EQ(eREQUEST_ACK_STATUS_SUCCESS, g_receivedAck);
    EXPECT_EQ(1, g_receivedNum);

    union { float f; uint32_t u; } result;
    result.f = testVar;
    EXPECT_EQ(conv.u, result.u);
}

TEST_F(RoundTripSetVarTest, SetVarTriggersActionProcedureOnce) {
    uint32_t startCount = g_apCallCount;

    tuREQUESTVALUE uVal;
    uVal.ui32_hex = 0x01;
    SCIRequestSetVar(7, uVal);
    bool finished = PumpUntilIdle();

    ASSERT_TRUE(finished) << "Master never returned to IDLE - transaction did not complete";
    ASSERT_TRUE(g_callbackFired) << "SetVar callback was never invoked";
    EXPECT_EQ(eREQUEST_ACK_STATUS_SUCCESS, g_receivedAck);
    EXPECT_EQ(startCount + 1, g_apCallCount);

    g_callbackFired = false;
    uVal.ui32_hex = 0x02;
    SCIRequestSetVar(7, uVal);
    finished = PumpUntilIdle();

    ASSERT_TRUE(finished) << "Master never returned to IDLE - transaction did not complete";
    ASSERT_TRUE(g_callbackFired) << "SetVar callback was never invoked";
    EXPECT_EQ(eREQUEST_ACK_STATUS_SUCCESS, g_receivedAck);
    EXPECT_EQ(startCount + 2, g_apCallCount);
}

TEST_F(RoundTripSetVarTest, SetVarEepromRoundTrip) {
    tuREQUESTVALUE uVal;
    uVal.ui32_hex = 0x1234;

    SCIRequestSetVar(6, uVal);

    bool finished = PumpUntilIdle();

    ASSERT_TRUE(finished) << "Master never returned to IDLE - transaction did not complete";
    ASSERT_TRUE(g_callbackFired) << "SetVar callback was never invoked";
    EXPECT_EQ(eREQUEST_ACK_STATUS_SUCCESS, g_receivedAck);
    EXPECT_EQ(6, g_receivedNum);
    EXPECT_EQ(0x1234u, ui16_eeprom_test);
    EXPECT_EQ(0x1234u, ui16EEPROMWordAddressable[0]);
}

TEST_F(RoundTripSetVarTest, SetVarEepromWriteFailureRollsBackValue) {
    uint16_t originalValue = ui16_eeprom_test;
    ASSERT_NE(0x5678u, originalValue) << "Test fixture assumption broken - pick a different new value";

    g_forceEEPROMWriteFailure = true;

    tuREQUESTVALUE uVal;
    uVal.ui32_hex = 0x5678;

    SCIRequestSetVar(6, uVal);

    bool finished = PumpUntilIdle();

    ASSERT_TRUE(finished) << "Master never returned to IDLE - transaction did not complete";
    ASSERT_TRUE(g_callbackFired) << "SetVar callback was never invoked";
    EXPECT_EQ(eREQUEST_ACK_STATUS_ERROR, g_receivedAck);
    EXPECT_NE(0u, g_receivedErr);
    EXPECT_EQ(originalValue, ui16_eeprom_test);

    g_forceEEPROMWriteFailure = false;
}

}  // namespace
