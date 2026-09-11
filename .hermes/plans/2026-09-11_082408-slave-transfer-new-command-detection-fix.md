# Fix: Slave doesn't detect a new COMMAND/UPSTREAM number mid-transfer

## Goal

Make `SCISlaveTransferProcessRequest()` in `C/Slave/Src/SCISlaveTransfer.c`
correctly distinguish "genuinely new COMMAND/UPSTREAM request number" from
"continuation of the still-ongoing transfer", instead of comparing against a
response field that has already been overwritten to always match.

## Current context / assumptions

This bug was found and pinned (not fixed) while writing round-trip tests,
documented in `C/Test/gtest/RoundTripSequenceTests.cpp` (file header, Part 2)
and cross-referenced from `C/Test/gtest/SlaveErrorTests.cpp`
(`UpstreamWithoutPriorCommandReturnsError` /
`UpstreamAfterNonUpstreamCommandReturnsError` comments). Also documented in
`.hermes.md`'s "Known bug, deliberately pinned by a test rather than fixed"
section — **update that section once this fix lands** (see Task 8 below).

Root cause, read from the actual call sequence in
`C/Slave/Src/SCISlave.c:131` (`ePROTOCOL_EVALUATING` case):

```c
// Take over command number and type
SCISlaveTransferInitiateResponse(&sSciSlave.sSciTransfer, sReq.i16Num, sReq.eReqType);

// Execute the command
if (eError == eSCI_SLAVE_ERROR_NONE)
    eError = SCISlaveTransferProcessRequest(&sSciSlave.sSciTransfer, &sSciSlave.sVarAccess, sReq);
```

`SCISlaveTransferInitiateResponse()` unconditionally stamps
`psTransfer->sResponseControl.sRsp.i16Num = i16Num` (the *incoming* request's
own number) on **every** request, of every type, before
`SCISlaveTransferProcessRequest()` ever runs. Two places inside
`SCISlaveTransferProcessRequest()` then try to compare that same field
against `sReq.i16Num` to detect "did the request number change since the
in-flight transfer started":

1. `SCISlaveTransfer.c:125` (COMMAND case, the `bNewCmd` check):
   `psTransfer->sResponseControl.sRsp.i16Num != sReq.i16Num`
2. `SCISlaveTransfer.c:176` (UPSTREAM case):
   `psTransfer->sResponseControl.sRsp.i16Num == sReq.i16Num`

Both comparisons are dead code: by the time they run, `sRsp.i16Num` has
*already* been set to `sReq.i16Num` for *this* call, so both sides of the
comparison are always equal, every single time, regardless of what number
was in flight before. Concretely:

- **COMMAND**: `bNewCmd` collapses to just `ongoing == false`. A new command
  number arriving while a previous command's data transfer is still
  `ongoing` is silently merged into the old command's state (old command's
  stale ack/data/ongoing bits get returned; the new command's callback never
  runs).
- **UPSTREAM**: the number-match half of the check is always true, so only
  `ui8ControlBits.upstream` actually gates the request. An Upstream request
  with the wrong number (relative to whichever COMMAND actually granted the
  transfer) is never rejected for that reason — it's currently *impossible*
  to even construct a test that exercises a genuine number mismatch here
  (see `SlaveErrorTests.cpp` comments).

Confirmed present state (build + test both currently green, 25/25):
```
cd C
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
cd build && ctest --output-on-failure
```
`SlaveTransferProcessRequestTest.NewCommandNumberIsNotRecognizedAsFreshMidTransfer`
(in `RoundTripSequenceTests.cpp`) currently passes *because* it pins the
buggy behavior as expected. This plan flips it to require the fixed
behavior instead.

Relevant fixture data (`C/Test/VariablesAndCommands.c`, `SIZE_OF_CMD_STRUCT
= 5`, `C/Test/config/SCIconfig.h`):
- Command 1 = `testCmdUpstream` → `eREQUEST_ACK_STATUS_SUCCESS_UPSTREAM`,
  20-byte payload → sets `ui8ControlBits.upstream = true`.
- Command 2 = `testCmdData` → `eREQUEST_ACK_STATUS_SUCCESS_DATA`,
  `ui32DatLen = 3` → sets `ui8ControlBits.ongoing = true`.
- Command 3 = `testCmdPlain` → `eREQUEST_ACK_STATUS_SUCCESS`,
  `ui32DatLen = 0` → sets neither bit.

## Architecture / proposed approach

Add a new field, `i16TransferCmdNum`, to `tsRESPONSECONTROL`
(`C/Slave/Inc/SCISlaveTransfer.h`) that records *which* command number owns
the currently in-flight `ongoing`/`upstream` transfer. It is written only
when `SCISlaveTransferProcessRequest()`'s COMMAND case decides a request is
genuinely new (the `if (bNewCmd)` branch), and is left untouched by
continuation calls — unlike `sRsp.i16Num`, which is forced to match every
incoming request by `SCISlaveTransferInitiateResponse()` regardless of
caller intent. Both the COMMAND `bNewCmd` check and the UPSTREAM
number-match check are switched to compare against
`i16TransferCmdNum` instead of `sRsp.i16Num`; this is the same defect in
both places (comments in the test files already say so), so both get fixed
together as one root-cause change. `SCISlaveTransferClearResponseControl()`
already zeroes the whole struct via `tsRESPONSECONTROL_DEFAULTS`, so the new
field resets for free once a transfer fully drains — no change needed there
beyond updating the defaults macro.

## Step-by-step tasks

### Task 0 — Branch

```
git status --short
git checkout -b fix/slave-transfer-new-command-detection
```
Expected: clean switch, no uncommitted changes lost (repo workflow rule:
never commit on `master`).

### Task 1 — Baseline build/test (confirm starting point)

```
cd C
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
cd build && ctest --output-on-failure
```
Expected: `100% tests passed, 0 tests failed out of 25`.

### Task 2 (RED) — Flip the pinned bug test to require the fix

File: `C/Test/gtest/RoundTripSequenceTests.cpp`.

**2a. Replace the file header's Part 2 description** (currently describes
the bug as an "UNEXPECTED FINDING... not fixed here"). Find the whole
comment block from `/*******...` (top of file, line 1) through the closing
`******/` right before `#include "RoundTripFixture.h"` (line 74), and
replace the entire block with:

```c
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
```

**2b. Replace the Part 2 section header comment** (currently starting
`/****... * Part 2: "new command number while a prior command's data
transfer is still ongoing" ...`). Find:

```c
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
```

Replace with:

```c
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
```

**2c. Replace the whole pinned-bug test** (`TEST_F(SlaveTransferProcessRequestTest,
NewCommandNumberIsNotRecognizedAsFreshMidTransfer)`, currently the last
`TEST_F` before the closing `}  // namespace`) with three tests: the
existing setup rewritten to assert the fix, plus a same-number-continuation
regression test, plus the new UPSTREAM mismatch test. Find the whole block
from:

```c
// Command 2 (testCmdData) returns eREQUEST_ACK_STATUS_SUCCESS_DATA with
```
through the end of the old test's closing `}` (just before
`}  // namespace`), and replace it with:

```c
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
```

**2d. Build and run — expect the new/renamed tests to FAIL** against the
still-unfixed production code:

```
cd C && cmake --build build 2>&1 | tail -n 20
cd build && ctest -R SlaveTransferProcessRequestTest --output-on-failure
```
Expected: `NewCommandNumberIsRecognizedAsFreshMidTransfer` and
`UpstreamRequestWithMismatchedNumberIsRejected` FAIL (production code still
has the bug); `SameCommandNumberContinuesOngoingTransfer` passes already
(continuation behavior is unaffected by the bug). If the build itself
fails, fix the C++ syntax before proceeding — do not skip ahead with a
non-compiling test file.

### Task 3 (GREEN) — Add the tracking field

File: `C/Slave/Inc/SCISlaveTransfer.h`. Find:

```c
typedef struct
{
    union
    {
        struct
        {
            uint8_t firstPacketNotSent  : 1;
            uint8_t ongoing             : 1;
            uint8_t upstream            : 1;
            uint8_t reserved            : 5;
        }ui8ControlBits;
        
        uint8_t ui8ControlByte;
    };
    uint32_t    ui32DataIdx;
    tsRESPONSE  sRsp;
}tsRESPONSECONTROL;

#define tsRESPONSECONTROL_DEFAULTS {{.ui8ControlByte = 0}, 0, tsRESPONSE_DEFAULTS}
```

Replace with:

```c
typedef struct
{
    union
    {
        struct
        {
            uint8_t firstPacketNotSent  : 1;
            uint8_t ongoing             : 1;
            uint8_t upstream            : 1;
            uint8_t reserved            : 5;
        }ui8ControlBits;
        
        uint8_t ui8ControlByte;
    };
    uint32_t    ui32DataIdx;
    int16_t     i16TransferCmdNum; /*!< Number of the COMMAND request that owns the
                                         current ongoing/upstream transfer. Set only
                                         when SCISlaveTransferProcessRequest()'s COMMAND
                                         case recognizes a genuinely new request (the
                                         bNewCmd branch); left untouched by continuation
                                         calls. Deliberately NOT the same as sRsp.i16Num,
                                         which SCISlaveTransferInitiateResponse()
                                         unconditionally overwrites from every incoming
                                         request before ProcessRequest() runs - comparing
                                         against sRsp.i16Num is always trivially true and
                                         can never detect a genuine number change.*/
    tsRESPONSE  sRsp;
}tsRESPONSECONTROL;

#define tsRESPONSECONTROL_DEFAULTS {{.ui8ControlByte = 0}, 0, 0, tsRESPONSE_DEFAULTS}
```

Also add a History bullet at the top of the file (after the existing
`2022-12-11` line):
```c
 *  - 2026-09-11 - Added i16TransferCmdNum to tsRESPONSECONTROL and fixed
 *                 SCISlaveTransferProcessRequest()'s COMMAND/UPSTREAM
 *                 number-comparison checks to use it instead of
 *                 sRsp.i16Num (see SCISlaveTransfer.c history for details).
```

### Task 4 (GREEN) — Fix the comparisons

File: `C/Slave/Src/SCISlaveTransfer.c`. Find the COMMAND case:

```c
        case eREQUEST_TYPE_COMMAND:
            {
                teREQUEST_ACKNOWLEDGE eReqAck = eREQUEST_ACK_STATUS_UNKNOWN;
                // tsTRANSFER_DATA sTransferData = tsTRANSFER_DATA_DEFAULTS;
                // Determine if a new command has been sent or if the ongoing command is to be processed
                bool bNewCmd = psTransfer->sResponseControl.ui8ControlBits.ongoing == false || (psTransfer->sResponseControl.sRsp.i16Num != sReq.i16Num);

                if (bNewCmd)
                {
                    // Check if a command structure has been passed
                    if (psTransfer->pCmdCBStruct != NULL && sReq.i16Num > 0 && sReq.i16Num <= SIZE_OF_CMD_STRUCT)
                    {
                        // TODO: Support for passing values to the command function
                        #ifdef VALUE_MODE_HEX
                        eReqAck = psTransfer->pCmdCBStruct[sReq.i16Num - 1](&sReq.uValArr[0].ui32_hex,sReq.ui8ValArrLen, &psTransfer->sResponseControl.sRsp.sTransferData);
                        #else
                        eReqAck = psTransfer->pCmdCBStruct[sReq.i16Num - 1](&sReq.uValArr[0].f_float,sReq.ui8ValArrLen, &psTransfer->sResponseControl.sRsp.sTransferData);
                        #endif
                    }
                    else
                    {
                        eError = eSCI_SLAVE_ERROR_REQUEST_UNKNOWN;
                        goto terminate;
                    }

                    // Response is getting sent independently of command success
                    
                    psTransfer->sResponseControl.sRsp.eReqAck          = eReqAck;
                    // psTransfer->sResponseControl.sRsp.sTransferData    = sTransferData;

                    // Fill the response control struct
                    psTransfer->sResponseControl.ui8ControlBits.firstPacketNotSent  = true;
                    psTransfer->sResponseControl.ui32DataIdx                        = 0;
                    
                    // Set the control bits if a data transfer has been initiated
                    psTransfer->sResponseControl.ui8ControlBits.ongoing = 
                        ((psTransfer->sResponseControl.sRsp.eReqAck == eREQUEST_ACK_STATUS_SUCCESS_DATA) && (psTransfer->sResponseControl.sRsp.sTransferData.ui32DatLen > 0));
                    psTransfer->sResponseControl.ui8ControlBits.upstream = 
                        ((psTransfer->sResponseControl.sRsp.eReqAck == eREQUEST_ACK_STATUS_SUCCESS_UPSTREAM) && (psTransfer->sResponseControl.sRsp.sTransferData.ui32DatLen > 0));
                    
                    // Save the response for later
                    // psTransfer->sResponseControl.sRsp = *psRsp;
                    
                }
                else
                {
                    // psRsp->eReqAck          = psTransfer->sResponseControl.sRsp.eReqAck;
                    // psRsp->sTransferData    = psTransfer->sResponseControl.sRsp.sTransferData;
                    psTransfer->sResponseControl.ui8ControlBits.firstPacketNotSent = false;
                }
            }
            break;
        
        case eREQUEST_TYPE_UPSTREAM:

            // Number must match with the previously sent command
            if (psTransfer->sResponseControl.sRsp.i16Num == sReq.i16Num && psTransfer->sResponseControl.ui8ControlBits.upstream == true)
            {
                psTransfer->sResponseControl.sRsp.eReqAck   = eREQUEST_ACK_STATUS_SUCCESS;
                // psRsp->sTransferData          = psTransfer->sResponseControl.sRsp.sTransferData;
                // Change the command type
                // psTransfer->sResponseControl.sRsp.eReqType = psRsp->eReqType;
            }
            // If conditions are not met, provide the minimal information to construct a proper answer
            // TODO: Error handling
            else
            {
                eError = eSCI_SLAVE_ERROR_UPSTREAM_NOT_INITIATED;
                goto terminate;
            }
            break;
```

Replace with:

```c
        case eREQUEST_TYPE_COMMAND:
            {
                teREQUEST_ACKNOWLEDGE eReqAck = eREQUEST_ACK_STATUS_UNKNOWN;
                // tsTRANSFER_DATA sTransferData = tsTRANSFER_DATA_DEFAULTS;
                // Determine if a new command has been sent or if the ongoing command is to be processed.
                // Compare against i16TransferCmdNum (stamped below only when a request is
                // recognized as genuinely new), NOT sRsp.i16Num: SCISlaveTransferInitiateResponse()
                // unconditionally overwrites sRsp.i16Num from every incoming request before this
                // runs, so sRsp.i16Num == sReq.i16Num is always true and can't detect a real change.
                bool bNewCmd = psTransfer->sResponseControl.ui8ControlBits.ongoing == false || (psTransfer->sResponseControl.i16TransferCmdNum != sReq.i16Num);

                if (bNewCmd)
                {
                    // Check if a command structure has been passed
                    if (psTransfer->pCmdCBStruct != NULL && sReq.i16Num > 0 && sReq.i16Num <= SIZE_OF_CMD_STRUCT)
                    {
                        // TODO: Support for passing values to the command function
                        #ifdef VALUE_MODE_HEX
                        eReqAck = psTransfer->pCmdCBStruct[sReq.i16Num - 1](&sReq.uValArr[0].ui32_hex,sReq.ui8ValArrLen, &psTransfer->sResponseControl.sRsp.sTransferData);
                        #else
                        eReqAck = psTransfer->pCmdCBStruct[sReq.i16Num - 1](&sReq.uValArr[0].f_float,sReq.ui8ValArrLen, &psTransfer->sResponseControl.sRsp.sTransferData);
                        #endif
                    }
                    else
                    {
                        eError = eSCI_SLAVE_ERROR_REQUEST_UNKNOWN;
                        goto terminate;
                    }

                    // Response is getting sent independently of command success
                    
                    psTransfer->sResponseControl.sRsp.eReqAck          = eReqAck;
                    // psTransfer->sResponseControl.sRsp.sTransferData    = sTransferData;

                    // Fill the response control struct
                    psTransfer->sResponseControl.ui8ControlBits.firstPacketNotSent  = true;
                    psTransfer->sResponseControl.ui32DataIdx                        = 0;
                    // Record which command number owns this (possibly ongoing/upstream)
                    // transfer, for the bNewCmd/upstream-number checks above and below.
                    psTransfer->sResponseControl.i16TransferCmdNum                  = sReq.i16Num;
                    
                    // Set the control bits if a data transfer has been initiated
                    psTransfer->sResponseControl.ui8ControlBits.ongoing = 
                        ((psTransfer->sResponseControl.sRsp.eReqAck == eREQUEST_ACK_STATUS_SUCCESS_DATA) && (psTransfer->sResponseControl.sRsp.sTransferData.ui32DatLen > 0));
                    psTransfer->sResponseControl.ui8ControlBits.upstream = 
                        ((psTransfer->sResponseControl.sRsp.eReqAck == eREQUEST_ACK_STATUS_SUCCESS_UPSTREAM) && (psTransfer->sResponseControl.sRsp.sTransferData.ui32DatLen > 0));
                    
                    // Save the response for later
                    // psTransfer->sResponseControl.sRsp = *psRsp;
                    
                }
                else
                {
                    // psRsp->eReqAck          = psTransfer->sResponseControl.sRsp.eReqAck;
                    // psRsp->sTransferData    = psTransfer->sResponseControl.sRsp.sTransferData;
                    psTransfer->sResponseControl.ui8ControlBits.firstPacketNotSent = false;
                }
            }
            break;
        
        case eREQUEST_TYPE_UPSTREAM:

            // Number must match the COMMAND that actually granted the upstream transfer.
            // Compare against i16TransferCmdNum, not sRsp.i16Num - see the COMMAND case
            // comment above for why the latter is always trivially equal to sReq.i16Num.
            if (psTransfer->sResponseControl.i16TransferCmdNum == sReq.i16Num && psTransfer->sResponseControl.ui8ControlBits.upstream == true)
            {
                psTransfer->sResponseControl.sRsp.eReqAck   = eREQUEST_ACK_STATUS_SUCCESS;
                // psRsp->sTransferData          = psTransfer->sResponseControl.sRsp.sTransferData;
                // Change the command type
                // psTransfer->sResponseControl.sRsp.eReqType = psRsp->eReqType;
            }
            // If conditions are not met, provide the minimal information to construct a proper answer
            // TODO: Error handling
            else
            {
                eError = eSCI_SLAVE_ERROR_UPSTREAM_NOT_INITIATED;
                goto terminate;
            }
            break;
```

Also add a History bullet at the top of the file (after the existing
`2022-12-11` line):
```c
 *  - 2026-09-11 - Fixed dead-code number comparisons in the COMMAND
 *                 (bNewCmd) and UPSTREAM cases: both used to compare
 *                 against sResponseControl.sRsp.i16Num, which
 *                 SCISlaveTransferInitiateResponse() unconditionally
 *                 overwrites from every incoming request right before this
 *                 function runs, making the comparison always trivially
 *                 true. Now tracked separately via the new
 *                 tsRESPONSECONTROL.i16TransferCmdNum field. Found via SCI
 *                 round-trip test expansion (RoundTripSequenceTests.cpp).
```

### Task 5 — Build and verify GREEN

```
cd C && cmake --build build 2>&1 | tail -n 30
cd build && ctest --output-on-failure
```
Expected: build succeeds with no new warnings/errors, and
`ctest --output-on-failure` reports `100% tests passed` with **28** tests
(25 original + `SameCommandNumberContinuesOngoingTransfer` was already
counted among the 3 replacing 1, so net +2: the renamed
`NewCommandNumberIsRecognizedAsFreshMidTransfer`,
`SameCommandNumberContinuesOngoingTransfer`, and
`UpstreamRequestWithMismatchedNumberIsRejected`). Specifically confirm:
```
ctest -R SlaveTransferProcessRequestTest --output-on-failure
```
shows all 3 `SlaveTransferProcessRequestTest.*` cases passing.

If anything fails, re-check that Task 3's struct/macro edit and Task 4's
two comparison edits were both applied — a partial edit (e.g. struct field
added but `.c` still compares `sRsp.i16Num`) will still compile but leave
the bug in place.

### Task 6 — Commit the fix

```
git add C/Slave/Inc/SCISlaveTransfer.h C/Slave/Src/SCISlaveTransfer.c C/Test/gtest/RoundTripSequenceTests.cpp
git commit -m "fix(slave): track owning command number for ongoing/upstream transfers

SCISlaveTransferProcessRequest()'s COMMAND (bNewCmd) and UPSTREAM number
checks compared against sResponseControl.sRsp.i16Num, which
SCISlaveTransferInitiateResponse() unconditionally overwrites from every
incoming request immediately before this function runs - both checks were
therefore always trivially true and could never observe a genuine number
change. Added tsRESPONSECONTROL.i16TransferCmdNum, set only when a request
is recognized as genuinely new, and switched both checks to use it.

Flips RoundTripSequenceTests.cpp's previously-pinned bug test
(NewCommandNumberIsNotRecognizedAsFreshMidTransfer) to assert the fixed
behavior, and adds SameCommandNumberContinuesOngoingTransfer +
UpstreamRequestWithMismatchedNumberIsRejected as regression coverage."
```

### Task 7 — Clean up now-stale comments in SlaveErrorTests.cpp

File: `C/Test/gtest/SlaveErrorTests.cpp` has two comment blocks that
describe the *old*, dead-code behavior as current fact. They need
correcting so they don't mislead future readers now that the check is live.

**7a.** In `UpstreamWithoutPriorCommandReturnsError`, find:
```c
    // Note on SCISlaveTransferProcessRequest()'s UPSTREAM case: it checks
    //   sResponseControl.sRsp.i16Num == sReq.i16Num && ui8ControlBits.upstream == true
    // but SCISlave.c always calls SCISlaveTransferInitiateResponse() with the
    // *current* request's number/type immediately before ProcessRequest() runs,
    // so sResponseControl.sRsp.i16Num is unconditionally overwritten to equal
    // sReq.i16Num for every request -- the number-equality half of that check
    // is therefore always trivially true and the "upstream" control bit is the
    // only condition that can actually fail. With no prior command, that bit
    // is false, so this trips eSCI_SLAVE_ERROR_UPSTREAM_NOT_INITIATED (enum
    // value 11 -> wire error number 0x100 + 11 = 0x10B).
```
Replace with:
```c
    // SCISlaveTransferProcessRequest()'s UPSTREAM case checks
    //   sResponseControl.i16TransferCmdNum == sReq.i16Num && ui8ControlBits.upstream == true
    // (see SCISlaveTransfer.c/.h - i16TransferCmdNum tracks which command
    // actually granted the transfer, fixed 2026-09-11; the number check used
    // to be dead code before that fix). With no prior command,
    // ui8ControlBits.upstream is false, so this trips
    // eSCI_SLAVE_ERROR_UPSTREAM_NOT_INITIATED (enum value 11 -> wire error
    // number 0x100 + 11 = 0x10B) regardless of the request number.
```

**7b.** In `UpstreamAfterNonUpstreamCommandReturnsError`, find:
```c
    // As explained in UpstreamWithoutPriorCommandReturnsError above, the
    // request-number check in the UPSTREAM branch is a no-op (it always
    // compares sReq.i16Num against itself), so it is *not* possible to
    // trigger eSCI_SLAVE_ERROR_UPSTREAM_NOT_INITIATED via a genuine
    // number mismatch against a prior upstream-granting command -- any
    // upstream number will be accepted as long as the upstream bit is
    // set. This test instead exercises the only condition that actually
    // gates the error: issuing an Upstream request after a command that
    // completed without granting an upstream transfer.
```
Replace with:
```c
    // This test exercises the "upstream bit never set" path: issuing an
    // Upstream request after a command that completed without granting an
    // upstream transfer. See
    // RoundTripSequenceTests.cpp's UpstreamRequestWithMismatchedNumberIsRejected
    // for the sibling case (a genuine number mismatch against a command
    // that DID grant an upstream transfer) - that case used to be
    // impossible to construct here because the number check was dead code;
    // it's now fixed and covered there via direct
    // SCISlaveTransferProcessRequest() calls instead of raw byte injection.
```

Build/run to confirm nothing regressed (comment-only change, but rebuild is
cheap and confirms no stray edit broke compilation):
```
cd C && cmake --build build 2>&1 | tail -n 10
cd build && ctest --output-on-failure
```
Expected: still 100% passing, same 28 tests.

Commit:
```
git add C/Test/gtest/SlaveErrorTests.cpp
git commit -m "docs(test): correct now-stale UPSTREAM dead-code comments in SlaveErrorTests.cpp"
```

### Task 8 — Update `.hermes.md`

The "Known bug, deliberately pinned by a test rather than fixed" paragraph
under `C/Test/` in `.hermes.md` describes exactly this bug and says "Not
fixed as part of the test expansion — flagged for a future decision". Since
this plan fixes it, remove or rewrite that paragraph (e.g. replace with a
one-line pointer: "Fixed 2026-09-11: see SCISlaveTransfer.c/.h history and
RoundTripSequenceTests.cpp's SlaveTransferProcessRequestTest cases.") and
update the "Recent commits" mention isn't needed (that section is a
snapshot, not maintained per-commit) — just fix the bug-description
paragraph itself and the `RoundTripSequenceTests.cpp` file bullet under the
directory map (it currently promises tests that "flip green" for a future
fix — reword since that future is now).

Commit:
```
git add .hermes.md
git commit -m "docs: update .hermes.md - slave transfer number-comparison bug is fixed"
```

### Task 9 — Final full-suite verification

```
cd C/build && ctest --output-on-failure
```
Expected: `100% tests passed, 0 tests failed out of 28`.

```
git log --oneline -5
git status --short
```
Expected: 3 commits on `fix/slave-transfer-new-command-detection` on top of
`master`'s current tip, clean working tree.

## Tests / validation summary

| Task | Test(s) | RED command | GREEN command |
|---|---|---|---|
| 2 | `SlaveTransferProcessRequestTest.NewCommandNumberIsRecognizedAsFreshMidTransfer`, `.UpstreamRequestWithMismatchedNumberIsRejected` | `ctest -R SlaveTransferProcessRequestTest --output-on-failure` (fails, 2 of 3) | same command after Task 4 (all 3 pass) |
| — | `SlaveTransferProcessRequestTest.SameCommandNumberContinuesOngoingTransfer` | passes even before the fix (continuation path unaffected) | still passes after — pins that the fix didn't break continuation |
| 5 | full suite | — | `ctest --output-on-failure` → 100%, 28 tests |

## Risks, tradeoffs, and open questions

- **Scope decision already made in this plan**: the UPSTREAM branch's
  identical dead-code pattern is fixed alongside COMMAND's, since it's
  literally the same root cause and is already cross-referenced from
  existing test comments. If you'd rather ship only the COMMAND-side fix
  (narrower diff, matches the user's literal wording most closely), skip
  the UPSTREAM half of Task 4's replacement (leave that `case` block
  unchanged) and drop `UpstreamRequestWithMismatchedNumberIsRejected` from
  Task 2 and the Task 7 rewrite — flag this explicitly if choosing that
  path, since it leaves a known-identical bug pattern unfixed on purpose.
- **`i16TransferCmdNum` naming**: chosen to avoid confusion with
  `sRsp.i16Num` (which still serves its original purpose — stamping the
  response frame's echoed number — and must NOT be repurposed, since
  `SCISlaveDataframe.c` and other read sites depend on it reflecting the
  current request). Don't collapse the two fields.
- **No change to `SCISlave.c` or `SCISlaveTransferInitiateResponse()`**:
  the call order (`InitiateResponse()` then `ProcessRequest()`) is
  unchanged; the fix is entirely inside `SCISlaveTransferProcessRequest()`
  and the struct it operates on. This keeps the diff minimal and avoids
  touching the datalink/dataframe layers.
- **Behavioral change for real hardware**: a genuinely new command number
  arriving mid-transfer will now actually interrupt the old transfer's
  ongoing/upstream state (old callback's data is simply discarded, new
  command runs). This matches the plan's fix intent and the test names,
  but confirm this is the desired protocol semantics before merging — the
  alternative (reject the new request until the old transfer drains) was
  not implemented here since nothing in the existing code/docs suggested
  that alternative was intended.
- Test count in Task 5/9 (28) assumes no other test file changes on this
  branch; recount if you've made unrelated edits.
