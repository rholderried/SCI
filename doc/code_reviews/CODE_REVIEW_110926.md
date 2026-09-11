# SCI Code Review

Informal grill-the-code review, done by reading the full tree: README, protocol
layers (Datalink / Transfer / Dataframe on both Master and Slave), Buffer/Helpers,
var-struct access, the Python driver, and the recent fix history recorded in file
headers. Intended as a learning aid and punch list, not a blocking gate.

## What's actually good

- Layered design is the right call for this domain: Datalink (framing) ->
  Transfer (state/sequencing) -> Dataframe (wire encoding) is a clean
  separation, and it's genuinely shared between Master and Slave where it
  should be (`Buffer`, `SCIDataLink`, `SCICommon`).
- The recent file-header "History" blocks are unusually good documentation
  practice — they explain *why* a change was made and which test caught it
  (e.g. `SCISlaveTransfer.c:11-21`, `SCIMasterTransfer.c:10-27`). Most
  projects don't bother; this makes the codebase's evolution auditable
  without `git blame` archaeology.
- The round-trip GoogleTest suite (pumping Master+Slave state machines in
  lockstep in one process) is a smart way to test a request/response
  protocol without hardware — and it's clearly earning its keep: three real
  bugs (NULL-pointer write, wrong callback wired up, heap corruption on
  multi-chunk upstream) were found and fixed via this suite recently, per
  the file histories.
- The var-struct/command-struct indirection (array of descriptors, addressed
  by index) is a reasonable, low-overhead way to expose device internals
  without implementing a full object/reflection system on an MCU.
- Non-blocking state machines throughout (Master SM, Slave SM, Datalink TX
  SM) rather than blocking loops — correct instinct for embedded, lets the
  caller interleave with other work.

## Concrete bugs (found by reading, not speculation)

1. **`VarAccess.c:412`, `GetVar()`:**
   ```c
   if ((i16VarNum > 0 && i16VarNum))
   ```
   Missing the upper-bound check. Every other accessor in this file
   (`ReadValFromVarStruct`, `WriteValToVarStruct`) correctly checks
   `i16VarNum > 0 && i16VarNum <= SIZE_OF_VAR_STRUCT`. This one doesn't —
   it's tautologically true for any positive number. `GetVar()` backs the
   public API `SCISlaveGetVarFromStruct()` (`SCISlave.h`/`.c`), so any
   application code that calls it with an out-of-range index gets an
   out-of-bounds read of `pVarStruct[]` and silently returns
   `eSCI_SLAVE_ERROR_NONE` with garbage data — no crash, no error, just
   wrong data.

   Good news: the wire protocol itself (GetVar/SetVar requests) doesn't go
   through this function — it uses the correctly-bounded
   `ReadValFromVarStruct`/`WriteValToVarStruct` — so this isn't remotely
   exploitable over the wire today. It's a live landmine for any future
   code that calls the public accessor directly, and reads like a
   copy-paste that lost its second condition.

2. **`eTRANSFER_ACK_REPEAT_REQUEST` is dead-ended.** It's a real enum value
   in the public callback contract (`SCITransferCommon.h:61`) that
   application callbacks are documented to be able to return — but every
   place that checks for it (`SCIMasterTransfer.c`, 3 occurrences) does:
   ```c
   if (eTransferAck != eTRANSFER_ACK_REPEAT_REQUEST)
       ReleaseProtocolCB();
   else
       ; // TODO: Repeat?
   ```
   The repeat path is an empty statement. If a callback ever returns this
   value, the protocol just doesn't release and doesn't repeat — it parks
   in whatever state it was in, since nothing re-arms a request. This is
   API surface that looks implemented but isn't.

3. **No response timeout on the Master** (`SCIMaster.h:16` literally has
   `@todo Response Timeout`). If the Slave never responds — reset,
   brown-out, a corrupted ETX that never arrives — the Master parks in
   `ePROTOCOL_RECEIVING` forever. `SCIInitiateRequest()` refuses any new
   request while `eProtocolState != ePROTOCOL_IDLE`, so the whole interface
   is permanently wedged with no built-in recovery. For a protocol
   explicitly designed to run over a real serial link to embedded devices —
   exactly the conditions where a slave reset or noise is likely — this is
   a significant robustness gap, not a nice-to-have.

## Memory / performance risks

- Both dataframe parsers (`SCIMasterResponseParser`, `SCISlaveRequestParser`)
  `malloc()` a small scratch buffer **per numeric field** they parse — one
  for the address/number, one per comma-separated value. A single response
  with 10 return values does up to 11 malloc/free cycles per message. On a
  long-running embedded target with a small, unmanaged heap this is a real
  risk: fragmentation over time, and worse — **none of these malloc() calls
  check for NULL**. Under memory pressure,
  `memcpy(pui8NumStr, pui8Buf, i)` on a NULL pointer is a hard fault. This
  contradicts the spirit of the README's own disclosure ("dynamic memory
  allocation... MISRA-C compliance concern") — that note reads like it's
  flagging occasional allocations, not a malloc-per-field hot path in the
  message parser that runs on every single transaction.

  **Update (2026-09-11):** Fixed — both parsers' per-value sites (plus the
  Master's "control number after the acknowledge" site) now use a fixed
  `MAX_NUMBER_OF_PARAMETER_DIGITS`-sized stack buffer instead of `malloc()`.
  `MAX_NUMBER_OF_PARAMETER_DIGITS` is a required `SCIconfig.h` parameter
  (compile-time `#error` if missing, see `SCITransferCommon.h`) — not a
  library-derived default. A value longer than the bound is rejected
  outright with a new dedicated error
  (`eSCI_SLAVE_ERROR_REQUEST_VALUE_TOO_LONG` /
  `eSCI_MASTER_ERROR_PARAMETER_TOO_LONG`, both in `SCICommon.h`), not
  truncated — no partial/silently-wrong value is ever accepted. Two things
  remain open: (1) the request/response **ID number** field (a separate,
  smaller site on both Master and Slave, conceptually distinct from a
  parameter *value*) still uses `malloc()` with no NULL check — same latent
  risk category, deliberately out of scope for this fix; (2)
  `SCIMaster.c`'s `SCIMasterSM()` discards `SCIMasterResponseParser()`'s
  `teSCI_MASTER_ERROR` return value entirely, so the new
  `eSCI_MASTER_ERROR_PARAMETER_TOO_LONG` (like every other Master parser
  error already in the codebase) never reaches the application callback in
  a real round-trip — proven here via direct unit tests on the parser
  function instead. Worth a follow-up pass wiring Master parser errors
  through to the requester's callback.

- `SCIMasterTransfer.c` also `malloc()`s the entire COMMAND result buffer
  and the entire upstream buffer per transfer, freed only at the end — at
  least this site is NULL-checked (`if (... == NULL) return false;`), so
  guarding is inconsistent across allocation sites: some checked, some not.
- `Buffer.h`'s `ui8_bufLen`/`ui8_bufSpace` are `uint8_t`, silently capping
  any configured `RX_PACKET_LENGTH`/`TX_PACKET_LENGTH` above 255 (both are
  plain `#define`d ints with no static assertion). Someone bumping packet
  size to, say, 512 for a big upstream transfer gets silent
  truncation/wraparound, not a build error.

## Protocol-level gaps (mostly self-acknowledged, worth surfacing anyway)

- No checksum/CRC (`SCIDataLink.h:13`, `@todo Checksum`, and
  `teDATALINK_ERROR` has an `eDATALINK_ERROR_CHECKSUM` value that's never
  produced anywhere). ASCII decimal/hex encoding gives no error detection
  either — a single corrupted digit that still parses as a valid number is
  accepted and, for a SetVar, written straight into device state. On a real
  UART link this isn't hypothetical.
- No addressing (`@todo Addressable clients`) — can't run multiple slaves on
  a shared bus (RS-485 multidrop) without an out-of-band extension.
- No protocol version negotiation at the wire level — `tsSCI_VERSION`
  exists as a struct but nothing exchanges or validates it, so a Master and
  Slave built against different protocol revisions will just talk past each
  other with whatever silent misinterpretation results.

## Code style / hygiene

- Two overlapping naming conventions coexist in the same codebase from an
  incomplete migration: `Buffer.h/.c` still use `p_inst`, `ui8_data`,
  `b_ovfl` (underscore-Hungarian), while `SCITransferCommon.h` and newer
  files use `psTransfer`, `i16Num` (no separating underscore). The
  transition is visible mid-file in `SCITransferCommon.h`, where large
  commented-out blocks preserve the *old* struct shape (`i16_num`,
  `e_cmdType`) next to the *new* one. Worth finishing that migration rather
  than leaving both alive.
- A lot of dead, commented-out code left in place: the entire `hexToStr()`
  function in `Helpers.c` (lines 190-235), large struct/enum blocks in
  `SCISlaveTransfer.h` and `SCITransferCommon.h`. Fine as scratch during a
  refactor, but it accumulates and makes it harder to tell current design
  from abandoned design on a skim.
- `goto terminate` used as a single-exit pattern in several functions
  (`VarAccess.c`, `SCISlaveTransfer.c`) — a legitimate, common embedded-C
  idiom for centralizing cleanup/return, not a smell by itself, just
  flagging it as a deliberate style choice worth being applied consistently
  (it isn't used everywhere a similar early-return-with-cleanup shape
  occurs).
- Error handling is frequently a `// TODO: What to do on error?` next to a
  no-op `;` (`SCIMaster.c:181-183`, `SCIMasterTransfer.c` multiple spots).
  These aren't bugs today (the paths may be unreachable in current tests)
  but they're explicit admissions that failure modes aren't designed yet,
  not just unimplemented — worth tracking as a punch list rather than
  scattered TODOs.

## Python driver (`Python/SCI.py`)

- `_encode()` in HEX mode does
  `struct.pack('>L', n).hex().upper().lstrip('0')` for both the number and
  every data value. `lstrip('0')` on a zero value produces an **empty
  string**, not `"0"`. For a GetVar/SetVar/Command with number `0`, or any
  data value of `0`, the encoder emits nothing where a digit belongs. On
  the C side this "accidentally" still works only because `strToHex()`
  treats an empty string as valid-and-equal-to-zero (`Helpers.c:161-166`) —
  that's incidental parser leniency, not a designed contract, and it's
  fragile: if anyone ever "fixes" `strToHex` to reject empty input (the
  more defensible behavior), every zero-valued field breaks silently
  across the wire. This should be an explicit `if not num: num = "0"` in
  the Python encoder rather than relying on the other side's leniency.
- `Iterable[float, int]` type hints (multiple places) aren't valid —
  `Iterable` takes one type argument. Harmless at runtime (no enforcement),
  but signals these annotations were never checked with a type checker.
- No automated tests for this module at all — `Python/Tests/*.py` need real
  hardware on `COM3`/`COM27`. The wire-format logic in `_encode`/`_decode`
  (string splitting on `;`/`,`, hex round-tripping) is exactly the kind of
  thing that's cheap to unit test without hardware — a fake
  `serial.Serial` or just direct tests of `_encode`/`_decode` against known
  byte strings would work, and would have caught the zero-value bug above.

## Design limitations (by choice, worth knowing)

- Both `SCIMaster.c` and `SCISlave.c` hold their entire state in a single
  `static` module-global (`sSciMaster`, `sSciSlave`). There's no way to run
  two independent SCI instances in one process/MCU (e.g. bridging two
  serial buses, or a device that's a Slave on one link and a Master on
  another) without duplicating the whole translation unit. The internal
  layers (Datalink, Transfer) already take instance pointers — only the
  public-facing `SCIMaster.h`/`SCISlave.h` API is hardcoded to the
  singleton. If multi-instance is ever a real need, that's a moderate
  refactor (thread the instance pointer through `SCIMasterInit`/
  `SCIRequestGetVar`/etc.) rather than a redesign.
- The union-based type punning for cross-datatype variable access is UB per
  strict ANSI C — but this is explicitly disclosed and accepted in the
  README as a tested tradeoff, so it isn't being relitigated here; just
  confirming it's real and it's where it's documented.

## Priority order for fixes

1. `VarAccess.c:412` bounds-check bug — five-minute fix, and a good example
   of how extracting the bounds-check pattern into a shared helper would
   have prevented a copy-paste-minus-one-condition bug from happening in
   the first place.
2. Response timeout on the Master — the single biggest real-world
   robustness gap for a serial-link protocol; worth designing before this
   goes anywhere near a noisy RS-232/485 line.
3. NULL-checks on the per-field `malloc()`s in both dataframe parsers, or
   better, replace the malloc-per-number pattern with a fixed small stack
   buffer (no heap allocation is needed to null-terminate a 10-digit
   number — a `char buf[12]` on the stack does the same job with zero
   fragmentation risk and no failure mode to handle).
4. Finish the `Buffer.h`/`.c` naming-convention migration so the whole
   codebase reads as one style.
