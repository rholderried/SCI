#include <gtest/gtest.h>

extern "C" {
#include "SCISlave.h"
#include "SCIMaster.h"
}

extern "C" {
    extern tsSCIVAR varStruct[];
    extern COMMAND_CB cmdStruct[];
    extern tsSCI_SLAVE_CALLBACKS sSlaveTestCbs;
    extern char cTxMsgBuf[];
    extern char cRxMsgBuf[];
}

namespace {

// Own fixture name (not "SlaveTest") to avoid a duplicate-test-suite clash
// with SlaveTests.cpp, which lives in the same test binary.
class SlaveErrorTest : public ::testing::Test {
protected:
    void SetUp() override {
        SCISlaveInit(sSlaveTestCbs, varStruct, cmdStruct);
    }
};

// Pumps the slave statemachine, injecting one message byte per tick,
// mirroring the loop shape used in SlaveTests.cpp.
static void PumpSlave(const uint8_t* msg, uint8_t msgLen, int numLoops = 100) {
    uint8_t j = 0;
    for (int i = 0; i < numLoops; i++) {
        if (j < msgLen) {
            SCISlaveReceiveData(msg[j]);
            j++;
        }
        SCISlaveStatemachine();
    }
}

// These tests drive the Slave directly with raw protocol bytes to reach
// error paths that the Master's public API cannot construct on purpose
// (the Master never emits a bare Upstream request without having first
// issued a matching Command, and never emits an unrecognized request
// identifier). Expected byte sequences below were derived by reading
// SCISlave.c / SCISlaveTransfer.c / SCISlaveDataframe.c and confirmed by
// running the tests; see the reasoning comments inline.
//
// Wire-level error encoding: for eReqAck == ERROR (or UNKNOWN with a zero
// error number) the dataframe builder (SCISlaveDataframe.c) writes
// "ERR;" followed by the hex-encoded value of
// GET_SCI_ERROR_NUMBER(eSCI_SLAVE_ERROR_xxx) == (0x100 + enum value),
// hex-encoded with leading zero nibbles shrunk away by hexToStrWord().
//   eSCI_SLAVE_ERROR_UPSTREAM_NOT_INITIATED           = 11 -> 0x100+11 = 0x10B
//   eSCI_SLAVE_ERROR_REQUEST_IDENTIFIER_NOT_FOUND     = 7  -> 0x100+7  = 0x107

TEST_F(SlaveErrorTest, UpstreamWithoutPriorCommandReturnsError) {
    // Fresh SetUp() guarantees no command has ever been processed, so
    // tsRESPONSECONTROL_DEFAULTS still holds: ui8ControlBits.upstream == false.
    //
    // SCISlaveTransferProcessRequest()'s UPSTREAM case checks
    //   sResponseControl.i16TransferCmdNum == sReq.i16Num && ui8ControlBits.upstream == true
    // (see SCISlaveTransfer.c/.h - i16TransferCmdNum tracks which command
    // actually granted the transfer, fixed 2026-09-11; the number check used
    // to be dead code before that fix). With no prior command,
    // ui8ControlBits.upstream is false, so this trips
    // eSCI_SLAVE_ERROR_UPSTREAM_NOT_INITIATED (enum value 11 -> wire error
    // number 0x100 + 11 = 0x10B) regardless of the request number.
    uint8_t msg[]    = {0x02, '1', '>', 0x03};
    uint8_t expect[] = {0x02, '1', '>', 'E', 'R', 'R', ';', '1', '0', 'B', 0x03};

    PumpSlave(msg, sizeof(msg));

    EXPECT_EQ(0, memcmp(expect, cTxMsgBuf, sizeof(expect)));
}

TEST_F(SlaveErrorTest, UpstreamAfterNonUpstreamCommandReturnsError) {
    // Command 3 (testCmdPlain) returns eREQUEST_ACK_STATUS_SUCCESS with no
    // data/upstream payload, so it never sets ui8ControlBits.upstream, and
    // since ui32DatLen == 0 after this exchange the Slave clears the whole
    // response-control structure at the end of the EVALUATING step.
    uint8_t cmdMsg[]    = {0x02, '3', ':', 0x03};
    uint8_t cmdExpect[] = {0x02, '3', ':', 'A', 'C', 'K', 0x03};

    PumpSlave(cmdMsg, sizeof(cmdMsg));
    ASSERT_EQ(0, memcmp(cmdExpect, cTxMsgBuf, sizeof(cmdExpect)))
        << "precondition failed: Command 3 did not ACK as expected";

    // This test exercises the "upstream bit never set" path: issuing an
    // Upstream request after a command that completed without granting an
    // upstream transfer. See
    // RoundTripSequenceTests.cpp's UpstreamRequestWithMismatchedNumberIsRejected
    // for the sibling case (a genuine number mismatch against a command
    // that DID grant an upstream transfer) - that case used to be
    // impossible to construct here because the number check was dead code;
    // it's now fixed and covered there via direct
    // SCISlaveTransferProcessRequest() calls instead of raw byte injection.
    uint8_t upMsg[]    = {0x02, '3', '>', 0x03};
    uint8_t upExpect[] = {0x02, '3', '>', 'E', 'R', 'R', ';', '1', '0', 'B', 0x03};

    PumpSlave(upMsg, sizeof(upMsg));

    EXPECT_EQ(0, memcmp(upExpect, cTxMsgBuf, sizeof(upExpect)));
}

TEST_F(SlaveErrorTest, UnknownIdentifierReturnsError) {
    // '@' is not one of the recognized request identifiers ('?','!',':','>','<'),
    // so SCISlaveRequestParser() returns eSCI_SLAVE_ERROR_REQUEST_IDENTIFIER_NOT_FOUND
    // (enum value 7) without ever setting sReq.eReqType away from its default
    // eREQUEST_TYPE_NONE, and without parsing a request number (i16Num stays 0).
    //
    // Contrary to a naive reading of the dataframe builder's ERR/NAK switch
    // (which special-cases sTransferData.ui16Error == 0 to write "NAK"), the
    // error number here is never 0: SCISlave.c only calls
    // SCISlaveTransferSetError() when eError != eSCI_SLAVE_ERROR_NONE, and it
    // always passes GET_SCI_ERROR_NUMBER(eError) = 0x100 + eError, which is
    // nonzero for every real error code. So this path produces "ERR;<hex>",
    // not "NAK" -- the NAK branch is effectively dead code given the current
    // callers. Wire error number: 0x100 + 7 = 0x107. The echoed request
    // number is "0" (i16Num defaulted, never parsed) and the echoed request
    // identifier is '#' (UNKNOWN_IDENTIFIER, since eReqType stayed NONE).
    uint8_t msg[]    = {0x02, '1', '@', 0x03};
    uint8_t expect[] = {0x02, '0', '#', 'E', 'R', 'R', ';', '1', '0', '7', 0x03};

    PumpSlave(msg, sizeof(msg));

    EXPECT_EQ(0, memcmp(expect, cTxMsgBuf, sizeof(expect)));

    // The malformed message must not leave the Slave's state machine stuck:
    // a subsequent well-formed request has to work exactly as it would from
    // a clean start (same as SlaveTest.PollVarUI8).
    uint8_t okMsg[]    = {0x02, '3', '?', 0x03};
    uint8_t okExpect[] = {0x02, '3', '?', 'A', 'C', 'K', ';', 'F', '5', 0x03};

    PumpSlave(okMsg, sizeof(okMsg));

    EXPECT_EQ(0, memcmp(okExpect, cTxMsgBuf, sizeof(okExpect)));
}

}  // namespace
