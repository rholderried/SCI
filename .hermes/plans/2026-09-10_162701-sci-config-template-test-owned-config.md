# SCIconfig.h: Template + Test-Owned Config Implementation Plan

> **For Hermes:** Use subagent-driven-development skill to implement this plan task-by-task, on a dedicated branch (never on `master` per this repo's workflow rules).

**Goal:** Turn `C/config/SCIconfig.h` into a non-compiled template/example, and give the GoogleTest suite (`C/Test`) its own real, self-contained `SCIconfig.h`, so the `sci` library never silently ships one hardcoded configuration to every consumer.

**Architecture:** Rename `C/config/SCIconfig.h` to `C/config/SCIconfig_Template.h` (documentation only, never on any include path), create a real `C/Test/config/SCIconfig.h` owned by the test suite, and replace the `sci` library's hardcoded `config` include directory in `C/CMakeLists.txt` with a `SCI_CONFIG_DIR` CMake cache variable that defaults to the test config (so `cmake --build` on this repo still "just works") but otherwise must be supplied explicitly by whoever integrates the library — making the "bring your own config" contract real instead of accidental.

**Tech Stack:** CMake 3.14+, C11/C++17, GoogleTest (via FetchContent, already wired).

---

## Current context / assumptions

- Confirmed by direct inspection (2026-09-10):
  - `C/config/SCIconfig.h` (49 lines) defines buffer sizes, `SIZE_OF_VAR_STRUCT`,
    `SIZE_OF_CMD_STRUCT`, EEPROM layout, etc. Its values (`SIZE_OF_VAR_STRUCT 5`)
    are tuned to match the 5-entry `varStruct[]` fixture in
    `C/Test/VariablesAndCommands.c` — i.e. today's "generic default config" is
    actually the test suite's own config, just not labeled or owned as such.
  - 11 files across `C/Common`, `C/Slave`, `C/Master`, `C/Test` do
    `#include "SCIconfig.h"` **unqualified** (no relative path). This is a
    deliberate pattern (same idea as `FreeRTOSConfig.h`/`lwipopts.h`): library
    source expects the *integrating project* to supply this header via the
    compiler's include path, not to find a bundled copy.
  - `C/CMakeLists.txt` currently adds `config` (i.e. `C/config/`) to the `sci`
    library's `PUBLIC` include directories (`C/CMakeLists.txt:22-27`). This is
    what actually breaks the "template" intent: it wires one specific config
    into the library target itself, and every consumer (today, only
    `C/Test` via `target_link_libraries(sci_tests PRIVATE sci)`) inherits it
    transitively whether or not it's appropriate for them.
  - `C/Test/CMakeLists.txt` does **not** add its own config include directory
    today — it gets `SCIconfig.h` for free through the `sci` target's
    `PUBLIC` include dir. This still works after the change below, because
    CMake propagates a linked target's `PUBLIC`/`INTERFACE` include
    directories to the consumer's own compilation regardless of the
    `PRIVATE`/`PUBLIC` keyword used in `target_link_libraries` — so no edit
    to `C/Test/CMakeLists.txt` is needed as long as `sci`'s config include
    dir points at `C/Test/config`.
  - Existing build in `D:\Git\SCI\C\build` was verified clean and all 5 tests
    pass before this change (`ctest --output-on-failure`: 100% tests passed).
  - No other tracked file in the repo references `C/config/SCIconfig.h` by
    path (checked `README.md`, `.hermes.md`, `.gitignore`) — only `.hermes.md`
    names the file, and that gets updated in this plan (Task 8).
  - `Doxyfile` is gitignored (not tracked), so no doc-generation config
    references the old path in-repo.
- Assumption: preserving the exact numeric config values used today
  (`RX_PACKET_LENGTH 128`, `SIZE_OF_VAR_STRUCT 5`, etc.) is required so the
  existing 5 GoogleTest cases keep passing unmodified — this plan only moves
  and re-labels the file, it does not change any `#define` value.
- Assumption: `SCI_CONFIG_DIR` should default to `C/Test/config` when
  `BUILD_TESTS=ON` (the common case — `cmake --build` on a fresh clone must
  keep working with zero extra flags), and hard-fail with a clear message
  when `BUILD_TESTS=OFF` and no `SCI_CONFIG_DIR` was given, since in that
  case nothing else in the CMake tree can supply a config. Flagged as an
  open question below in case the user prefers always-explicit.

## Step-by-step tasks

### Task 0: Create a working branch

Never commit on `master` (repo workflow rule).

```bash
cd /d/Git/SCI
git checkout master
git pull --ff-only
git checkout -b refactor/sci-config-template
```

Verify: `git branch` shows `* refactor/sci-config-template`.

---

### Task 1: Create the test suite's own `SCIconfig.h`

**Objective:** Give `C/Test` a real, compilable config file it owns, with
identical `#define` values to today's `C/config/SCIconfig.h` (so no test
behavior changes), but a header comment that's honest about scope.

**Files:**
- Create: `C/Test/config/SCIconfig.h`

**Step 1: Write the file**

```c
/**************************************************************************//**
 * \file SCIconfig.h
 * \author Roman Holderried
 *
 * \brief SCI configuration owned by the C/Test GoogleTest suite.
 *
 * This header configures buffer sizes, var/command struct sizes, and other
 * compile-time SCI parameters used ONLY by the C/Test GoogleTest suite
 * (VariablesAndCommands.c, TestCallbacks.c, gtest/*.cpp). It is NOT a
 * generic library default — SCI is platform independent and every
 * integration is expected to supply its own SCIconfig.h. See
 * C/config/SCIconfig_Template.h for a documented starting point to copy
 * into your own project.
 *
 * <b> History </b>
 * 	- 2022-01-13 - File creation (as C/config/SCIconfig.h)
 *  - 2022-03-17 - Port to C (Originally from SerialProtocol)
 *  - 2022-12-13 - Adapted code for unified master/slave repo structure.
 *  - 2026-09-10 - Split off as C/Test/config/SCIconfig.h, the GoogleTest
 *                 suite's own configuration; C/config/SCIconfig.h became a
 *                 template (see C/config/SCIconfig_Template.h).
 *****************************************************************************/

#ifndef _SCICONFIG_H_
#define _SCICONFIG_H_

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "SCICommon.h"

/******************************************************************************
 * Defines
 *****************************************************************************/
#define RX_PACKET_LENGTH    128
#define TX_PACKET_LENGTH    128

#define SIZE_OF_VAR_STRUCT  5
#define SIZE_OF_CMD_STRUCT  2
#define MAX_NUMBER_OF_EEPROM_VARS 10

// Mode configuration
#define SEND_MODE_BYTE_BY_BYTE
#define VALUE_MODE_HEX

// EEPROM configuration
#define EEPROM_ADDRESSTYPE  EEPROM_WORD_ADDRESSABLE
#define ADDRESS_OFFET       0

// SCI error offset (SCI currently defines 11 errors)
#define SCI_ERROR_OFFSET    0x100

// Number of request and response values
#define MAX_NUM_REQUEST_VALUES  10
#define MAX_NUM_RESPONSE_VALUES 10

#endif // _SCICONFIG_H_
```

**Verify:** `test -f C/Test/config/SCIconfig.h && echo OK` prints `OK`. Don't
build yet — the CMake wiring (Task 3) isn't in place, so a build now would
still use the old `C/config/` path.

**Commit:**

```bash
git add C/Test/config/SCIconfig.h
git commit -m "test: add C/Test's own SCIconfig.h (not yet wired into CMake)"
```

---

### Task 2: Turn `C/config/SCIconfig.h` into a documented, non-compiled template

**Objective:** Rename the old config to make clear it's a copy-and-adapt
starting point, never something the build compiles directly.

**Files:**
- Rename: `C/config/SCIconfig.h` → `C/config/SCIconfig_Template.h`

**Step 1: Rename with git (preserves history/blame)**

```bash
git mv C/config/SCIconfig.h C/config/SCIconfig_Template.h
```

**Step 2: Rewrite its content**

Replace the full contents of `C/config/SCIconfig_Template.h` with:

```c
/**************************************************************************//**
 * \file SCIconfig_Template.h
 * \author Roman Holderried
 *
 * \brief TEMPLATE SCI configuration — not compiled by this repo's build.
 *
 * SCI is designed to be platform independent: every file under Common/,
 * Master/, and Slave/ does `#include "SCIconfig.h"` unqualified, and
 * expects the *integrating project* to provide that header via its own
 * include path — this repo does not ship one canonical config.
 *
 * To use SCI in your own project:
 *   1. Copy this file into your project, e.g. `myproject/sci_config/SCIconfig.h`
 *      (the filename must be exactly `SCIconfig.h`, only the directory differs).
 *   2. Adjust the values below for your target/application (buffer sizes,
 *      var-struct size, EEPROM layout, transfer mode, etc.).
 *   3. Point your build's include path at that directory. With this repo's
 *      CMake build, that's `-DSCI_CONFIG_DIR=<dir containing your SCIconfig.h>`
 *      (see C/CMakeLists.txt).
 *
 * The C/Test GoogleTest suite in this repo has its own copy at
 * C/Test/config/SCIconfig.h, wired in automatically when BUILD_TESTS=ON.
 * Do not edit that file to experiment with values — copy this template
 * instead and point SCI_CONFIG_DIR at your copy.
 *
 * <b> History </b>
 * 	- 2022-01-13 - File creation (as SCIconfig.h)
 *  - 2022-03-17 - Port to C (Originally from SerialProtocol)
 *  - 2022-12-13 - Adapted code for unified master/slave repo structure.
 *  - 2026-09-10 - Repurposed as a template; the GoogleTest suite now owns
 *                 its own copy at C/Test/config/SCIconfig.h.
 *****************************************************************************/

#ifndef _SCICONFIG_H_
#define _SCICONFIG_H_

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "SCICommon.h"

/******************************************************************************
 * Defines
 *****************************************************************************/
#define RX_PACKET_LENGTH    128
#define TX_PACKET_LENGTH    128

#define SIZE_OF_VAR_STRUCT  5
#define SIZE_OF_CMD_STRUCT  2
#define MAX_NUMBER_OF_EEPROM_VARS 10

// Mode configuration
#define SEND_MODE_BYTE_BY_BYTE
#define VALUE_MODE_HEX

// EEPROM configuration
#define EEPROM_ADDRESSTYPE  EEPROM_WORD_ADDRESSABLE
#define ADDRESS_OFFET       0

// SCI error offset (SCI currently defines 11 errors)
#define SCI_ERROR_OFFSET    0x100

// Number of request and response values
#define MAX_NUM_REQUEST_VALUES  10
#define MAX_NUM_RESPONSE_VALUES 10

#endif // _SCICONFIG_H_
```

(Values are unchanged from the original — only the file header comment
changed. The include guard stays `_SCICONFIG_H_` so a direct copy-and-rename
works without further edits, since this file is never itself on a build
include path simultaneously with another `SCIconfig.h`.)

**Verify:** `test -f C/config/SCIconfig_Template.h && ! test -f C/config/SCIconfig.h && echo OK` prints `OK`.

**Commit:**

```bash
git add C/config/SCIconfig_Template.h
git commit -m "docs: turn C/config/SCIconfig.h into SCIconfig_Template.h"
```

---

### Task 3: Wire `SCI_CONFIG_DIR` into `C/CMakeLists.txt`

**Objective:** Stop hardcoding `config` as the library's include directory;
require an explicit (or sensibly-defaulted) `SCI_CONFIG_DIR` instead.

**Files:**
- Modify: `C/CMakeLists.txt`

**Step 1: Replace the file's full contents**

```cmake
cmake_minimum_required(VERSION 3.14)
project(sci C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTS "Build the sci_tests GoogleTest suite" ON)

# SCIconfig.h is intentionally NOT bundled as a fixed part of this library.
# Every file under Common/, Master/, and Slave/ does `#include "SCIconfig.h"`
# unqualified, and expects the integrating project to supply that header on
# the include path (buffer sizes, var-struct size, EEPROM layout, etc. are
# all target/application specific). See config/SCIconfig_Template.h for a
# documented starting point to copy into your own project.
#
# SCI_CONFIG_DIR must point at a directory containing your SCIconfig.h. When
# BUILD_TESTS is ON and SCI_CONFIG_DIR is not set, it defaults to the
# GoogleTest suite's own config (Test/config), so `cmake --build` works out
# of the box for running this repo's own tests.
set(SCI_CONFIG_DIR "" CACHE PATH "Directory containing your project's SCIconfig.h")

if(NOT SCI_CONFIG_DIR)
    if(BUILD_TESTS)
        set(SCI_CONFIG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Test/config")
    else()
        message(FATAL_ERROR
            "SCI_CONFIG_DIR is not set. The sci library requires an "
            "application-supplied SCIconfig.h (see config/SCIconfig_Template.h). "
            "Configure with -DSCI_CONFIG_DIR=<dir containing your SCIconfig.h>, "
            "or enable -DBUILD_TESTS=ON to use the bundled test config.")
    endif()
endif()

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
    ${SCI_CONFIG_DIR}
)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(Test)
endif()
```

**Verify (syntax only, no build yet):** `mcp__patch`/`write_file`'s automatic
CMake lint (if any) shows no new errors; otherwise proceed to Task 4's build
verification, which will catch any typo.

**Commit:**

```bash
git add C/CMakeLists.txt
git commit -m "build: replace hardcoded config include dir with SCI_CONFIG_DIR"
```

---

### Task 4: Verify the default (repo test) build still passes

**Objective:** Prove the change is behavior-preserving for the common case —
running this repo's own test suite.

**Commands:**

```bash
cd /d/Git/SCI/C
rm -rf build
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
cd build && ctest --output-on-failure
```

**Expected output (last command):**

```
Test project D:/Git/SCI/C/build
    Start 1: RoundTripTest.GetVarUI8RoundTrip
1/5 Test #1: RoundTripTest.GetVarUI8RoundTrip ...   Passed
    Start 2: SlaveTest.PollVarUI8
2/5 Test #2: SlaveTest.PollVarUI8 ...............   Passed
    Start 3: SlaveTest.PollVarUI16
3/5 Test #3: SlaveTest.PollVarUI16 ..............   Passed
    Start 4: SlaveTest.PollVarI32
4/5 Test #4: SlaveTest.PollVarI32 ...............   Passed
    Start 5: SlaveTest.PollVarF32
5/5 Test #5: SlaveTest.PollVarF32 ...............   Passed

100% tests passed, 0 tests failed out of 5
```

If any test fails or the build errors on a missing `SCIconfig.h`, re-check
Task 1's file path (`C/Test/config/SCIconfig.h`, exact filename) and Task 3's
`SCI_CONFIG_DIR` default path before continuing — do not proceed to Task 5
until this passes cleanly.

No commit needed (verification only, no file changes).

---

### Task 5: Verify a library-only build fails without a config

**Objective:** Prove the library no longer silently defaults to a bundled
config when built standalone (the core of "template, not a default").

**Commands:**

```bash
cd /d/Git/SCI/C
rm -rf build_libonly
cmake -S . -B build_libonly -G "MinGW Makefiles" -DBUILD_TESTS=OFF
```

**Expected output:** CMake configure step fails with:

```
CMake Error at CMakeLists.txt:22 (message):
  SCI_CONFIG_DIR is not set. The sci library requires an
  application-supplied SCIconfig.h (see config/SCIconfig_Template.h).
  Configure with -DSCI_CONFIG_DIR=<dir containing your SCIconfig.h>, or
  enable -DBUILD_TESTS=ON to use the bundled test config.
```

(Exact line number may differ slightly — the important part is the
`FATAL_ERROR` message text and a non-zero exit code.)

Clean up the failed config dir:

```bash
rm -rf /d/Git/SCI/C/build_libonly
```

No commit needed (verification only).

---

### Task 6: Verify a library-only build succeeds with an explicit config

**Objective:** Prove a downstream integrator can supply their own config and
get a working library build with no test suite involved.

**Commands:**

```bash
cd /d/Git/SCI/C
rm -rf build_libonly
cmake -S . -B build_libonly -G "MinGW Makefiles" -DBUILD_TESTS=OFF -DSCI_CONFIG_DIR="$(pwd)/Test/config"
cmake --build build_libonly
```

**Expected output:** Configure succeeds (no `FATAL_ERROR`), build succeeds,
and `build_libonly/libsci.a` exists with no `sci_tests` executable produced
(since `BUILD_TESTS=OFF` skips `add_subdirectory(Test)`).

Verify: `test -f build_libonly/libsci.a && ! test -f build_libonly/Test/sci_tests* && echo OK` prints `OK`.

Clean up:

```bash
rm -rf /d/Git/SCI/C/build_libonly
```

No commit needed (verification only; `build_libonly/` is caught by the
existing `.gitignore` pattern `C/build/`? — no, it is NOT, since the pattern
is `C/build/` specifically. Confirm `git status` shows nothing untracked
before moving on, or add `build_libonly/` explicitly to `.gitignore` if you
want to keep this verification directory name around — otherwise always
`rm -rf` it as shown above so it never gets committed by accident.)

---

### Task 7: Update `.gitignore` if verification directories are kept

Only needed if Task 5/6's `build_libonly/` directory is intentionally kept
around for repeat verification instead of deleted each time. If so:

**Files:**
- Modify: `.gitignore`

Add a line:

```
C/build_libonly/
```

Otherwise skip this task — the plan as written deletes `build_libonly/`
after each verification run, so nothing untracked survives.

**Commit (only if applied):**

```bash
git add .gitignore
git commit -m "chore: ignore C/build_libonly/ verification build directory"
```

---

### Task 8: Update `.hermes.md`

**Objective:** Keep the project context file in sync with the new layout, as
required by this repo's own Hermes rules ("If you touch directory
structure... update this file in the same commit/session").

**Files:**
- Modify: `.hermes.md`

**Step 1: Update the directory map entry**

Find this line in `.hermes.md`:

```
- `C/config/SCIconfig.h` — build-time configuration (buffer sizes etc.).
```

Replace it with:

```
- `C/config/SCIconfig_Template.h` — documented **template** for the
  build-time SCI configuration (buffer sizes, var/command struct sizes,
  EEPROM layout, etc.). Not compiled by this repo's build; copy it into your
  own project and adapt. Every SCI source file does `#include "SCIconfig.h"`
  unqualified and expects the integrating project/build to supply that
  header via its own include path — see `C/CMakeLists.txt`'s
  `SCI_CONFIG_DIR` cache variable.
```

**Step 2: Note the test suite's own config in the `C/Test/` bullet**

Find the `C/Test/` bullet describing `C/Test/gtest/`, and append a sentence
mentioning `C/Test/config/SCIconfig.h` — the test suite's own copy, wired in
automatically by `C/CMakeLists.txt` when `BUILD_TESTS=ON` (the default).

**Step 3: Update the Build / test section**

In the `C unit tests` bullet, after the existing `cmake -S . -B build ...`
block, add a short note:

```
  The `sci` library requires an application-supplied `SCIconfig.h` via the
  `SCI_CONFIG_DIR` CMake cache variable (see `C/CMakeLists.txt`). It
  defaults to `C/Test/config` when `BUILD_TESTS=ON` (the default), so the
  commands above work with no extra flags. For a library-only build, pass
  `-DBUILD_TESTS=OFF -DSCI_CONFIG_DIR=<dir containing your SCIconfig.h>` —
  see `C/config/SCIconfig_Template.h` for a documented starting point.
```

**Verify:** `search_files(pattern="C/config/SCIconfig.h", path=".hermes.md")`
returns no matches (old path fully replaced); `search_files(pattern="SCI_CONFIG_DIR", path=".hermes.md")` returns at least one match.

**Commit:**

```bash
git add .hermes.md
git commit -m "docs: update .hermes.md for SCIconfig.h template split"
```

---

### Task 9: Add a short "Configuration" note to `README.md`

**Objective:** Document the config contract for anyone reading the protocol
spec, not just Hermes sessions.

**Files:**
- Modify: `README.md`

**Step 1: Locate the insertion point**

In the `## General` section, find this existing paragraph (around line 25-33):

```
Although the code is written to be highly portable across platforms (no hardware specific
code), care must be taken about the following aspects:

- The code uses dynamic memory allocation, which can be problematic in MISRA-C compliant 
code.
- The code makes reinterprets variable values by using the union data type which as for ANSI C
causes 'undefined behaviour'. Whether the code works as intended is therefore dependent
on the compiler. It has been proven to work on several platforms though, including STM32 
and Microchip PIC processors, as well as the GNU compiler collection.
```

**Step 2: Insert a new subsection right after it, before `## Layered design`**

```markdown
### Configuration

SCI has no single fixed configuration. Every source file includes an
unqualified `SCIconfig.h` and expects the integrating project to supply it
via the build's include path — buffer sizes, the size of the var struct, the
EEPROM addressing mode, and similar target/application-specific values all
live there. `C/config/SCIconfig_Template.h` is a documented starting point:
copy it into your project, rename it to `SCIconfig.h`, adjust the values,
and point your build at the containing directory. The C/CMake test suite in
this repo (`C/Test`) is one example integration — it keeps its own copy at
`C/Test/config/SCIconfig.h`, wired in through the `SCI_CONFIG_DIR` CMake
variable in `C/CMakeLists.txt`.
```

**Verify:** `search_files(pattern="Configuration", path="README.md")` shows
the new heading; render check not required (plain markdown, no build step).

**Commit:**

```bash
git add README.md
git commit -m "docs: document the SCIconfig.h template/config contract in README"
```

---

### Task 10: Final full verification + push

**Commands:**

```bash
cd /d/Git/SCI/C
rm -rf build
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
cd build && ctest --output-on-failure
cd /d/Git/SCI
git status
git log --oneline master..refactor/sci-config-template
```

**Expected:** `ctest` shows `100% tests passed, 0 tests failed out of 5`
(same as Task 4); `git status` shows a clean tree; `git log` shows the
sequence of commits from Tasks 1-3, 8, 9 (and 7 if applied).

Do not push or open a PR unless the user asks — leave the branch ready for
review.

---

## Tests / validation summary

This is an infra/build-config change, not application logic, so validation
is CMake configure/build/test behavior rather than unit tests:

| Task | Verifies | Pass criterion |
|------|----------|-----------------|
| 4 | Default repo build (BUILD_TESTS=ON, no flags) | `ctest` 5/5 passed, unchanged from baseline |
| 5 | Library-only build with no config supplied | CMake configure fails with the `SCI_CONFIG_DIR` `FATAL_ERROR` message |
| 6 | Library-only build with explicit `-DSCI_CONFIG_DIR` | Configure + build succeed, `libsci.a` produced, no test executable |

Run Task 4 before Tasks 5/6 (confirms nothing is broken before proving the
new guard rail exists), and re-run Task 4 once more at the end (Task 10) as
the final regression check.

## Risks, tradeoffs, and open questions

- **Breaking change for any out-of-repo consumer** that already hardcoded
  `C/config/SCIconfig.h` or relied on `sci`'s `PUBLIC` include dir always
  containing a config: after this change they must pass
  `-DSCI_CONFIG_DIR=...` explicitly (or rely on the `BUILD_TESTS=ON`
  default, which only makes sense inside this repo's own test run). Given
  this is a solo-maintained repo with no known external consumers yet, this
  is judged acceptable, but flag it if that assumption is wrong.
- **Stale CMake cache risk:** `SCI_CONFIG_DIR` is a `CACHE PATH` variable —
  if someone reconfigures an existing `build/` directory without also
  clearing the cache, a previously-set value silently persists even after
  editing `C/CMakeLists.txt`'s default logic. The verification tasks above
  all use fresh `build*` directories for exactly this reason; recommend the
  same discipline (`rm -rf build` before reconfiguring) whenever
  `SCI_CONFIG_DIR` behavior is in question, consistent with this repo's
  existing `cmake-googletest` skill guidance.
- **Open question — default-on-BUILD_TESTS ergonomics vs. explicitness:**
  this plan defaults `SCI_CONFIG_DIR` to `Test/config` whenever
  `BUILD_TESTS=ON`, so `cmake -S . -B build` keeps working with zero extra
  flags. An alternative, stricter design would require `-DSCI_CONFIG_DIR`
  to always be explicit, even for the bundled test suite, to make the
  "bring your own config" contract impossible to overlook. Went with the
  ergonomic default since this repo's own CI/dev-loop convenience matters
  more day-to-day than that stricter guarantee — revisit if the user
  disagrees.
- **Not in scope:** the pre-existing `SIZE_OF_CMD_STRUCT 2` vs. the actual
  single-entry `cmdStruct` fixture in `C/Test/VariablesAndCommands.c`
  mismatch (noticed during investigation) is unrelated to this refactor and
  left untouched — flag separately if it turns out to matter.
- **Not in scope:** `Python/config/DataloggerCfg_Template.py` already
  follows the same "template, not default" naming pattern this plan applies
  to the C side — no Python changes are needed or proposed here.
