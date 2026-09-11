/**************************************************************************//**
 * \file MasterDataframeParserTests.cpp
 * \author Roman Holderried
 *
 * \brief Direct unit tests for SCIMasterResponseParser(), calling it
 *        standalone (no Master/Slave statemachine, no fixture) with
 *        hand-built response buffers. Direct calls are required here
 *        rather than a round-trip test because SCIMaster.c's
 *        ePROTOCOL_EVALUATING case currently discards this function's
 *        teSCI_MASTER_ERROR return value entirely - a pre-existing gap,
 *        not fixed by this file.
 *
 *        Covers both remaining per-field malloc sites converted to fixed
 *        stack buffers, each bounded by MAX_NUMBER_OF_PARAMETER_DIGITS
 *        (SCITransferCommon.h, required from SCIconfig.h): the "control
 *        number" parsed right after the acknowledge (GetVar's return
 *        value / a Command's data length or error number), and the
 *        per-comma-separated-value loop (a Command's multi-value DATA
 *        response).
 *
 * <b> History </b>
 * \t- 2026-09-11 - File creation.
 *****************************************************************************/
#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "SCIMasterDataframe.h"
#include "SCITransferCommon.h"
#include "SCICommon.h"
}

namespace {

// GetVar's return value is parsed by the "control number after the
// acknowledge" site. A short, normal-length value must keep working
// exactly as before this fix - pure regression check, mirrors
// SlaveTests.cpp's PollVarUI8 wire shape (buffer excludes STX/ETX, which
// the Datalink layer strips before this parser ever sees the bytes).
TEST(MasterDataframeParserTest, GetVarNormalValueStillParses) {
    uint8_t buf[] = {'5','?','A','C','K',';','F','5'};
    tsRESPONSE sRsp = tsRESPONSE_DEFAULTS;
    uint8_t ui8MsgDataLen = 0;

    teSCI_MASTER_ERROR eError = SCIMasterResponseParser(buf, sizeof(buf), &ui8MsgDataLen, &sRsp);

    ASSERT_EQ(eSCI_MASTER_ERROR_NONE, eError);
    EXPECT_EQ(eREQUEST_TYPE_GETVAR, sRsp.eReqType);
    EXPECT_EQ(eREQUEST_ACK_STATUS_SUCCESS, sRsp.eReqAck);
    EXPECT_EQ(5, sRsp.i16Num);
    EXPECT_EQ(0xF5u, sRsp.sTransferData.puRespVals[0].ui32_hex);
}

// Same wire shape, value is exactly 8 hex characters -
// MAX_NUMBER_OF_PARAMETER_DIGITS (C/Test/config/SCIconfig.h). Must still
// succeed - boundary/regression check (today's strToHex() already accepts
// up to 8 nibbles, so this isn't new behavior, just proof the new bound
// check doesn't reject the legal case).
TEST(MasterDataframeParserTest, GetVarValueAtLengthLimitIsAccepted) {
    uint8_t buf[] = {'5','?','A','C','K',';','1','1','2','2','3','3','4','4'};
    tsRESPONSE sRsp = tsRESPONSE_DEFAULTS;
    uint8_t ui8MsgDataLen = 0;

    teSCI_MASTER_ERROR eError = SCIMasterResponseParser(buf, sizeof(buf), &ui8MsgDataLen, &sRsp);

    ASSERT_EQ(eSCI_MASTER_ERROR_NONE, eError);
    EXPECT_EQ(5, sRsp.i16Num);
    EXPECT_EQ(0x11223344u, sRsp.sTransferData.puRespVals[0].ui32_hex);
}

// Same wire shape, value is 9 hex characters - one over the bound. Before
// this fix, the "control number" site malloc'd a buffer sized to the full
// 9 characters and passed it whole to strToHex(), which independently
// caps at 8 nibbles and would have returned
// eSCI_MASTER_ERROR_PARAMETER_CONVERSION_FAILED. After this fix, the
// length check trips first, before any copy or conversion attempt, with
// the more specific eSCI_MASTER_ERROR_PARAMETER_TOO_LONG.
TEST(MasterDataframeParserTest, GetVarValueOverLengthLimitIsRejected) {
    uint8_t buf[] = {'5','?','A','C','K',';','1','1','2','2','3','3','4','4','5'};
    tsRESPONSE sRsp = tsRESPONSE_DEFAULTS;
    uint8_t ui8MsgDataLen = 0;

    teSCI_MASTER_ERROR eError = SCIMasterResponseParser(buf, sizeof(buf), &ui8MsgDataLen, &sRsp);

    EXPECT_EQ(eSCI_MASTER_ERROR_PARAMETER_TOO_LONG, eError);
}

// COMMAND SUCCESS_DATA response carrying 3 values, matching the exact wire
// shape RoundTripCommandTests.cpp's CommandDataRoundTrip proves the Slave
// really emits (<num>:DAT;<count>;<v1>,<v2>,<v3>) - but with the first
// value stretched to 9 hex characters (one over
// MAX_NUMBER_OF_PARAMETER_DIGITS) to exercise the per-value-loop rejection
// path. The boundary case (all three values at exactly 8 characters) is
// already covered by CommandDataRoundTrip itself and is deliberately not
// duplicated here.
TEST(MasterDataframeParserTest, CommandMultiValueOverlongFirstValueIsRejected) {
    uint8_t buf[] = {
        '2',':','D','A','T',';','3',';',
        '1','1','2','2','3','3','4','4','5', ',',
        '2','2','2','2','2','2','2','2', ',',
        '3','3','3','3','3','3','3','3'
    };
    tsRESPONSE sRsp = tsRESPONSE_DEFAULTS;
    uint8_t ui8MsgDataLen = 0;

    teSCI_MASTER_ERROR eError = SCIMasterResponseParser(buf, sizeof(buf), &ui8MsgDataLen, &sRsp);

    EXPECT_EQ(eSCI_MASTER_ERROR_PARAMETER_TOO_LONG, eError);
}

}  // namespace
