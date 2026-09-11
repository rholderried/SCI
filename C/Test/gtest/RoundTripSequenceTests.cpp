/**************************************************************************//**
 * \file RoundTripSequenceTests.cpp
 * \author Roman Holderried
 *
 * \brief GoogleTest sequencing/statefulness round-trip tests.
 *
 * Three concerns are covered here:
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
 *  2. SlaveTransferProcessRequestTest - narrower, Slave-only unit tests of
 *     the "new command/upstream vs. continuing transfer" branches in
 *     SCISlaveTransfer.c's eREQUEST_TYPE_COMMAND and eREQUEST_TYPE_UPSTREAM
 *     cases (the `bNewCmd` check and the upstream number-match check).
 *     These exercise SCISlaveTransferProcessRequest() directly against a
 *     tsSCI_TRANSFER_SLAVE instance, rather than going through the full
 *     Master-driven round trip.
 *
 *     Rationale (documented limitation of the approach): the Master's own
 *     SCITransferControl() (SCIMasterTransfer.c) always fully drains one
 *     command's data - looping the SAME command number's request - before
 *     the public API could ever issue a different command number. There is
 *     therefore no way to reach the Slave's "different command number
 *     arrives while a previous command's data transfer is still ongoing"
 *     branch through a single coherent Master-driven flow
 *     (SCIRequestCommand() etc.), nor even through raw byte injection into
 *     SCISlaveReceiveData()/SCISlaveStatemachine(): with this test suite's
 *     SCIconfig.h (TX_PACKET_LENGTH=128, MAX_NUM_RESPONSE_VALUES=10) every
 *     fixture command's SUCCESS_DATA payload (at most 10 values, ~90 ASCII
 *     bytes) fits into a single TX packet, so the Slave's response-builder
 *     step always clears the "ongoing" flag again within the very same
 *     statemachine cycle that set it - there is no multi-cycle window in
 *     which a raw follow-up request could observe ongoing==true for a
 *     still-incomplete transfer without editing the fixture data (out of
 *     scope for this task). SCISlaveTransferProcessRequest() is called
 *     directly instead, mirroring the exact call sequence
 *     SCISlaveStatemachine() uses: SCISlaveTransferInitiateResponse()
 *     immediately before SCISlaveTransferProcessRequest() on every request.
 *
 *     FIXED BUG (previously pinned here as a known issue, see git history
 *     for the original test body): SCISlaveTransferInitiateResponse()
 *     unconditionally stamps psTransfer->sResponseControl.sRsp.i16Num from
 *     the *current* incoming request's own number, on every request,
 *     before SCISlaveTransferProcessRequest() runs. The COMMAND case's
 *     `bNewCmd` check and the UPSTREAM case's number-match check used to
 *     compare against that same field - which is therefore always equal to
 *     sReq.i16Num by construction and can never observe a genuine number
 *     change. Fixed by tracking the owning command number separately (see
 *     tsRESPONSECONTROL.i16TransferCmdNum in SCISlaveTransfer.h), which is
 *     written only when a request is recognized as genuinely new and left
 *     alone by continuation calls. The tests below now assert the
 *     corrected behavior: a new command number arriving mid-transfer is
 *     recognized as fresh (its own callback runs, its own response shape
 *     is returned), the same command number continues the existing
 *     transfer as before, and an Upstream request is only accepted when
 *     its number matches the COMMAND that actually granted the transfer.
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
 * Part 2: "new command/upstream number while a prior transfer is still
 * ongoing" - Slave-only unit tests of SCISlaveTransferProcessRequest().
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
// exactly as it would be mid-transfer in the real statemachine. This must
// be recognized as a genuinely new command: testCmdPlain's own callback
// must run and its own response shape (plain SUCCESS, no data) must be
// what's returned - not command 2's stale SUCCESS_DATA/ongoing state.
TEST_F(SlaveTransferProcessRequestTest, NewCommandNumberIsRecognizedAsFreshMidTransfer) {
    tsREQUEST sReqFirst = {};
    sReqFirst.eReqType     = eREQUEST_TYPE_COMMAND;
    sReqFirst.i16Num       = 2;          // testCmdData
    sReqFirst.ui8ValArrLen = 0;

    SCISlaveTransferInitiateResponse(&sTransfer, sReqFirst.i16Num, sReqFirst.eReqType);
    teSCI_SLAVE_ERROR eError1 = SCISlaveTransferProcessRequest(&sTransfer, &sVarAccess, sReqFirst);

    ASSERT_EQ(eSCI_SLAVE_ERROR_NONE, eError1);
    ASSERT_TRUE(sTransfer.sResponseControl.ui8ControlBits.ongoing)
        << "Test setup assumption broken - testCmdData's SUCCESS_DATA response should mark the transfer ongoing";
    EXPECT_EQ(eREQUEST_ACK_STATUS_SUCCESS_DATA, sTransfer.sResponseControl.sRsp.eReqAck);
    EXPECT_EQ(3u, sTransfer.sResponseControl.sRsp.sTransferData.ui32DatLen);
    EXPECT_TRUE(sTransfer.sResponseControl.ui8ControlBits.firstPacketNotSent);

    // Second request: a DIFFERENT command number, arriving while the first
    // command's "ongoing" data transfer state is still in place.
    tsREQUEST sReqSecond = {};
    sReqSecond.eReqType     = eREQUEST_TYPE_COMMAND;
    sReqSecond.i16Num       = 3;          // testCmdPlain - deliberately different from 2
    sReqSecond.ui8ValArrLen = 0;

    SCISlaveTransferInitiateResponse(&sTransfer, sReqSecond.i16Num, sReqSecond.eReqType);
    teSCI_SLAVE_ERROR eError2 = SCISlaveTransferProcessRequest(&sTransfer, &sVarAccess, sReqSecond);

    ASSERT_EQ(eSCI_SLAVE_ERROR_NONE, eError2);
    EXPECT_EQ(3, sTransfer.sResponseControl.sRsp.i16Num);

    // testCmdPlain's own response shape must win: plain SUCCESS, zero data,
    // fresh-command control bits - not command 2's leftover state.
    EXPECT_EQ(eREQUEST_ACK_STATUS_SUCCESS, sTransfer.sResponseControl.sRsp.eReqAck)
        << "testCmdPlain's callback must actually run and set its own ack";
    EXPECT_EQ(0u, sTransfer.sResponseControl.sRsp.sTransferData.ui32DatLen)
        << "testCmdPlain sets ui32DatLen to 0 - command 2's leftover length must not survive";
    EXPECT_TRUE(sTransfer.sResponseControl.ui8ControlBits.firstPacketNotSent)
        << "A genuinely fresh command must set firstPacketNotSent, not clear it";
    EXPECT_FALSE(sTransfer.sResponseControl.ui8ControlBits.ongoing)
        << "testCmdPlain sets no data - the ongoing bit from command 2 must not survive";
}

// Companion to the test above: the SAME command number arriving while its
// own data transfer is still ongoing must still be treated as a
// continuation, not spuriously flagged as a new command.
TEST_F(SlaveTransferProcessRequestTest, SameCommandNumberContinuesOngoingTransfer) {
    tsREQUEST sReqFirst = {};
    sReqFirst.eReqType     = eREQUEST_TYPE_COMMAND;
    sReqFirst.i16Num       = 2;          // testCmdData
    sReqFirst.ui8ValArrLen = 0;

    SCISlaveTransferInitiateResponse(&sTransfer, sReqFirst.i16Num, sReqFirst.eReqType);
    ASSERT_EQ(eSCI_SLAVE_ERROR_NONE, SCISlaveTransferProcessRequest(&sTransfer, &sVarAccess, sReqFirst));
    ASSERT_TRUE(sTransfer.sResponseControl.ui8ControlBits.ongoing);

    // Same command number (2) arrives again while still ongoing - e.g. the
    // Master re-requesting the next chunk of the same command's data.
    tsREQUEST sReqRepeat = {};
    sReqRepeat.eReqType     = eREQUEST_TYPE_COMMAND;
    sReqRepeat.i16Num       = 2;
    sReqRepeat.ui8ValArrLen = 0;

    SCISlaveTransferInitiateResponse(&sTransfer, sReqRepeat.i16Num, sReqRepeat.eReqType);
    teSCI_SLAVE_ERROR eError = SCISlaveTransferProcessRequest(&sTransfer, &sVarAccess, sReqRepeat);

    ASSERT_EQ(eSCI_SLAVE_ERROR_NONE, eError);
    // Continuation branch explicitly sets firstPacketNotSent = false and
    // does not re-run the command callback or touch the ack/data-length
    // fields.
    EXPECT_FALSE(sTransfer.sResponseControl.ui8ControlBits.firstPacketNotSent)
        << "Same command number mid-transfer must be treated as a continuation";
    EXPECT_EQ(eREQUEST_ACK_STATUS_SUCCESS_DATA, sTransfer.sResponseControl.sRsp.eReqAck);
    EXPECT_EQ(3u, sTransfer.sResponseControl.sRsp.sTransferData.ui32DatLen);
    EXPECT_TRUE(sTransfer.sResponseControl.ui8ControlBits.ongoing);
}

// UPSTREAM sibling of the same bug: an Upstream request whose number
// doesn't match the COMMAND that actually granted the transfer must be
// rejected. Previously impossible to construct as a genuine mismatch (see
// SlaveErrorTests.cpp) because the check compared against a field that was
// always overwritten to equal the incoming request's own number.
TEST_F(SlaveTransferProcessRequestTest, UpstreamRequestWithMismatchedNumberIsRejected) {
    // Command 1 (testCmdUpstream) grants an upstream transfer under number 1.
    tsREQUEST sCmdReq = {};
    sCmdReq.eReqType = eREQUEST_TYPE_COMMAND;
    sCmdReq.i16Num   = 1;

    SCISlaveTransferInitiateResponse(&sTransfer, sCmdReq.i16Num, sCmdReq.eReqType);
    ASSERT_EQ(eSCI_SLAVE_ERROR_NONE, SCISlaveTransferProcessRequest(&sTransfer, &sVarAccess, sCmdReq));
    ASSERT_TRUE(sTransfer.sResponseControl.ui8ControlBits.upstream)
        << "Test setup assumption broken - testCmdUpstream should grant an upstream transfer";

    // Upstream request under the WRONG number (2 - no command 2 was issued here).
    tsREQUEST sUpBad = {};
    sUpBad.eReqType = eREQUEST_TYPE_UPSTREAM;
    sUpBad.i16Num   = 2;

    SCISlaveTransferInitiateResponse(&sTransfer, sUpBad.i16Num, sUpBad.eReqType);
    teSCI_SLAVE_ERROR eBadError = SCISlaveTransferProcessRequest(&sTransfer, &sVarAccess, sUpBad);

    EXPECT_EQ(eSCI_SLAVE_ERROR_UPSTREAM_NOT_INITIATED, eBadError)
        << "Upstream request number must match the command that granted the transfer";

    // Upstream request under the CORRECT number (1) must still succeed.
    tsREQUEST sUpGood = {};
    sUpGood.eReqType = eREQUEST_TYPE_UPSTREAM;
    sUpGood.i16Num   = 1;

    SCISlaveTransferInitiateResponse(&sTransfer, sUpGood.i16Num, sUpGood.eReqType);
    teSCI_SLAVE_ERROR eGoodError = SCISlaveTransferProcessRequest(&sTransfer, &sVarAccess, sUpGood);

    EXPECT_EQ(eSCI_SLAVE_ERROR_NONE, eGoodError);
    EXPECT_EQ(eREQUEST_ACK_STATUS_SUCCESS, sTransfer.sResponseControl.sRsp.eReqAck);
}

}  // namespace
