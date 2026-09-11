/**************************************************************************//**
 * \file SlaveParameterLengthTests.cpp
 * \author Roman Holderried
 *
 * \brief Raw-byte Slave tests proving MAX_NUMBER_OF_PARAMETER_DIGITS is
 *        enforced by SCISlaveRequestParser(): a SetVar value exactly at
 *        the bound is accepted (ACK), one character over is rejected with
 *        eSCI_SLAVE_ERROR_REQUEST_VALUE_TOO_LONG (wire: "ERR;10C" - the
 *        new error is appended after the 11 existing teSCI_SLAVE_ERROR
 *        values, enum index 12, so GET_SCI_ERROR_NUMBER(12) = 0x100 + 12 =
 *        0x10C - see SCICommon.h). No value ever gets truncated: an
 *        oversized value is rejected outright, before any copy happens.
 *        See SCITransferCommon.h for the macro and SCISlaveDataframe.c's
 *        SCISlaveRequestParser() for the rejection logic.
 *
 * <b> History </b>
 * \t- 2026-09-11 - File creation.
 *****************************************************************************/
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

// Own fixture name to avoid a duplicate-test-suite clash with SlaveTests.cpp
// / SlaveErrorTests.cpp, which live in the same test binary.
class SlaveParameterLengthTest : public ::testing::Test {
protected:
    void SetUp() override {
        SCISlaveInit(sSlaveTestCbs, varStruct, cmdStruct);
    }
};

// Pumps the slave statemachine, injecting one message byte per tick,
// mirroring the loop shape used in SlaveTests.cpp / SlaveErrorTests.cpp.
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

// Var 5 (i32_test, eDTYPE_INT32, RAM). Value is exactly 8 hex characters -
// MAX_NUMBER_OF_PARAMETER_DIGITS (C/Test/config/SCIconfig.h) - so this must
// still succeed exactly as before this fix (regression/boundary check).
TEST_F(SlaveParameterLengthTest, SetVarValueAtLengthLimitIsAccepted) {
    uint8_t msg[]    = {0x02, '5', '!', '1','1','2','2','3','3','4','4', 0x03};
    uint8_t expect[] = {0x02, '5', '!', 'A', 'C', 'K', 0x03};

    PumpSlave(msg, sizeof(msg));

    EXPECT_EQ(0, memcmp(expect, cTxMsgBuf, sizeof(expect)));
}

// Same var, value is 9 hex characters - one over the bound. Before this
// fix, SCISlaveRequestParser() malloc'd a buffer sized to the full 9
// characters and passed it whole to strToHex(), which independently caps
// at 8 nibbles and would have rejected it as
// eSCI_SLAVE_ERROR_REQUEST_VALUE_CONVERSION_FAILED (wire "ERR;109"). After
// this fix, the length check trips first, before any copy or conversion
// attempt, with the more specific eSCI_SLAVE_ERROR_REQUEST_VALUE_TOO_LONG
// (wire "ERR;10C").
TEST_F(SlaveParameterLengthTest, SetVarValueOverLengthLimitIsRejected) {
    uint8_t msg[]    = {0x02, '5', '!', '1','1','2','2','3','3','4','4','5', 0x03};
    uint8_t expect[] = {0x02, '5', '!', 'E', 'R', 'R', ';', '1', '0', 'C', 0x03};

    PumpSlave(msg, sizeof(msg));

    ASSERT_EQ(0, memcmp(expect, cTxMsgBuf, sizeof(expect)));

    // The rejected request must not leave the Slave's state machine stuck:
    // a subsequent well-formed request has to work exactly as it would
    // from a clean start. Only checks the ACK prefix (not the trailing
    // value bytes) - the exact value of var 3 is not hermetic when this
    // binary runs its full suite in one process (RoundTripSetVarTest
    // mutates the same global variables the var struct points to, and
    // GoogleTest does not reset globals between test suites), so this
    // check intentionally does not depend on it, only on "the Slave
    // answered a subsequent request with a normal ACK, not stuck echoing
    // the prior error".
    uint8_t okMsg[]      = {0x02, '3', '?', 0x03};
    uint8_t okAckPrefix[] = {0x02, '3', '?', 'A', 'C', 'K', ';'};

    PumpSlave(okMsg, sizeof(okMsg));

    EXPECT_EQ(0, memcmp(okAckPrefix, cTxMsgBuf, sizeof(okAckPrefix)));
}

}  // namespace
