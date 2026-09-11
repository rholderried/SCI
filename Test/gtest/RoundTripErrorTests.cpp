/**************************************************************************//**
 * \file RoundTripErrorTests.cpp
 * \author Roman Holderried
 *
 * \brief Round-trip tests for GetVar/SetVar/Command error paths (invalid
 *        numbers), exercised through the Master's public API against a
 *        real Slave in the same process.
 *
 * Var numbers are 1-indexed and valid in [1, SIZE_OF_VAR_STRUCT]; command
 * numbers are 1-indexed and valid in [1, SIZE_OF_CMD_STRUCT] (see
 * C/Test/config/SCIconfig.h). Requests using numbers outside those ranges
 * must be rejected by the Slave with a well-defined teSCI_SLAVE_ERROR,
 * reported back to the Master with SCI_ERROR_OFFSET added (see
 * GET_SCI_ERROR_NUMBER() in SCISlave.c) - never crash, never silently
 * succeed, and always return the Master's protocol state to IDLE.
 *
 * <b> History </b>
 * \t- 2026-09-10 - File creation (Phase 3 - error-path round trips).
 *****************************************************************************/
#include "RoundTripFixture.h"

extern "C" {
#include "SCICommon.h"
}

namespace {

// -----------------------------------------------------------------------
// GetVar error path
// -----------------------------------------------------------------------

static bool                  g_getVarCbFired  = false;
static teREQUEST_ACKNOWLEDGE g_getVarAck      = eREQUEST_ACK_STATUS_UNKNOWN;
static uint16_t              g_getVarErrNum   = 0;

static teTRANSFER_ACK OnGetVarResponse(teREQUEST_ACKNOWLEDGE eAck, int16_t i16Num, uint32_t ui32Data, uint16_t ui16ErrNum)
{
    (void)i16Num;
    (void)ui32Data;
    g_getVarCbFired = true;
    g_getVarAck     = eAck;
    g_getVarErrNum  = ui16ErrNum;
    return eTRANSFER_ACK_SUCCESS;
}

class RoundTripGetVarErrorTest : public RoundTripTest {
protected:
    void SetUp() override {
        RoundTripTest::SetUp();

        g_getVarCbFired = false;
        g_getVarAck     = eREQUEST_ACK_STATUS_UNKNOWN;
        g_getVarErrNum  = 0;

        tsSCI_MASTER_CALLBACKS sMasterCbs = tsSCI_MASTER_CALLBACKS_DEFAULTS;
        sMasterCbs.GetVarExternalCB     = OnGetVarResponse;
        sMasterCbs.BlockingTxExternalCB = MasterTxCbBlocking;
        SCIMasterInit(sMasterCbs);
    }
};

TEST_F(RoundTripGetVarErrorTest, GetVarInvalidNumberReturnsError) {
    // 999 is far outside the valid range [1, SIZE_OF_VAR_STRUCT=7].
    SCIRequestGetVar(999);

    bool finished = PumpUntilIdle();

    ASSERT_TRUE(finished) << "Master never returned to IDLE - transaction did not complete";
    ASSERT_TRUE(g_getVarCbFired) << "GetVar callback was never invoked";
    EXPECT_EQ(eREQUEST_ACK_STATUS_ERROR, g_getVarAck);
    EXPECT_EQ((uint16_t)(SCI_ERROR_OFFSET + eSCI_SLAVE_ERROR_VAR_NUMBER_INVALID), g_getVarErrNum);
}

// -----------------------------------------------------------------------
// SetVar error path
// -----------------------------------------------------------------------

static bool                  g_setVarCbFired  = false;
static teREQUEST_ACKNOWLEDGE g_setVarAck      = eREQUEST_ACK_STATUS_UNKNOWN;
static uint16_t              g_setVarErrNum   = 0;

static teTRANSFER_ACK OnSetVarResponse(teREQUEST_ACKNOWLEDGE eAck, int16_t i16Num, uint16_t ui16ErrNum)
{
    (void)i16Num;
    g_setVarCbFired = true;
    g_setVarAck     = eAck;
    g_setVarErrNum  = ui16ErrNum;
    return eTRANSFER_ACK_SUCCESS;
}

class RoundTripSetVarErrorTest : public RoundTripTest {
protected:
    void SetUp() override {
        RoundTripTest::SetUp();

        g_setVarCbFired = false;
        g_setVarAck     = eREQUEST_ACK_STATUS_UNKNOWN;
        g_setVarErrNum  = 0;

        tsSCI_MASTER_CALLBACKS sMasterCbs = tsSCI_MASTER_CALLBACKS_DEFAULTS;
        sMasterCbs.SetVarExternalCB     = OnSetVarResponse;
        sMasterCbs.BlockingTxExternalCB = MasterTxCbBlocking;
        SCIMasterInit(sMasterCbs);
    }
};

TEST_F(RoundTripSetVarErrorTest, SetVarInvalidNumberReturnsError) {
    tuREQUESTVALUE uVal;
    uVal.ui32_hex = 42u;

    // 999 is far outside the valid range [1, SIZE_OF_VAR_STRUCT=7].
    SCIRequestSetVar(999, uVal);

    bool finished = PumpUntilIdle();

    ASSERT_TRUE(finished) << "Master never returned to IDLE - transaction did not complete";
    ASSERT_TRUE(g_setVarCbFired) << "SetVar callback was never invoked";
    EXPECT_EQ(eREQUEST_ACK_STATUS_ERROR, g_setVarAck);
    EXPECT_EQ((uint16_t)(SCI_ERROR_OFFSET + eSCI_SLAVE_ERROR_VAR_NUMBER_INVALID), g_setVarErrNum);
}

// -----------------------------------------------------------------------
// Command error path
// -----------------------------------------------------------------------

static bool                  g_commandCbFired = false;
static teREQUEST_ACKNOWLEDGE g_commandAck     = eREQUEST_ACK_STATUS_UNKNOWN;
static uint16_t              g_commandErrNum  = 0;

static teTRANSFER_ACK OnCommandResponse(teREQUEST_ACKNOWLEDGE eAck, int16_t i16Num, uint32_t *pui32Data, uint8_t ui8DataCnt, uint16_t ui16ErrNum)
{
    (void)i16Num;
    (void)pui32Data;
    (void)ui8DataCnt;
    g_commandCbFired = true;
    g_commandAck     = eAck;
    g_commandErrNum  = ui16ErrNum;
    return eTRANSFER_ACK_SUCCESS;
}

class RoundTripCommandErrorTest : public RoundTripTest {
protected:
    void SetUp() override {
        RoundTripTest::SetUp();

        g_commandCbFired = false;
        g_commandAck     = eREQUEST_ACK_STATUS_UNKNOWN;
        g_commandErrNum  = 0;

        tsSCI_MASTER_CALLBACKS sMasterCbs = tsSCI_MASTER_CALLBACKS_DEFAULTS;
        sMasterCbs.CommandExternalCB    = OnCommandResponse;
        sMasterCbs.BlockingTxExternalCB = MasterTxCbBlocking;
        SCIMasterInit(sMasterCbs);
    }
};

TEST_F(RoundTripCommandErrorTest, CommandInvalidNumberReturnsError) {
    // 99 is outside the valid range [1, SIZE_OF_CMD_STRUCT=5].
    SCIRequestCommand(99, NULL, 0);

    bool finished = PumpUntilIdle();

    ASSERT_TRUE(finished) << "Master never returned to IDLE - transaction did not complete";
    ASSERT_TRUE(g_commandCbFired) << "Command callback was never invoked";
    EXPECT_EQ(eREQUEST_ACK_STATUS_ERROR, g_commandAck);
    EXPECT_EQ((uint16_t)(SCI_ERROR_OFFSET + eSCI_SLAVE_ERROR_REQUEST_UNKNOWN), g_commandErrNum);
}

}  // namespace
