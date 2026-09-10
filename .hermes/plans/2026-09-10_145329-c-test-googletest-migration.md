# C Test Environment: Migrate to GoogleTest + Verify Master/Slave Round Trip

> **For Hermes:** Use subagent-driven-development to implement this plan
> task-by-task. Work happens on a dedicated branch — never commit directly
> to `master` (see `.hermes.md` workflow rules).

**Goal:** Replace the unused Unity test scaffold with a CMake + GoogleTest
environment, port the existing Slave-only tests as a safety net, and add the
first real Master↔Slave round-trip test to actually verify (not assume) that
current Master/Slave interop works.

**Architecture:** A new `C/CMakeLists.txt` builds the SCI sources (Common +
Master + Slave) as a static library and, when `BUILD_TESTS` is on, pulls in
GoogleTest via `FetchContent` (no git submodule) and builds a `sci_tests`
executable discovered by CTest. Existing C sources need no production-code
changes for this migration except one confirmed bug fix (see Current
Context). Tests are C++ (`.cpp`) linking against the C library via the
already-present `extern "C"` guards in the public headers.

---

## Current context / assumptions (verified before writing this plan)

- **Repo root:** `D:\Git\SCI`. C library code lives under `C/Common`,
  `C/Master`, `C/Slave`, config in `C/config/SCIconfig.h`, tests in
  `C/Test/`.
- **Unity is not usable right now.** `C/Test/Unity` is declared as a git
  submodule (`.gitmodules`) but is **not checked out** (`git submodule
  status` shows a `-` prefix — empty local directory). This plan removes
  the Unity submodule and its config file instead of initializing it.
- **The existing 4 tests only exercise the Slave.** `C/Test/UnitTests.c`
  calls `SCISlaveInit()` in `setUp()` and only drives
  `SCISlaveReceiveData()` / `SCISlaveStatemachine()`. `SCIMasterInit()` is
  **never called anywhere in `C/Test/`** (verified by repo-wide search) — so
  the Master side of the protocol currently has zero test coverage.
- **`NativeTestControl.c` is dead scaffold code.** It defines a
  `TriggerMaster()` function and an `sTestControl` struct with
  `sTriggerSlave`/`sTriggerMaster` sub-structs suggesting an abandoned
  attempt at full-duplex Master/Slave wiring, but `TriggerMaster` is never
  called from anywhere and the struct fields are never used. This is the
  concrete evidence behind "not sure this version of the code works" — the
  round trip has never actually been exercised.
- **The code compiles**, but with warnings, using:
  `C:\MinGW64\bin\gcc.exe` (MinGW-W64 7.3.0, confirmed present) against all
  `C/Common/Src/*.c`, `C/Master/Src/*.c`, `C/Slave/Src/*.c` with the four
  `Inc` dirs + `C/config` on the include path.
- **One real bug found and confirmed by compiler warning:**
  `C/Master/Src/SCIMasterTransfer.c:35-48`, function `SCITransferStart`, is
  declared to return `bool` but has no `return` statement on the
  non-early-return path (`control reaches end of non-void function
  [-Wreturn-type]`). This plan fixes it as part of round-trip hardening
  (Task 12).
- **Toolchain available on this machine:** `gcc`/`g++` 7.3.0 (MinGW-W64),
  `cmake` 3.23.2, `mingw32-make` 4.2.1 (GNU Make for MinGW). No `ninja`.
  Internet access confirmed reachable (`github.com` returns HTTP 200), so
  CMake `FetchContent` can pull GoogleTest from GitHub.
- **Message/variable numbering used by existing tests** (for round-trip
  test parity): `C/Test/VariablesAndCommands.c` defines `varStruct[]` where
  index 2 (`ui8_test`, value `245` = `0xF5`) is addressed as protocol
  number `'3'` (1-based) — this is exactly what
  `test_SCISlavePollVarUI8` in the old suite exercises. Task 11 reuses the
  same variable/value for its round-trip assertion.
- **Config is fixed, not something this plan changes:**
  `C/config/SCIconfig.h` defines `VALUE_MODE_HEX` and
  `SEND_MODE_BYTE_BY_BYTE` — i.e. all values are hex-encoded ASCII, and
  transmission happens one byte per state-machine tick via the *blocking*
  TX callback path. All new test code must use the blocking-callback shape
  (see `SlaveTxCbBlocking` in `C/Test/TestCallbacks.c` as the existing
  reference pattern for the Slave side).

## Files likely to change

- New: `C/CMakeLists.txt`
- New: `C/Test/gtest/SlaveTests.cpp` (ported legacy tests)
- New: `C/Test/gtest/RoundTripTests.cpp` (new Master↔Slave test)
- New: `C/Test/gtest/MasterTestCallbacks.c` + `.h` (Master-side test
  callbacks, mirroring the existing Slave-side ones)
- Modify: `C/Master/Src/SCIMasterTransfer.c` (missing `return true;`)
- Modify: `.hermes.md` (Build/test section — describe CMake+GTest workflow)
- Modify: `.vscode/tasks.json` (point the default build task at CMake)
- Delete: `C/Test/UnitTests.c`, `C/Test/unity_config.h`, `C/Test/Unity`
  (submodule reference), `C/Test/UnitTest` (stale checked-in binary),
  `.gitmodules`
- Unchanged, reused as-is: `C/Test/TestCallbacks.c/.h`,
  `C/Test/VariablesAndCommands.c`

---

## Step-by-step tasks

### Task 1: Create the working branch

Per the repo's workflow rule (see `.hermes.md`), never commit to `master`
directly.

```bash
cd D:/Git/SCI
git checkout master
git pull
git checkout -b test/googletest-migration
```

Verify: `git branch --show-current` → `test/googletest-migration`.

No commit yet — nothing changed.

---

### Task 2: Remove the unused Unity submodule and its config

**Files:**
- Delete: `C/Test/Unity` (submodule working tree, currently empty anyway)
- Delete: `C/Test/unity_config.h`
- Delete: `.gitmodules`

```bash
git submodule deinit -f C/Test/Unity
git rm -f C/Test/Unity
git rm -f C/Test/unity_config.h
git rm -f .gitmodules
```

Verify:
```bash
git status
```
Expected: `.gitmodules`, `C/Test/Unity`, `C/Test/unity_config.h` staged as
deleted. `ls C/Test` no longer lists `Unity` or `unity_config.h`.

Commit:
```bash
git add -A
git commit -m "test: remove unused Unity submodule and config"
```

---

### Task 3: Add the CMake library target

**File:** Create `C/CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.14)
project(sci C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(sci STATIC
    Common/Src/Buffer.c
    Common/Src/Helpers.c
    Common/Src/SCIDatalink.c
    Master/Src/SCIMaster.c
    Master/Src/SCIMasterDataframe.c
    Master/Src/SCIMasterTransfer.c
    Slave/Src/SCISlave.c
    Slave/Src/SCISlaveDataframe.c
    Slave/Src/SCISlaveTransfer.c
    Slave/Src/VarAccess.c
)

target_include_directories(sci PUBLIC
    Common/Inc
    Master/Inc
    Slave/Inc
    config
)

option(BUILD_TESTS "Build the sci_tests GoogleTest suite" ON)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(Test)
endif()
```

Verify (configure only — `Test/CMakeLists.txt` doesn't exist yet, so
temporarily set `BUILD_TESTS=OFF` for this one check):
```bash
cd D:/Git/SCI/C
cmake -S . -B build -G "MinGW Makefiles" -DBUILD_TESTS=OFF
```
Expected: `-- Build files have been written to: .../C/build`, no errors.

```bash
cmake --build build
```
Expected: `build/libsci.a` produced, no compile errors (warnings from the
existing code are OK and expected at this stage — same ones seen in Current
Context).

Commit:
```bash
git add C/CMakeLists.txt
git commit -m "build: add CMake library target for the SCI C sources"
```

---

### Task 4: Add the CMake test target with GoogleTest via FetchContent

**File:** Create `C/Test/CMakeLists.txt`

```cmake
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
)
# Keep gtest's runtime library selection consistent with this project on MSVC;
# harmless on MinGW/GCC.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(sci_tests
    TestCallbacks.c
    VariablesAndCommands.c
)

target_include_directories(sci_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(sci_tests PRIVATE sci gtest_main)

include(GoogleTest)
gtest_discover_tests(sci_tests)
```

This intentionally does not reference `RoundTripTests.cpp` or
`SlaveTests.cpp` yet, and has zero actual `TEST()` cases yet — that's
expected; `gtest_main` supplies a `main()` that runs zero tests.

Verify:
```bash
cd D:/Git/SCI/C
rm -rf build
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```
Expected: downloads GoogleTest sources (requires internet, confirmed
reachable), builds `build/Test/sci_tests.exe` with no compile errors.

```bash
cd build
ctest --output-on-failure
```
Expected: `No tests were found!!!` (CTest exit code non-zero is fine here —
there are genuinely no tests yet). This confirms the harness itself wires
up correctly before any test content is added.

Commit:
```bash
git add C/Test/CMakeLists.txt
git commit -m "build: wire up GoogleTest via CMake FetchContent (no tests yet)"
```

---

### Task 5: Port the first legacy test — UINT8 GetVar (TDD)

**File:** Create `C/Test/gtest/SlaveTests.cpp`

**Step 1 — write the test:**
```cpp
#include <gtest/gtest.h>

extern "C" {
#include "SCISlave.h"
#include "SCIMaster.h"
}

extern "C" {
    extern tsSCIVAR varStruct[];
    extern COMMAND_CB cmdStruct;
    extern tsSCI_SLAVE_CALLBACKS sSlaveTestCbs;
    extern char cTxMsgBuf[];
    extern char cRxMsgBuf[];
}

namespace {

class SlaveTest : public ::testing::Test {
protected:
    void SetUp() override {
        SCISlaveInit(sSlaveTestCbs, varStruct, &cmdStruct);
    }
};

// Pumps the slave statemachine, injecting one message byte per tick,
// mirroring the loop shape used by the pre-migration Unity tests.
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

TEST_F(SlaveTest, PollVarUI8) {
    uint8_t msg[]    = {0x02, '3', '?', 0x03};
    uint8_t expect[] = {0x02, '3', '?', 'A', 'C', 'K', ';', 'F', '5', 0x03};

    PumpSlave(msg, sizeof(msg));

    EXPECT_EQ(0, memcmp(expect, cTxMsgBuf, sizeof(expect)));
}

}  // namespace
```

**Step 2 — wire it into the build.** Update `C/Test/CMakeLists.txt`:
add `gtest/SlaveTests.cpp` to the `add_executable(sci_tests ...)` source
list, and add `gtest` (not just `gtest_main`) to `target_link_libraries` if
not already implied — `gtest_main` already pulls in `gtest`, so no change
needed there.

**Step 3 — run and verify it fails first.** Temporarily break the
assertion on purpose to prove the test harness actually executes it (this
substitutes for "write failing test" since we're porting known-good
legacy behavior, not developing new behavior — we still must prove the
test can fail before trusting it can pass):
```cpp
EXPECT_EQ(1, memcmp(expect, cTxMsgBuf, sizeof(expect)));  // deliberately wrong
```
```bash
cd D:/Git/SCI/C
cmake --build build
cd build && ctest --output-on-failure
```
Expected: `1 test, 0 passed, 1 failed` — confirms the test actually runs
and can fail.

**Step 4 — fix the assertion back to `EXPECT_EQ(0, ...)` and re-run:**
```bash
cmake --build ../build
ctest --output-on-failure
```
Expected: `100% tests passed, 0 tests failed out of 1`.

**Step 5 — commit:**
```bash
git add C/Test/gtest/SlaveTests.cpp C/Test/CMakeLists.txt
git commit -m "test: port PollVarUI8 to GoogleTest"
```

---

### Task 6: Port the remaining 3 legacy Slave tests

**File:** Modify `C/Test/gtest/SlaveTests.cpp` — add three more `TEST_F`
cases inside the `SlaveTest` fixture, same `PumpSlave` helper, same
deliberate-failure-then-fix TDD discipline as Task 5 but you may batch the
fail/pass verification for all three together to save time:

```cpp
TEST_F(SlaveTest, PollVarUI16) {
    uint8_t msg[]    = {0x02, '4', '?', 0x03};
    uint8_t expect[] = {0x02, '4', '?', 'A', 'C', 'K', ';', '8', '6', 'E', '6', 0x03};
    PumpSlave(msg, sizeof(msg));
    EXPECT_EQ(0, memcmp(expect, cTxMsgBuf, sizeof(expect)));
}

TEST_F(SlaveTest, PollVarI32) {
    uint8_t msg[]    = {0x02, '5', '?', 0x03};
    uint8_t expect[] = {0x02, '5', '?', 'A', 'C', 'K', ';', 'F', 'A', 'C', 'B', '3', 'B', '0', '3', 0x03};
    PumpSlave(msg, sizeof(msg));
    EXPECT_EQ(0, memcmp(expect, cTxMsgBuf, sizeof(expect)));
}

TEST_F(SlaveTest, PollVarF32) {
    uint8_t msg[]    = {0x02, '1', '?', 0x03};
    uint8_t expect[] = {0x02, '1', '?', 'A', 'C', 'K', ';', '4', '0', '1', '6', 'C', '8', 'B', '4', 0x03};
    PumpSlave(msg, sizeof(msg));
    EXPECT_EQ(0, memcmp(expect, cTxMsgBuf, sizeof(expect)));
}
```

Verify:
```bash
cd D:/Git/SCI/C
cmake --build build
cd build && ctest --output-on-failure
```
Expected: `100% tests passed, 0 tests failed out of 4`.

Commit:
```bash
git add C/Test/gtest/SlaveTests.cpp
git commit -m "test: port remaining legacy Slave poll tests to GoogleTest"
```

---

### Task 7: Delete the retired Unity test files

**Files:**
- Delete: `C/Test/UnitTests.c`
- Delete: `C/Test/UnitTest` (stale checked-in binary — was Linux ELF,
  wrong platform for this repo's Windows/MinGW workflow anyway)

```bash
git rm C/Test/UnitTests.c
git rm C/Test/UnitTest
```

Verify:
```bash
cd D:/Git/SCI/C
cmake --build build
cd build && ctest --output-on-failure
```
Expected: still `100% tests passed ... out of 4` — proves the deleted
files were never part of the new build (they weren't referenced by
`C/Test/CMakeLists.txt`).

Commit:
```bash
git commit -m "test: remove retired Unity test runner and stale binary"
```

---

### Task 8: Fix the confirmed `SCITransferStart` return-path bug

**File:** Modify `C/Master/Src/SCIMasterTransfer.c:35-48`

Current code (confirmed by compiler warning
`control reaches end of non-void function [-Wreturn-type]`):
```c
bool SCITransferStart (tsSCI_TRANSFER *psSciTransfer, teREQUEST_TYPE eReqType, int16_t i16CmdNum, tuREQUESTVALUE *uVal, uint8_t ui8ArgNum)
{
    tsREQUEST sReq = tsREQUEST_DEFAULTS;
    // Take over the arguments
    sReq.eReqType       = eReqType;
    sReq.i16Num         = i16CmdNum;
    sReq.uValArr        = uVal;
    sReq.ui8ValArrLen   = ui8ArgNum;

    if(!psSciTransfer->sCallbacks.RequestCB(sReq))
        return false;

    psSciTransfer->sTransferInfo.sReq = sReq;
}
```

Fix — add the missing `return true;`:
```c
bool SCITransferStart (tsSCI_TRANSFER *psSciTransfer, teREQUEST_TYPE eReqType, int16_t i16CmdNum, tuREQUESTVALUE *uVal, uint8_t ui8ArgNum)
{
    tsREQUEST sReq = tsREQUEST_DEFAULTS;
    // Take over the arguments
    sReq.eReqType       = eReqType;
    sReq.i16Num         = i16CmdNum;
    sReq.uValArr        = uVal;
    sReq.ui8ValArrLen   = ui8ArgNum;

    if(!psSciTransfer->sCallbacks.RequestCB(sReq))
        return false;

    psSciTransfer->sTransferInfo.sReq = sReq;
    return true;
}
```

Verify — the warning disappears and existing tests stay green:
```bash
cd D:/Git/SCI/C
rm -rf build && cmake -S . -B build -G "MinGW Makefiles"
cmake --build build 2>&1 | grep -i "SCIMasterTransfer.c:48"
```
Expected: no output (warning gone; previously this grep would have matched
the `control reaches end of non-void function` line).
```bash
cd build && ctest --output-on-failure
```
Expected: `100% tests passed, 0 tests failed out of 4` (unchanged — this
return value isn't consumed by any currently-tested path, so behavior is
unaffected; Task 9's round-trip test will be the first thing that actually
depends on the Master transfer path working correctly end to end).

Commit:
```bash
git add C/Master/Src/SCIMasterTransfer.c
git commit -m "fix: add missing return in SCITransferStart (was UB on success path)"
```

---

### Task 9: Add Master-side test callbacks (new — nothing to port)

The existing `C/Test/TestCallbacks.c` only wires Slave callbacks. The
Master side has never been initialized in any test, so there's no
"NonBlocking"/"Blocking" TX callback for it yet. Add one, symmetric to
`SlaveTxCbBlocking`.

**File:** Create `C/Test/gtest/MasterTestCallbacks.h`
```c
#ifndef _MASTERTESTCALLBACKS_H_
#define _MASTERTESTCALLBACKS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void MasterTxCbBlocking(uint8_t* pui8Data, uint8_t ui8Size);

#ifdef __cplusplus
}
#endif

#endif // _MASTERTESTCALLBACKS_H_
```

**File:** Create `C/Test/gtest/MasterTestCallbacks.c`
```c
#include "MasterTestCallbacks.h"
#include "SCISlave.h"

// Mirrors SlaveTxCbBlocking in TestCallbacks.c: forwards every byte the
// Master transmits directly into the Slave's receive function, in-process.
// Valid only because both sides run in the same test binary/address space.
void MasterTxCbBlocking(uint8_t* pui8Data, uint8_t ui8Size)
{
    for (uint8_t i = 0; i < ui8Size; i++)
        SCISlaveReceiveData(pui8Data[i]);
}
```

Add both files to `C/Test/CMakeLists.txt`'s `add_executable(sci_tests ...)`
source list.

Verify (no test references it yet — just confirm it compiles/links):
```bash
cd D:/Git/SCI/C
cmake --build build
```
Expected: builds cleanly, no new warnings/errors from the new files.

Commit:
```bash
git add C/Test/gtest/MasterTestCallbacks.c C/Test/gtest/MasterTestCallbacks.h C/Test/CMakeLists.txt
git commit -m "test: add Master-side blocking TX test callback"
```

---

### Task 10: Write the failing Master↔Slave round-trip test

This is the actual point of this plan: prove (or disprove) that a full
GetVar request/response cycle works end-to-end between the Master and
Slave code as they exist today.

**File:** Create `C/Test/gtest/RoundTripTests.cpp`

**Step 1 — write the test:**
```cpp
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
```

Add `gtest/RoundTripTests.cpp` to `C/Test/CMakeLists.txt`'s source list.

**Step 2 — run and observe the actual result (this IS the failing-test
step — we genuinely don't know if it passes):**
```bash
cd D:/Git/SCI/C
cmake --build build
cd build && ctest --output-on-failure
```

Two honest outcomes are possible here, and both are useful information:

- **If it fails:** read the `ASSERT_TRUE`/`EXPECT_EQ` failure message
  carefully. Common places this can break given the code as read:
  `SCIMasterInit` doesn't wire the `GetTxBusyStateExternalCB`, so if a
  future non-blocking mode were involved it would misbehave — but current
  config is `SEND_MODE_BYTE_BY_BYTE`, so that specific path is inactive.
  More likely failure points to check first: whether `PumpUntilIdle`'s
  iteration budget is large enough (increase `maxIterations`), or whether
  the Slave's own state machine needs to run enough extra cycles after
  Master TX to finish `ePROTOCOL_SENDING`. Add temporary `printf`/gtest
  `RecordProperty` tracing of `SCIGetProtocolState()` and
  `SCIDatalinkGetTransmitState()`/`SCIDatalinkGetReceiveState()` each
  iteration if it's not obvious. **Do not weaken the assertions to force a
  pass** — this test's entire purpose is to tell the truth about whether
  the round trip works.
- **If it passes on the first try:** good — that's the answer to "does
  this code work end-to-end," now backed by a real test instead of a
  guess. Still worth adding one iteration-budget stress check: rerun with
  `maxIterations` temporarily set to something absurdly small (e.g. `5`)
  to confirm `PumpUntilIdle` correctly reports `finished == false` and the
  test correctly fails in that case — proving the safety net isn't a
  tautology. Then restore `maxIterations` to `500`.

**Step 3 — once genuinely green, commit:**
```bash
git add C/Test/gtest/RoundTripTests.cpp C/Test/CMakeLists.txt
git commit -m "test: add first Master<->Slave round-trip integration test (GetVar)"
```

If this task uncovers a real protocol/state-machine bug rather than a test
setup mistake, stop and treat it as a new task: write down the exact
failure, fix the minimal cause in the relevant `C/Master/Src/*.c` or
`C/Slave/Src/*.c` file, re-run, and commit the fix separately from the
test (small, reviewable diffs) before returning to this task's own commit.

---

### Task 11: Wire CTest labels and a single `ctest` entry point

**File:** Modify `C/Test/CMakeLists.txt` — confirm `gtest_discover_tests`
is already present (it was added in Task 4); add test labels so Slave-only
vs. round-trip tests can be filtered independently later:

```cmake
gtest_discover_tests(sci_tests
    PROPERTIES LABELS "sci"
)
```

Verify:
```bash
cd D:/Git/SCI/C/build
ctest --output-on-failure
ctest -L sci --output-on-failure
```
Expected: both commands show `100% tests passed, 0 tests failed out of 5`
(4 legacy-ported + 1 round-trip).

Commit:
```bash
git add C/Test/CMakeLists.txt
git commit -m "test: label discovered tests for future filtering"
```

---

### Task 12: Replace the stale VS Code build task

**File:** Modify `.vscode/tasks.json`

The current default task invokes `gcc.exe` directly against a glob that
assumes `${workspaceFolder}` is `C/`, and it built the now-deleted
Unity-based `UnitTest.exe`. Replace it with a CMake-driven task:

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "type": "shell",
            "label": "CMake Configure",
            "command": "cmake",
            "args": ["-S", ".", "-B", "build", "-G", "MinGW Makefiles"],
            "options": { "cwd": "${workspaceFolder}/C" }
        },
        {
            "type": "shell",
            "label": "Build and Run Tests",
            "command": "cmake",
            "args": ["--build", "build"],
            "options": { "cwd": "${workspaceFolder}/C" },
            "dependsOn": ["CMake Configure"],
            "problemMatcher": ["$gcc"],
            "group": { "kind": "test", "isDefault": true }
        }
    ]
}
```

Verify manually by re-running the same commands from a clean checkout of
this branch (simulates what the VS Code task will do):
```bash
cd D:/Git/SCI/C
rm -rf build
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
cd build && ctest --output-on-failure
```
Expected: `100% tests passed, 0 tests failed out of 5`.

Commit:
```bash
git add .vscode/tasks.json
git commit -m "build: point VS Code default task at CMake instead of raw gcc"
```

---

### Task 13: Update `.hermes.md` with the new build/test workflow

**File:** Modify `.hermes.md`, `## Build / test` section. Keep the existing
documentation style (short prose + fenced command blocks, no new
headings). Replace the current C-unit-test subsection (MinGW gcc glob +
Unity submodule init instructions) with:

```markdown
- **C unit tests (CMake + GoogleTest, Windows/MinGW; tested with
  `cmake` 3.23, MinGW-W64 gcc/g++ 7.3.0):**
  ```
  cd C
  cmake -S . -B build -G "MinGW Makefiles"
  cmake --build build
  cd build && ctest --output-on-failure
  ```
  GoogleTest is pulled automatically by CMake `FetchContent` (pinned to
  `v1.15.2`) — no git submodule, but the first configure needs internet
  access. Test sources live in `C/Test/gtest/`; `C/Test/TestCallbacks.c`
  and `C/Test/VariablesAndCommands.c` provide the shared Slave fixture
  data and callbacks reused by all tests. `C/Test/gtest/RoundTripTests.cpp`
  is the only test that exercises the Master and Slave together in one
  process — everything else only covers the Slave in isolation.
```

Also update the directory map bullet for `C/Test/` (currently says "unit
tests on the Unity framework... `C/Test/Unity` is a git submodule... NOT
checked out by default") to reflect GoogleTest instead, and remove the
now-stale submodule-init instruction from the same bullet.

Verify: re-read the file and confirm no remaining references to Unity, to
`git submodule update --init`, or to the deleted
`${workspaceFolder}\*.c`-glob gcc command.
```bash
grep -in "unity" .hermes.md
```
Expected: no output.

Commit:
```bash
git add .hermes.md
git commit -m "doc: update .hermes.md build/test section for CMake + GoogleTest"
```

---

### Task 14: Final full-suite verification and push

```bash
cd D:/Git/SCI/C
rm -rf build
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
cd build && ctest --output-on-failure
```
Expected: `100% tests passed, 0 tests failed out of 5`.

```bash
cd D:/Git/SCI
git log --oneline master..test/googletest-migration
```
Expected: one commit per task above (roughly 12-13 commits), each with a
clear, scoped message.

```bash
git push -u origin test/googletest-migration
```

Do not merge into `master` as part of this task — open the branch for
review per the repo's workflow rule; merging is a separate, explicit
decision for the user.

---

## Tests / validation summary

Every behavioral task above (5, 6, 8, 10) follows write→run(prove it can
fail or genuinely doesn't know the answer)→fix/implement→run(green)→commit.
Scaffolding tasks (1-4, 7, 9, 11-13) use "configure/build/run and check the
expected CMake/CTest output" as their equivalent verification step, since
there's no application behavior to assert on yet at that point.

Final state: `ctest` from `C/build` reports 5 passing tests — 4 ported
legacy Slave-only tests plus 1 new Master↔Slave round-trip test — with zero
Unity references left in the repo.

---

## Risks, tradeoffs, and open questions

- **Task 10 (round-trip test) is the one genuinely open question in this
  plan.** Every other task is mechanical (porting/scaffolding). Whether
  the Master and Slave state machines actually interoperate correctly
  today is unverified as of writing this plan — that's the whole reason
  this work was requested. The plan is honest about this: it does not
  assume a specific outcome, and explicitly forbids weakening assertions
  to force a pass.
- **In-process loopback is not a full transport test.** `MasterTxCbBlocking`
  calls `SCISlaveReceiveData` directly in the same call stack/process —
  this proves the protocol logic and state machines are correct, but does
  **not** prove anything about real serial hardware, timing, or the
  non-blocking (`SEND_MODE_BYTE_BY_BYTE` undefined) code path, which is
  currently unused by `SCIconfig.h` and therefore also untested by this
  plan. Flag as a follow-up if that mode is ever needed.
- **GoogleTest via FetchContent requires internet on every clean
  `build/` dir** (verified reachable during planning, but this is a new
  external dependency vs. the old — never-checked-out — submodule
  approach). If fully offline CI is ever needed, switch to
  `FetchContent_Declare(... SOURCE_DIR <vendored-path>)` or reintroduce a
  submodule pinned to a GoogleTest release, and note that as a future
  change to this same `C/Test/CMakeLists.txt`.
- **`MinGW Makefiles` generator is hardcoded** in verification commands and
  the VS Code task because that's the confirmed working toolchain on this
  machine (`mingw32-make` 4.2.1 present, `ninja` absent). If the
  implementer is on a different machine/OS, swap the `-G` argument
  accordingly (e.g. `Ninja` or `Unix Makefiles` on Linux/macOS) — the
  `CMakeLists.txt` files themselves are generator-agnostic.
- **Python side is entirely out of scope** for this plan — no Python files
  are touched, per `.hermes.md`'s existing guidance to split C and Python
  work.
- **Open question for the user:** should the checked-in stale
  `C/Test/UnitTest` Linux ELF binary's removal (Task 7) also prompt adding
  a `.gitignore` rule for `C/build/` so future build artifacts don't get
  accidentally committed again? This plan does not add one — worth a
  quick decision before or during implementation.
