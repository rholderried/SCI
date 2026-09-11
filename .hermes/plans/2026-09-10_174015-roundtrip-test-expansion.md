# SCI Round-Trip Test Expansion — Implementation Plan

> **For Hermes:** Use subagent-driven-development skill to implement this plan
> task-by-task, OR implement sequentially in this session — either way, run
> `ctest --output-on-failure` after every phase before moving on.

**Goal:** Extend `C/Test/gtest/` with SetVar, Command (all three response
shapes), Upstream, error-path, and sequencing round-trip tests, plus the
slave-side error/malformed-message coverage that can't be reached through the
Master's public API.

**Architecture:** Reuse the existing `RoundTripTest` fixture pattern from
`C/Test/gtest/RoundTripTests.cpp` (real `SCIMasterSM`/`SCISlaveStatemachine`
pumped in lockstep via the in-process blocking TX callback in
`gtest/MasterTestCallbacks.c`). New round-trip test files are added per
request-type category to keep files small; a couple of tests that must
inject raw bytes directly into the Slave (malformed frames, out-of-sequence
upstream) go in `SlaveTests.cpp`-style raw-byte tests instead, since the
Master's request builder can't produce invalid frames on purpose. Fixture
data (`VariablesAndCommands.c`, `TestCallbacks.c`, `C/Test/config/SCIconfig.h`)
gets extended with new vars/commands, appended after the existing ones so
current hardcoded addresses (`SlaveTests.cpp` uses var numbers 1,3,4,5) don't
shift.

**Tech Stack:** C (C11), CMake + GoogleTest (`FetchContent`, pinned
`v1.15.2`), MinGW-W64 gcc/g++ build already configured at `C/build`.

---

## Current state (verified by reading the code)

- `C/Test/gtest/RoundTripTests.cpp` has exactly one test:
  `GetVarUI8RoundTrip`. `PumpUntilIdle(maxIterations=500)` pumps both state
  machines until `SCIGetProtocolState() == ePROTOCOL_IDLE`.
- `C/Test/VariablesAndCommands.c` defines `varStruct[5]` (all `eVARTYPE_RAM`,
  no `ap` action-procedure callback set on any of them) and
  `COMMAND_CB cmdStruct = {testCmd};` — **this is a latent bug**: `SCIconfig.h`
  sets `SIZE_OF_CMD_STRUCT 2`, but `cmdStruct` is declared as a bare struct
  initializer, not a 2-element array. `pCmdCBStruct[sReq.i16Num - 1]` in
  `SCISlaveTransfer.c:134` will read out of bounds if command number 2 is
  ever requested against current fixture. Must fix as part of Phase 0.
- `testCmd` (the only command) always returns `eCOMMAND_STATUS_SUCCESS_UPSTREAM`
  with a 20-byte buffer (`ui8_testBuffer`) — no fixture exercises
  `SUCCESS_DATA` or plain `SUCCESS` today.
- No EEPROM-typed var, no var with `ap` set — `InitVarstruct`'s EEPROM
  partition-table logic and the SetVar EEPROM-write-rollback branch
  (`SCISlaveTransfer.c:96-106`) are currently untested at any level.
- `TX_PACKET_LENGTH`/`RX_PACKET_LENGTH` are 128 in `C/Test/config/SCIconfig.h`.
  The Upstream continuation path (`_SCIFillBufferWithValues`,
  `SCISlaveDataframe.c:283-311`) only gets exercised across multiple
  messages if the upstream payload exceeds one packet's worth of raw bytes —
  a fixture command must return a buffer clearly larger than 128 bytes to
  force chunking.
- `SCIRequestGetVar`/`SCIRequestSetVar`/`SCIRequestCommand` are the only
  public Master request entry points (`SCIMaster.h:161-176`). There is no
  public API to send a bare, out-of-sequence Upstream request or a malformed
  frame — those cases must be tested by injecting raw bytes into the Slave
  directly (`SCISlaveReceiveData`), the same way `SlaveTests.cpp` does.

---

## Phase 0 — Fixture fixes and additions (prerequisite for everything else)

### Task 0.1: Fix `cmdStruct` to be a real 2+-element array and add response-shape variety

**Files:**
- Modify: `C/Test/VariablesAndCommands.c`
- Modify: `C/Test/config/SCIconfig.h` (`SIZE_OF_CMD_STRUCT`)

**Details:**
Rename current `testCmd` → `testCmdUpstream` (keeps existing upstream
behavior, still returns the 20-byte `ui8_testBuffer`, command number 1).
Add:
- `testCmdData` (command number 2): returns `eREQUEST_ACK_STATUS_SUCCESS_DATA`
  (`VALUE_MODE_HEX` build) with a small, fixed, known numeric array (e.g. 3
  `uint32_t` values) written into `psData->puRespVals`, `ui32DatLen = 3`, so
  the round-trip test can assert exact returned values.
- `testCmdPlain` (command number 3): returns `eREQUEST_ACK_STATUS_SUCCESS`
  with `ui32DatLen = 0` (no data) — exercises the `default:` branch in
  `SCIMasterTransfer.c:179-186`.
- `testCmdUpstreamLarge` (command number 4): returns
  `eREQUEST_ACK_STATUS_SUCCESS_UPSTREAM` with a buffer > `TX_PACKET_LENGTH`
  (e.g. 300 bytes, deterministic content such as `i % 256`) to force
  multi-packet upstream chunking. Needs its own backing byte array (don't
  reuse `ui8_testBuffer`, which is only 20 bytes and shared with the args
  test below).
- `testCmdWithArgs` (command number 5): accepts and validates the incoming
  `pui32_valArray`/`ui8_valArrayLen` (record what it received into file-scope
  statics the test can inspect, e.g. `g_lastCmdArgs[]`, `g_lastCmdArgCount`),
  then returns plain `SUCCESS`.

Set `SIZE_OF_CMD_STRUCT` to `5` in `C/Test/config/SCIconfig.h`.
`cmdStruct` becomes:
```c
COMMAND_CB cmdStruct[5] = { testCmdUpstream, testCmdData, testCmdPlain,
                             testCmdUpstreamLarge, testCmdWithArgs };
```
Update the `extern COMMAND_CB cmdStruct;` declarations in
`RoundTripTests.cpp` and `SlaveTests.cpp` to `extern COMMAND_CB cmdStruct[];`.

**Validation:** `cmake --build build` succeeds; existing `SlaveTests.cpp` and
`RoundTripTests.cpp` still compile (they don't invoke commands, only GetVar,
so behavior is unaffected).

### Task 0.2: Add an EEPROM-backed var and an action-procedure var

**Files:**
- Modify: `C/Test/VariablesAndCommands.c`
- Modify: `C/Test/config/SCIconfig.h` (`SIZE_OF_VAR_STRUCT`)

**Details:**
Append (don't insert — preserve existing addresses 1-5) two entries to
`varStruct`:
- Address 6: `uint16_t ui16_eeprom_test = 1000;`, `eVARTYPE_EEPROM`,
  `eDTYPE_UINT16`, no `ap`. Relies on the existing `SlaveReadEEROM`/
  `SlaveWriteEEROM` fixture callbacks in `TestCallbacks.c`
  (`EEPROM_ADDRESSTYPE == EEPROM_WORD_ADDRESSABLE` per current config).
- Address 7: `uint8_t ui8_ap_test = 0;`, `eVARTYPE_RAM`, `eDTYPE_UINT8`, with
  `ap` set to a new function `IncrementApCounter(void)` that increments a
  file-scope `uint32_t g_apCallCount` (expose via a getter or extern global
  so tests can assert call count and reset it in `SetUp()`).

Set `SIZE_OF_VAR_STRUCT` to `7` in `C/Test/config/SCIconfig.h`.

**Details for EEPROM write-failure rollback test (Task 3.4 below):** add a
controllable failure switch to `TestCallbacks.c`'s `SlaveWriteEEROM`, e.g.
```c
bool g_forceEEPROMWriteFailure = false;
bool SlaveWriteEEROM(uint32_t ui32Val, uint16_t ui16Address)
{
    if (g_forceEEPROMWriteFailure) return false;
    ... existing body ...
}
```
Declare `extern bool g_forceEEPROMWriteFailure;` in `TestCallbacks.h` so
tests can flip it. Reset to `false` in every fixture's `SetUp()`/`TearDown()`
to avoid cross-test leakage.

**Validation:** `cmake --build build`; run existing `SlaveTests.cpp` — none of
its hardcoded var numbers (1,3,4,5) or expected byte sequences change since
new vars are appended at 6/7.

### Task 0.3: Wire new fixture sources into CMake, rebuild baseline

**Files:**
- Modify: `C/Test/CMakeLists.txt` only if new fixture `.c`/`.h` files are
  added (e.g. if `TestCallbacks.h` additions require no new file, skip).

**Validation:**
```
cd C/build
cmake --build .
ctest --output-on-failure
```
Expected: all existing tests (`SlaveTest.*`, `RoundTripTest.GetVarUI8RoundTrip`)
still pass, 0 regressions. Commit this phase alone before moving on —
fixture correctness is load-bearing for every later phase.

---

## Phase 1 — SetVar round trips

**File (new):** `C/Test/gtest/RoundTripSetVarTests.cpp`
**Fixture:** same `RoundTripTest`-style class as `RoundTripTests.cpp` (copy
the `SetUp`, `PumpUntilIdle` helpers, or factor them into a shared header —
see Task 1.0).

### Task 1.0: Factor shared round-trip test scaffolding into a header

**Files:**
- Create: `C/Test/gtest/RoundTripFixture.h`
- Modify: `C/Test/gtest/RoundTripTests.cpp` to use it

**Details:** Move `PumpUntilIdle` and the `RoundTripTest` fixture class
(SetUp: `SCISlaveInit` + `SCIMasterInit`) into `RoundTripFixture.h` so every
new round-trip test file in this plan includes one header instead of
duplicating ~20 lines. Keep callback-capture statics (`g_callbackFired` etc.)
per-file/per-test-suite since different request types need different
captured payloads — don't over-generalize those into the shared header.

**Validation:** `cmake --build build && ctest` — `RoundTripTest.GetVarUI8RoundTrip`
still passes unchanged.

### Task 1.1: SetVar round trip per datatype, asserting the Slave-side variable actually changed

**Test cases** (one `TEST_F` each, in `RoundTripSetVarTests.cpp`):
- `SetVarUI8RoundTrip` — var 3 (`ui8_test`), set to e.g. `0x11`, assert
  Master's `SetVarExternalCB` receives `eREQUEST_ACK_STATUS_SUCCESS`, and
  read back via `extern uint8_t ui8_test;` (declared in
  `VariablesAndCommands.c`) that the Slave's backing variable equals `0x11`.
- `SetVarUI16RoundTrip` — var 4 (`ui16_test`).
- `SetVarI32RoundTrip` — var 5 (`i32_test`).
- `SetVarF32RoundTrip` — var 1 or 2 (`testVar`/`f_test`).

Use `SCIRequestSetVar(i16VarNum, uVal)` where `uVal.ui32_hex` holds the raw
hex-mode bit pattern (mirrors how `SlaveTests.cpp` encodes expected ASCII hex
today — reuse the same hex constants where the existing `PollVar*` tests
already prove the wire encoding, e.g. `0xF5` for UI8).

**Validation:** `ctest --output-on-failure -R RoundTripSetVar`

### Task 1.2: SetVar with the action-procedure var — assert `ap` fires exactly once

**Test:** `SetVarTriggersActionProcedureOnce` — SetVar on address 7
(`ui8_ap_test`), assert `g_apCallCount == 1` after one round trip, and
`== 2` after a second round trip (proves it's not accidentally firing twice
per call or not firing at all).

### Task 1.3: SetVar on an EEPROM-backed var — round trip through the EEPROM callbacks

**Test:** `SetVarEepromRoundTrip` — SetVar on address 6
(`ui16_eeprom_test`), assert the ACK, assert the RAM shadow value changed,
and assert (via a small EEPROM-read helper or by reading the
`ui16EEPROMWordAddressable` array declared in `TestCallbacks.c`) that the
EEPROM-backed storage was actually written, not just the RAM copy.

### Task 1.4: SetVar with a failing EEPROM write — rollback path

**Test:** `SetVarEepromWriteFailureRollsBackValue` — set
`g_forceEEPROMWriteFailure = true`, SetVar on address 6 with a new value,
assert the Master callback receives `eREQUEST_ACK_STATUS_ERROR` with
`ui16ErrNum` corresponding to `GET_SCI_ERROR_NUMBER(eSCI_SLAVE_ERROR_EEPROM_WRITE_FAILED)`
(offset defined by `SCI_ERROR_OFFSET` in `SCIconfig.h`, error enum in
`SCICommon.h`), and — this is the actual regression-worthy assertion — that
`ui16_eeprom_test` on the Slave side still holds its **original** value (the
rollback in `SCISlaveTransfer.c:103`), not the rejected new one. Reset the
failure flag in `TearDown()`.

**Validation:** `ctest --output-on-failure -R RoundTripSetVar`

---

## Phase 2 — Command round trips (all three response shapes + args)

**File (new):** `C/Test/gtest/RoundTripCommandTests.cpp`

### Task 2.1: Extend Master callback capture for Command responses

Add a `MASTER_COMMAND_CB` capture function (mirrors `OnGetVarResponse` in
`RoundTripTests.cpp`) recording `eAck`, `i16Num`, a copy of the
`uint32_t *pui32Data` array (bounded by `ui8DataCnt`), and `ui16ErrNum`.
Wire it into `sMasterCbs.CommandExternalCB` in each test's `SetUp`.

### Task 2.2: Plain-success command round trip

**Test:** `CommandPlainSuccessRoundTrip` — `SCIRequestCommand(3, NULL, 0)`
(command 3 = `testCmdPlain`), assert callback fires with
`eREQUEST_ACK_STATUS_SUCCESS`, `ui8DataCnt == 0`, `pui32Data == NULL` (or
unused — check what `SCIMasterTransfer.c:179-186`'s default branch actually
passes).

### Task 2.3: Data-response command round trip

**Test:** `CommandDataRoundTrip` — `SCIRequestCommand(2, NULL, 0)` (command 2
= `testCmdData`), assert the 3 known values come back intact and in order.
This is the first test to exercise
`eREQUEST_ACK_STATUS_SUCCESS_DATA` end-to-end through
`SCIMasterTransfer.c:87-146` (memory allocation, `ui32ReceivedDataCnt`
bookkeeping, single-packet case since only 3 values).

### Task 2.4: Command-with-arguments round trip

**Test:** `CommandWithArgsRoundTrip` — build a `tuREQUESTVALUE` array (e.g. 2
values), call `SCIRequestCommand(5, arr, 2)` (command 5 = `testCmdWithArgs`),
assert on the Slave side (`g_lastCmdArgs`/`g_lastCmdArgCount` fixture
globals from Task 0.1) that the exact values arrived, and that the round
trip completes with `SUCCESS`.

### Task 2.5: Upstream-triggering command round trip (small payload, single packet)

**Test:** `CommandUpstreamRoundTrip` — `SCIRequestCommand(1, NULL, 0)`
(command 1 = `testCmdUpstream`, existing 20-byte buffer). Add a
`MASTER_UPSTREAM_CB` capture (`UpstreamExternalCB`) recording
`pui8Data`/`ui32ByteCnt`. Assert `PumpUntilIdle` completes (this
automatically drives Command → Upstream sequencing per
`SCIMasterTransfer.c:148-176`) and the 20 received bytes match
`ui8_testBuffer`'s known content exactly, in order.

### Task 2.6: Large-payload upstream round trip — forces multi-packet chunking

**Test:** `CommandUpstreamLargeRoundTripChunked` — `SCIRequestCommand(4, ...)`
(command 4 = `testCmdUpstreamLarge`, >128-byte buffer from Task 0.1). Bump
`PumpUntilIdle`'s iteration budget (the default 500 may not be enough for a
multi-message transfer — compute or generously overestimate: roughly
`(payloadSize / TX_PACKET_LENGTH + 2) * 4` state-machine ticks, verify
empirically). Assert:
- The transfer completes (`PumpUntilIdle` returns true).
- The reassembled buffer length equals the fixture's payload size exactly.
- Byte content matches exactly, in order (catches off-by-one chunk-boundary
  bugs in `_SCIFillBufferWithValues`/the Master's
  `ui32ReceivedDataCnt`/`ui32ExpectedDataCnt` loop).
- (Optional but valuable) Add a counter in the test harness — not
  production code — that counts how many times `SCIMasterSM` transitions
  through `ePROTOCOL_EVALUATING` for this transaction, and assert it's `> 1`,
  proving the test actually exercised chunking rather than accidentally
  fitting in one packet.

**Validation:** `ctest --output-on-failure -R RoundTripCommand`

---

## Phase 3 — Upstream and error-path round trips reachable via the Master API

**File (new):** `C/Test/gtest/RoundTripErrorTests.cpp`

### Task 3.1: GetVar with an invalid variable number

**Test:** `GetVarInvalidNumberReturnsError` — `SCIRequestGetVar(999)` (or
`0`), assert Master's `GetVarExternalCB` receives
`eREQUEST_ACK_STATUS_ERROR` with `ui16ErrNum ==
GET_SCI_ERROR_NUMBER(eSCI_SLAVE_ERROR_VAR_NUMBER_INVALID)` (the offset macro
is private to `SCISlave.c` — either expose it for tests or duplicate the
`SCI_ERROR_OFFSET + eSCI_SLAVE_ERROR_VAR_NUMBER_INVALID` computation using
the public `SCI_ERROR_OFFSET` define from `SCIconfig.h` and the enum from
`SCICommon.h`).

### Task 3.2: SetVar with an invalid variable number

**Test:** `SetVarInvalidNumberReturnsError` — same shape as 3.1 via
`SCIRequestSetVar`.

### Task 3.3: Command with an invalid command number

**Test:** `CommandInvalidNumberReturnsError` — `SCIRequestCommand(99, NULL, 0)`,
assert `eREQUEST_ACK_STATUS_ERROR` with
`eSCI_SLAVE_ERROR_REQUEST_UNKNOWN` (from `SCISlaveTransfer.c:141`).

**Validation:** `ctest --output-on-failure -R RoundTripError`

---

## Phase 4 — Slave-only raw-byte tests for cases the Master API can't reach

**File (new):** `C/Test/gtest/SlaveErrorTests.cpp` — reuse the `PumpSlave`
helper pattern from `C/Test/gtest/SlaveTests.cpp` (raw byte injection via
`SCISlaveReceiveData`, then `SCISlaveStatemachine`), not the Master.

### Task 4.1: Upstream request without a preceding command

**Test:** `UpstreamWithoutPriorCommandReturnsError` — send a raw
`{STX, '1', '>', ETX}` message (upstream request for var/cmd number 1)
without ever sending a Command first. Assert the response frame's error code
corresponds to `eSCI_SLAVE_ERROR_UPSTREAM_NOT_INITIATED`
(`SCISlaveTransfer.c:186-189`), matching the exact-byte-comparison style
already used in `SlaveTests.cpp` (`expect[]` array + `memcmp`).

### Task 4.2: Upstream request with a mismatched number after a real command

**Test:** `UpstreamWithMismatchedNumberReturnsError` — issue a real Command
(number 1) via raw bytes, then send an Upstream request for a *different*
number (e.g. 2). Assert the same `eSCI_SLAVE_ERROR_UPSTREAM_NOT_INITIATED`
path triggers (`SCISlaveTransfer.c:176` condition fails on the number
mismatch, not just the `ongoing`/`upstream` bits).

### Task 4.3: Malformed message — unknown identifier

**Test:** `UnknownIdentifierReturnsNak` — send `{STX, '1', '@', ETX}` (`@` is
not a valid identifier). Assert the response matches the `UNKNOWN`
acknowledge path (`SCISlaveDataframe.c:253-260`), i.e. exact bytes
`{..., 'U','N','K', ETX}`-shaped per the existing encoding, and — important —
that the Slave's `e_state` returns to `ePROTOCOL_IDLE` on the next
`SCISlaveStatemachine()` tick rather than getting stuck (protects against a
state-machine deadlock regression).

**Validation:** `ctest --output-on-failure -R SlaveError`

---

## Phase 5 — Sequencing / statefulness round trips

**File (new):** `C/Test/gtest/RoundTripSequenceTests.cpp`

### Task 5.1: Back-to-back heterogeneous transactions don't leak state

**Test:** `GetVarThenSetVarThenCommandSequenceRoundTrip` — in one test,
`PumpUntilIdle` after each of: GetVar(var 3), SetVar(var 4, new value),
Command(3, plain success). Assert each callback fires with the correct data
for *its own* request (catches `sResponseControl`/`sTransferInfo` state
bleeding between transactions since `SCISlaveTransferClearResponseControl`
must actually reset everything used by the next transaction).

### Task 5.2: New command number arrives while a multi-packet command is still ongoing

**Test:** `NewCommandNumberDuringOngoingTransferIsTreatedAsNew` — this
exercises `bNewCmd` in `SCISlaveTransfer.c:125`
(`ongoing == false || i16Num != sReq.i16Num`). Likely needs raw-byte Slave
injection (Phase-4 style) rather than the Master API, since the Master's
`SCITransferControl` always finishes draining one command's data before
issuing another request — to get a genuinely interleaved raw request you
must bypass the Master and hand-craft the byte sequence: start Command 4
(`testCmdUpstreamLarge`, multi-packet), then before requesting the second
chunk, send a fresh Command 2 request (`testCmdData`) instead. Assert the
Slave treats it as a brand-new command (fresh response, not corrupted
leftover state from command 4), matching `bNewCmd == true` behavior.
**Flag this test as exploratory** — if constructing the raw byte sequence
proves too fragile/timing-dependent, it's acceptable to descope to a
smaller unit-level test calling `SCISlaveTransferProcessRequest` directly
twice with different `sReq.i16Num` values and asserting
`psTransfer->sResponseControl.ui8ControlBits.firstPacketNotSent` behaves as
a "new command" would, without going through the full byte-level protocol.

**Validation:** `ctest --output-on-failure -R RoundTripSequence`

---

## Phase 6 — Wire everything into CMake and final full-suite run

### Task 6.1: Add all new test sources to `C/Test/CMakeLists.txt`

```cmake
add_executable(sci_tests
    TestCallbacks.c
    VariablesAndCommands.c
    gtest/SlaveTests.cpp
    gtest/SlaveErrorTests.cpp
    gtest/MasterTestCallbacks.c
    gtest/RoundTripTests.cpp
    gtest/RoundTripSetVarTests.cpp
    gtest/RoundTripCommandTests.cpp
    gtest/RoundTripErrorTests.cpp
    gtest/RoundTripSequenceTests.cpp
)
```
(`RoundTripFixture.h` is header-only, no source entry needed.)

### Task 6.2: Full suite run and regression check

```
cd C/build
cmake --build .
ctest --output-on-failure
```
Expected: every test from Phases 0-5 plus the pre-existing `SlaveTest.*` and
`RoundTripTest.GetVarUI8RoundTrip` pass, 0 regressions.

### Task 6.3: Update `.hermes.md`

Add a line under "Directory map" → `C/Test/` describing the new
`gtest/RoundTrip*Tests.cpp` / `gtest/SlaveErrorTests.cpp` files and the
`cmdStruct` array fix, since `.hermes.md` currently only mentions
`SlaveTests.cpp`/`RoundTripTests.cpp`.

---

## Files touched (summary)

- Modify: `C/Test/config/SCIconfig.h` (`SIZE_OF_VAR_STRUCT`, `SIZE_OF_CMD_STRUCT`)
- Modify: `C/Test/VariablesAndCommands.c` (new vars/commands, fix `cmdStruct` array)
- Modify: `C/Test/TestCallbacks.c` / `TestCallbacks.h` (EEPROM-write failure switch)
- Modify: `C/Test/gtest/RoundTripTests.cpp` (use shared fixture header)
- Modify: `C/Test/gtest/SlaveTests.cpp` (`extern COMMAND_CB cmdStruct[];`)
- Create: `C/Test/gtest/RoundTripFixture.h`
- Create: `C/Test/gtest/RoundTripSetVarTests.cpp`
- Create: `C/Test/gtest/RoundTripCommandTests.cpp`
- Create: `C/Test/gtest/RoundTripErrorTests.cpp`
- Create: `C/Test/gtest/RoundTripSequenceTests.cpp`
- Create: `C/Test/gtest/SlaveErrorTests.cpp`
- Modify: `C/Test/CMakeLists.txt`
- Modify: `.hermes.md`

## Risks / open questions

- **`cmdStruct` array fix (Task 0.1) changes an existing symbol's type.**
  Any other file referencing `cmdStruct` as a scalar would break — confirmed
  only `RoundTripTests.cpp` and `SlaveTests.cpp` reference it (both via
  `extern COMMAND_CB cmdStruct;`, neither dereferences it directly), so this
  is safe, but double-check with a repo-wide grep before landing.
- **Iteration budget for the large-upstream chunking test (Task 2.6)** is an
  estimate — the plan calls for empirically verifying it rather than trusting
  the formula; if `PumpUntilIdle` returns `false`, raise the cap rather than
  changing the payload size (keep it deliberately >1 packet).
- **Task 5.2 (interleaved command numbers)** is explicitly flagged as
  possibly needing to be descoped to a narrower unit-style test — raw
  byte-level interleaving with a real multi-packet transfer might be too
  fiddly to construct reliably; the plan gives a fallback.
- **EEPROM error-number assertions (Task 1.4, 3.x)** depend on
  `GET_SCI_ERROR_NUMBER`, which is a private macro in `SCISlave.c`
  (`#define GET_SCI_ERROR_NUMBER(e) (e + SCI_ERROR_OFFSET)`). Tests must
  either replicate this computation using the public `SCI_ERROR_OFFSET`
  define and public error enums, or a small public accessor could be added —
  decide during implementation; replication is simpler and doesn't touch
  production code.
- This plan does not cover `SEND_MODE_BYTE_BY_BYTE` vs. other send-mode
  configs, or `VALUE_MODE_HEX` vs. float ASCII mode — both are compile-time
  switches in `SCIconfig.h` fixed for the whole test binary; testing the
  alternate modes would need a second test binary/config, out of scope here.
