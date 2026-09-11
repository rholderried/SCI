/**************************************************************************//**
 * \file RoundTripSequenceTests.cpp
 * \author Roman Holderried
 *
 * \brief GoogleTest sequencing/statefulness round-trip tests.
 *
 * Two concerns are covered here:
 *
 *  1. RoundTripSequenceTest.GetVarThenSetVarThenCommandSequenceRoundTrip -
 *     drives three independent, fully-completed Master/Slave transactions
 *     back-to-back through the real round-trip path (Master + Slave, both
 *     statemachines pumped via PumpUntilIdle()) and asserts that each
 *     transaction's callback fires exactly once, with exactly the
 *     response shape appropriate to ITS OWN request - guarding against
 *     state leaking between transactions in the Slave's
 *     sResponseControl/sTransferInfo structures.
 *
 *  2. SlaveTransferProcessRequestTest.NewCommandNumberIsNotRecognizedAsFreshMidTransfer -
 *     a narrower, Slave-only unit test of the "new command vs. continuing
 *     command" branch in SCISlaveTransfer.c's eREQUEST_TYPE_COMMAND case
 *     (the `bNewCmd` check). This exercises
 *     SCISlaveTransferProcessRequest() directly against a
 *     tsSCI_TRANSFER_SLAVE instance, rather than going through the full
 *     Master-driven round trip.
 *
 *     Rationale (documented limitation of the approach, see task context):
 *     the Master's own SCITransferControl() (SCIMasterTransfer.c) always
 *     fully drains one command's data - looping the SAME command number's
 *     request - before the public API could ever issue a different
 *     command number. There is therefore no way to reach the Slave's
 *     "different command number arrives while a previous command's data
 *     transfer is still ongoing" branch through a single coherent
 *     Master-driven flow (SCIRequestCommand() etc.), nor even through raw
 *     byte injection into SCISlaveReceiveData()/SCISlaveStatemachine():
 *     with this test suite's SCIconfig.h (TX_PACKET_LENGTH=128,
 *     MAX_NUM_RESPONSE_VALUES=10) every fixture command's SUCCESS_DATA
 *     payload (at most 10 values, ~90 ASCII bytes) fits into a single TX
 *     packet, so the Slave's response-builder step always clears the
 *     "ongoing" flag again within the very same statemachine cycle that
 *     set it - there is no multi-cycle window in which a raw follow-up
 *     request could observe ongoing==true for a still-incomplete transfer
 *     without editing the fixture data (out of scope for this task).
 *
 *     SCISlaveTransferProcessRequest() is called directly instead: this
 *     sets up the exact "ongoing==true, previous command's data not yet
 *     drained" state (mirroring the exact call sequence
 *     SCISlaveStatemachine() uses: SCISlaveTransferInitiateResponse()
 *     immediately before SCISlaveTransferProcessRequest() on every
 *     request), then issues a second request with a *different* command
 *     number on the same tsSCI_TRANSFER_SLAVE instance while that ongoing
 *     state is still present.
 *
 *     UNEXPECTED FINDING (this test documents a real bug, it does not
 *     assert the "ideal" behavior): the second request is NOT treated as
 *     fresh. SCISlaveStatemachine() unconditionally calls
 *     SCISlaveTransferInitiateResponse(psTransfer, sReq.i16Num, ...)
 *     *before* SCISlaveTransferProcessRequest() runs, on every request -
 *     which overwrites psTransfer->sResponseControl.sRsp.i16Num to the
 *     incoming request's own number first. By the time bNewCmd evaluates
 *     `psTransfer->sResponseControl.sRsp.i16Num != sReq.i16Num`, both
 *     sides of that comparison are always the same request's number, so
 *     the comparison is dead code - it can never observe a "different"
 *     command number, and bNewCmd collapses to just `ongoing == false`.
 *     Concretely: a new command number arriving while `ongoing == true`
 *     is silently swallowed into the "continuing" branch - its callback
 *     is never invoked, and the response sent back still carries the
 *     PREVIOUS command's ack/data-length/upstream state (only
 *     firstPacketNotSent is cleared). This is the same class of dead
 *     "number equality" check already called out for the UPSTREAM branch
 *     in SlaveErrorTests.cpp's UpstreamAfterNonUpstreamCommandReturnsError
 *     comment - it affects COMMAND's bNewCmd the same way. Not fixed here
 *     (out of scope - this is a test-writing task), but pinned down by
 *     this test so a future fix has a regression test to flip green.
 ******************************************************************************/
#include "RoundTripFixture.h"

extern "C" {
    extern uint16_t ui16_test;    // varStruct[3] (var number 4)
}

namespace {

/*******************************************************************************
 * Part 1: GetVar -> SetVar -> Command sequencing round trip (Master+Slave)
 ******************************************************************************/

// Fire counts + last-received payload per callback type, so we can assert
// each callback fired EXACTLY once per transaction it belongs to (neither
// zero - callback missed - nor two - stale/duplicate invocation caused by
// leftover state from a previous transaction).
static int             g_getVarFireCount = 0;
static teREQUEST_ACKNOWLEDGE g_getVarLastAck  = eREQUEST_ACK_STATUS_UNKNOWN;
static uint32_t        g_getVarLastValue = 0;

static int             g_setVarFireCount = 0;
static teREQUEST_ACKNOWLEDGE g_setVarLastAck  = eREQUEST_ACK_STATUS_UNKNOWN;

static int             g_cmdFireCount    = 0;
static teREQUEST_ACKNOWLEDGE g_cmdLastAck     = eREQUEST_ACK_STATUS_UNKNOWN;
static uint8_t          g_cmdLastDataCnt  = 0xFF;

static teTRANSFER_ACK OnSeqGetVarResponse(teREQUEST_ACKNOWLEDGE eAck, int16_t i16Num, uint32_t ui32Data, uint16_t ui16ErrNum)
{
    (void)i16Num;
    (void)ui16ErrNum;
    g_getVarFireCount++;
    g_getVarLastAck   = eAck;
    g_getVarLastValue = ui32Data;
    return eTRANSFER_ACK_SUCCESS;
}

static teTRANSFER_ACK OnSeqSetVarResponse(teREQUEST_ACKNOWLEDGE eAck, int16_t i16Num, uint16_t ui16ErrNum)
{
    (void)i16Num;
    (void)ui16ErrNum;
    g_setVarFireCount++;
    g_setVarLastAck = eAck;
    return eTRANSFER_ACK_SUCCESS;
}

static teTRANSFER_ACK OnSeqCommandResponse(teREQUEST_ACKNOWLEDGE eAck, int16_t i16Num, uint32_t *pui32Data, uint8_t ui8DataCnt, uint16_t ui16ErrNum)
{
    (void)i16Num;
    (void)pui32Data;
    (void)ui16ErrNum;
    g_cmdFireCount++;
    g_cmdLastAck    = eAck;
    g_cmdLastDataCnt = ui8DataCnt;
    return eTRANSFER_ACK_SUCCESS;
}

class RoundTripSequenceTest : public RoundTripTest {
protected:
    void SetUp() override {
        RoundTripTest::SetUp();

        g_getVarFireCount = 0;
        g_getVarLastAck   = eREQUEST_ACK_STATUS_UNKNOWN;
        g_getVarLastValue = 0;

        g_setVarFireCount = 0;
        g_setVarLastAck   = eREQUEST_ACK_STATUS_UNKNOWN;

        g_cmdFireCount    = 0;
        g_cmdLastAck      = eREQUEST_ACK_STATUS_UNKNOWN;
        g_cmdLastDataCnt  = 0xFF;

        tsSCI_MASTER_CALLBACKS sMasterCbs = tsSCI_MASTER_CALLBACKS_DEFAULTS;
        sMasterCbs.GetVarExternalCB     = OnSeqGetVarResponse;
        sMasterCbs.SetVarExternalCB     = OnSeqSetVarResponse;
        sMasterCbs.CommandExternalCB    = OnSeqCommandResponse;
        sMasterCbs.BlockingTxExternalCB = MasterTxCbBlocking;
        SCIMasterInit(sMasterCbs);
    }
};

// Runs three independent, fully-completed transactions (GetVar, SetVar,
// Command) back-to-back in a single test and checks that no response state
// leaks from one transaction into the next: each callback must fire
// exactly once, at the step it belongs to, carrying exactly its own
// transaction's data - never a stale/duplicate firing caused by another
// transaction's response shape (e.g. the Slave's sResponseControl not
// being properly reset between requests).
TEST_F(RoundTripSequenceTest, GetVarThenSetVarThenCommandSequenceRoundTrip) {
    // ---- Step 1: GetVar(3) -> ui8_test == 245 (0xF5) ----------------------
    SCIRequestGetVar(3);
    bool finished1 = PumpUntilIdle();

    ASSERT_TRUE(finished1) << "Master never returned to IDLE after GetVar - transaction did not complete";
    EXPECT_EQ(1, g_getVarFireCount) << "GetVar callback must fire exactly once for the GetVar transaction";
    EXPECT_EQ(0, g_setVarFireCount) << "SetVar callback must NOT fire as a side effect of a GetVar transaction";
    EXPECT_EQ(0, g_cmdFireCount)    << "Command callback must NOT fire as a side effect of a GetVar transaction";
    EXPECT_EQ(eREQUEST_ACK_STATUS_SUCCESS, g_getVarLastAck);
    EXPECT_EQ(0xF5u, g_getVarLastValue);

    // ---- Step 2: SetVar(4, 0x1234) -> ui16_test ----------------------------
    tuREQUESTVALUE uVal;
    uVal.ui32_hex = 0x1234;
    SCIRequestSetVar(4, uVal);
    bool finished2 = PumpUntilIdle();

    ASSERT_TRUE(finished2) << "Master never returned to IDLE after SetVar - transaction did not complete";
    // GetVar's fire count must stay pinned at 1 - if the Slave's
    // response-control state leaked forward, this SetVar transaction could
    // spuriously look like the still-pending prior GetVar, or fire it
    // again.
    EXPECT_EQ(1, g_getVarFireCount) << "GetVar callback must not fire again during the SetVar transaction";
    EXPECT_EQ(1, g_setVarFireCount) << "SetVar callback must fire exactly once for the SetVar transaction";
    EXPECT_EQ(0, g_cmdFireCount)    << "Command callback must NOT fire as a side effect of a SetVar transaction";
    EXPECT_EQ(eREQUEST_ACK_STATUS_SUCCESS, g_setVarLastAck);
    EXPECT_EQ(0x1234u, ui16_test) << "Slave-side variable was not actually updated by the SetVar transaction";

    // ---- Step 3: Command(3) - testCmdPlain, plain SUCCESS, no data --------
    SCIRequestCommand(3, NULL, 0);
    bool finished3 = PumpUntilIdle();

    ASSERT_TRUE(finished3) << "Master never returned to IDLE after Command - transaction did not complete";
    EXPECT_EQ(1, g_getVarFireCount) << "GetVar callback must not fire again during the Command transaction";
    EXPECT_EQ(1, g_setVarFireCount) << "SetVar callback must not fire again during the Command transaction";
    EXPECT_EQ(1, g_cmdFireCount)    << "Command callback must fire exactly once for the Command transaction";
    EXPECT_EQ(eREQUEST_ACK_STATUS_SUCCESS, g_cmdLastAck);
    EXPECT_EQ(0u, g_cmdLastDataCnt) << "testCmdPlain returns no result data - a nonzero count would indicate stale data from a previous transaction (e.g. the SetVar step)";
}

/*******************************************************************************
 * Part 2: "new command number while a prior command's data transfer is
 * still ongoing" - Slave-only unit test of SCISlaveTransferProcessRequest().
 *
 * See the file-header comment for why this uses Option B (direct calls to
 * SCISlaveTransferProcessRequest() on a manually constructed
 * tsSCI_TRANSFER_SLAVE) rather than Option A (raw byte injection): with
 * this test suite's SCIconfig.h, no fixture command's response actually
 * spans multiple TX packets, so there is no window in which a raw
 * follow-up request could observe genuine leftover "ongoing" state.
 ******************************************************************************/

class SlaveTransferProcessRequestTest : public ::testing::Test {
protected:
    tsSCI_TRANSFER_SLAVE sTransfer = tsSCI_TRANSFER_SLAVE_DEFAULTS;
    tsVAR_ACCESS          sVarAccess = tsVAR_ACCESS_DEFAULTS;

    void SetUp() override {
        sTransfer.pCmdCBStruct = cmdStruct;
        sVarAccess.pVarStruct  = varStruct;
    }
};

// Command 2 (testCmdData) returns eREQUEST_ACK_STATUS_SUCCESS_DATA with
// ui32DatLen == 3 and sets the "ongoing" control bit. A second request
// then arrives for a DIFFERENT command number (3, testCmdPlain) while
// that "ongoing" state is still in place (not yet drained/cleared) -
// exactly as it would be mid-transfer in the real statemachine.
//
// This documents the actual (buggy) behavior found by reading
// SCISlaveTransfer.c's bNewCmd check together with SCISlave.c's calling
// sequence: see the file-header comment for the full explanation of why
// the i16Num-mismatch half of `bNewCmd` is dead code. The second
// request's DIFFERENT command number is NOT recognized as a fresh
// command - it gets silently merged into command 2's still-ongoing
// state (testCmdPlain's callback never runs; the response sent back
// still reflects command 2's SUCCESS_DATA/ui32DatLen==3/ongoing state).
TEST_F(SlaveTransferProcessRequestTest, NewCommandNumberIsNotRecognizedAsFreshMidTransfer) {
    // Zero-initialize and set fields by name (equivalent to
    // tsREQUEST_DEFAULTS, spelled out here since this test only cares
    // about eReqType/i16Num/ui8ValArrLen).
    tsREQUEST sReqFirst = {};
    sReqFirst.eReqType     = eREQUEST_TYPE_COMMAND;
    sReqFirst.i16Num       = 2;          // testCmdData
    sReqFirst.ui8ValArrLen = 0;          // uValArr is a fixed array member - zero-initialized above, no assignment needed

    // SCISlave.c's statemachine always calls
    // SCISlaveTransferInitiateResponse() to stamp the response's
    // i16Num/eReqType from the incoming request immediately before
    // SCISlaveTransferProcessRequest() runs - mirror that here since we're
    // driving the transfer layer directly.
    SCISlaveTransferInitiateResponse(&sTransfer, sReqFirst.i16Num, sReqFirst.eReqType);
    teSCI_SLAVE_ERROR eError1 = SCISlaveTransferProcessRequest(&sTransfer, &sVarAccess, sReqFirst);

    ASSERT_EQ(eSCI_SLAVE_ERROR_NONE, eError1);
    ASSERT_TRUE(sTransfer.sResponseControl.ui8ControlBits.ongoing)
        << "Test setup assumption broken - testCmdData's SUCCESS_DATA response should mark the transfer ongoing";
    EXPECT_EQ(2, sTransfer.sResponseControl.sRsp.i16Num);
    EXPECT_EQ(eREQUEST_ACK_STATUS_SUCCESS_DATA, sTransfer.sResponseControl.sRsp.eReqAck);
    EXPECT_EQ(3u, sTransfer.sResponseControl.sRsp.sTransferData.ui32DatLen);
    EXPECT_TRUE(sTransfer.sResponseControl.ui8ControlBits.firstPacketNotSent);

    // Second request: a DIFFERENT command number, arriving while the first
    // command's "ongoing" data transfer state is still in place (not yet
    // cleared by SCISlaveTransferClearResponseControl(), exactly as it
    // would be mid-transfer in the real statemachine).
    tsREQUEST sReqSecond = {};
    sReqSecond.eReqType     = eREQUEST_TYPE_COMMAND;
    sReqSecond.i16Num       = 3;          // testCmdPlain - deliberately different from 2
    sReqSecond.ui8ValArrLen = 0;          // uValArr is a fixed array member - zero-initialized above, no assignment needed

    SCISlaveTransferInitiateResponse(&sTransfer, sReqSecond.i16Num, sReqSecond.eReqType);
    teSCI_SLAVE_ERROR eError2 = SCISlaveTransferProcessRequest(&sTransfer, &sVarAccess, sReqSecond);

    ASSERT_EQ(eSCI_SLAVE_ERROR_NONE, eError2);

    // SCISlaveTransferInitiateResponse() unconditionally overwrote
    // sRsp.i16Num to 3 (the *incoming* request's number) BEFORE
    // ProcessRequest() ran, regardless of whether bNewCmd treated this as
    // new or continuing - so this field alone can't distinguish the two
    // cases here (see file header). The response ack/data-length/ongoing
    // fields below are what actually reveal which branch executed.
    EXPECT_EQ(3, sTransfer.sResponseControl.sRsp.i16Num);

    // bNewCmd's `sRsp.i16Num != sReq.i16Num` half never fires (dead code -
    // see file header), so with ongoing==true, bNewCmd is false: the
    // Slave treats this as a CONTINUATION of command 2, not a fresh
    // command 3. testCmdPlain's callback is therefore never invoked, and
    // the response still carries command 2's ack/data-length state.
    EXPECT_EQ(eREQUEST_ACK_STATUS_SUCCESS_DATA, sTransfer.sResponseControl.sRsp.eReqAck)
        << "Documents the discovered bug: command 2's SUCCESS_DATA ack leaks into command 3's response "
           "because bNewCmd's i16Num check is dead code (see file header) - testCmdPlain's own "
           "eREQUEST_ACK_STATUS_SUCCESS was never assigned.";
    EXPECT_EQ(3u, sTransfer.sResponseControl.sRsp.sTransferData.ui32DatLen)
        << "Documents the discovered bug: command 2's leftover data length (3) is still present - "
           "testCmdPlain (which would set ui32DatLen to 0) was never actually invoked.";
    // Continuation branch explicitly sets firstPacketNotSent = false (see
    // SCISlaveTransfer.c's `else` arm of `if (bNewCmd)`), unlike a fresh
    // command which would set it true.
    EXPECT_FALSE(sTransfer.sResponseControl.ui8ControlBits.firstPacketNotSent)
        << "Documents the discovered bug: firstPacketNotSent was cleared as a continuation would, "
           "not set as a genuinely fresh command would.";
    // ongoing bit is untouched by the continuation branch, so it still
    // reflects command 2's not-yet-fully-drained data transfer.
    EXPECT_TRUE(sTransfer.sResponseControl.ui8ControlBits.ongoing)
        << "Documents the discovered bug: the ongoing bit still reflects command 2's data transfer - "
           "command 3 (testCmdPlain, which sets no data) was never actually run.";
}

}  // namespace
