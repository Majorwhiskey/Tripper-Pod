# Test log

A build step is done only when its check has been run and the output is recorded here.
Newest entries at the top.

## Template

```
### Step N — <name> — YYYY-MM-DD
Check: <the verify condition from CLAUDE.md>
Setup: <phone model, Android + Maps version, firmware commit, etc.>
Command / procedure: <what was run>
Result: PASS | FAIL
Output:
<pasted output or measurements>
Notes: <anything surprising>
```

## Entries

### Pre-hardware firmware (steps 4 and 5 code) — 2026-11-26
Not a step completion: steps 4 and 5 need the board and are **not verified**.
This records that the code builds and its host-testable logic passes.
Setup: as step 3, plus espressif32 8.0.0, LovyanGFX 1.3.0, NimBLE-Arduino 2.5.2,
target ESP32-S3-DevKitC-1 N8R8 (provisional, D9).
Command / procedure: `pio test -e native`; `pio run -e esp32s3`
Result: PASS (host tests, build). On-device behaviour untested.
Output:
```
native         test_icon       PASSED    00:00:00.318
native         test_pod_state  PASSED    00:00:00.319
native         test_utf8       PASSED    00:00:00.320
native         test_decode     PASSED    00:00:00.305
native         test_text_fit   PASSED    00:00:00.310
native         test_format     PASSED    00:00:00.307
================= 42 test cases: 42 succeeded in 00:00:01.879 =================
RAM:   [=         ]  11.3% (used 37188 bytes from 327680 bytes)
Flash: [==        ]  19.6% (used 653783 bytes from 3342336 bytes)
esp32s3        SUCCESS
```
Build has zero warnings from project code. `test_format` caught a real overflow
(`formatDistance(4294967294)` printed "0 km") before the fix. Bench-test packets in
`docs/bench-test.md` were decoded by the firmware codec on the host: all valid ones
returned Ok, the two bad ones returned UnsupportedVersion and LengthMismatch.

### Step 3 — lib/proto encode/decode with host tests — 2026-11-26
Check: `pio test -e native` passes, including malformed/short/unknown-type packets.
Setup: macOS, PlatformIO Core 6.2.0, Apple clang 21.0.0, Unity 2.6.1,
flags `-std=c++17 -Wall -Wextra -Werror`. Protocol per `docs/protocol.md` (D5).
Command / procedure: `cd firmware && pio test -e native`
Result: PASS
Output:
```
native         test_icon    PASSED    00:00:06.432
native         test_utf8    PASSED    00:00:00.613
native         test_decode  PASSED    00:00:00.621
================= 22 test cases: 22 succeeded in 00:00:07.665 =================
```
Mutation check (tests must fail when code is broken): relaxed the `len` equality
check to `<` and removed the chunk reset on a new `icon_seq`; three tests failed:
```
test_icon.cpp:51: test_new_seq_discards_partial_icon: Expected FALSE Was TRUE [FAILED]
test_icon.cpp:59: test_new_seq_replaces_complete_icon: Expected FALSE Was TRUE [FAILED]
test_decode.cpp:130: test_short_and_mismatched_lengths: Expected 2 Was 0 [FAILED]
```
Code restored; re-run 22/22 passed.
Notes: protocol is a draft until the Android side (separate branch) sends real
packets; re-run on any protocol change.
