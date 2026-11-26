# Hunter Tripper — DIY turn-by-turn pod for a Royal Enfield Hunter 350

A handlebar display that shows Google Maps turn-by-turn directions plus ride info.
The phone does the navigation; this project only mirrors it.

```
Android phone (Google Maps navigating)
   └─ Companion app: NotificationListenerService parses the Maps nav notification
        └─ BLE (custom GATT service on the pod, phone writes)
             └─ ESP32-S3 + round TFT on the handlebar
                  └─ powered from ignition-switched 12 V via fused, protected 5 V buck
```

Google offers no public API for live turn-by-turn data. Everything here depends on
parsing the Maps navigation notification, whose layout is **not a contract** and can
change with any Maps update. Parsing must degrade gracefully (text-only if the icon
fails, "no data" screen if text fails) and never crash.

Platform: **Android only.** iOS does not expose Maps nav notification content.

---

## Repo layout

```
android/      Kotlin companion app (Gradle)
firmware/     ESP32-S3 firmware (PlatformIO)
  src/        application code
  lib/proto/  packet encode/decode, shared logic, host-testable
  test/       Unity tests, run on host via the `native` env
hardware/     schematic, BOM, enclosure drawings
docs/         protocol spec, captured notification fixtures, test logs
```

## Proposed stack (dependencies not yet approved — ask before adding any)

| Part | Proposal | Why |
|---|---|---|
| MCU | ESP32-S3 (8 MB PSRAM variant) | BLE built in, RAM for a 240×240 framebuffer |
| Display | 1.28″ GC9A01 240×240 round IPS | cheap, fits a tripper-style pod; sunlight test pending |
| Firmware | PlatformIO, Arduino framework | fastest start; revisit ESP-IDF if BLE gets limiting |
| Graphics | LovyanGFX (decided, D2) | no LVGL until a real need appears |
| BLE (fw) | NimBLE-Arduino | lighter than Bluedroid |
| Android | Kotlin, minSdk 26, no extra libs beyond AndroidX | standard library first |

## BLE protocol (v1 draft)

Pod is the GATT server and advertises; phone is the client. One write-without-response
characteristic phone → pod, one notify characteristic pod → phone (acks, later).
Negotiate MTU 247. UUIDs: generate once, record in `docs/protocol.md`, never change
within a protocol version. `docs/protocol.md` is the authoritative spec.

Every packet: `[ver:u8][type:u8][len:u16 LE][payload]`

| type | name | payload |
|---|---|---|
| 0x01 | NAV_TEXT | icon_seq:u8, dist_m:u32, eta_min:u16, remaining_m:u32, instr: UTF-8 ≤ 96 B |
| 0x02 | NAV_ICON | icon_seq:u8, chunk_idx:u8, chunk_cnt:u8, data: 128 B (4 chunks = 64×64 1-bpp) |
| 0x03 | NAV_END | — |
| 0x10 | CALL | caller: UTF-8 ≤ 48 B |
| 0x20 | STATUS | phone_batt_pct:u8, unix_time:u32, utc_offset_min:i16 |

Rules: phone sends only on change; icon is resent only when its hash changes;
pod treats unknown `ver`/`type` as ignore-and-continue; length is always checked
before reading payload.

## Notification parsing (Android)

- Filter: package `com.google.android.apps.maps`, ongoing notification.
- Candidate fields: `android.title`, `android.text`, `android.subText`, large icon.
  Which field holds distance vs instruction vs ETA is **unverified** — capture real
  notifications first (step 1 below) and save them as fixtures in `docs/fixtures/`.
- Distance strings arrive localised ("300 m", "1.2 km"); parse to metres.
- Icon: downscale to 64×64, threshold to 1-bpp.

## Hardware notes

- Power: ignition-switched 12 V → 1 A inline fuse → TVS rated for load dump →
  automotive-rated buck → 5 V. Pod must be fully off with ignition off.
- Bike voltage sense: 100 kΩ / 22 kΩ divider into an ADC pin (16 V → 2.89 V),
  with a clamp. Calibrate against a multimeter; report in volts with one decimal.
- Enclosure: IP65 minimum, gasketed lid, anti-glare cover glass, conformal-coated
  board, locking connectors. Target environment includes monsoon rain, coastal salt
  air and ghat-road vibration.
- Ambient temperature sensor optional; display in °C (font must include the ° glyph).

## Build order

Each step: `step → verify: check`. A step is done only when the check has been run
and its output is recorded in `docs/test-log.md`.

1. Android listener that logs parsed Maps fields → verify: on a real 10-minute drive,
   logcat shows correct distance + instruction at every turn; fixtures saved.
2. Parser unit tests from those fixtures → verify: JUnit passes; a test fails if
   parsing is broken.
3. `lib/proto` encode/decode with host tests → verify: `pio test -e native` passes,
   including malformed/short/unknown-type packets.
4. Firmware BLE receive, printing packets to serial → verify: turn-change latency
   ≤ 200 ms measured on serial timestamps.
5. Display rendering (arrow, distance, street, clock screen when idle) → verify:
   readable at arm's length in direct midday sun.
6. Power stage and enclosure → verify: 20 ignition on/off cycles without reset or
   brownout; hose test with no water ingress.
7. Extras, one at a time: call alert, phone battery, bike voltage, temperature.

## Open decisions

- Decided items live in `docs/decisions.md` (font: JetBrains Mono, D1; LovyanGFX, D2;
  pod is GATT server, D8).
- Display size (1.28″ vs 2.1″) after the sunlight test.
- Mount: clamp to handlebar vs bracket on the instrument cluster.

## Working rules

- No new file over 300 lines; split into modules.
- Ask before deleting any file, adding any dependency, or running destructive commands.
- Never commit or push unless asked. No force-push. Commit subjects: imperative, ≤ 72
  chars, no emoji, no `Co-Authored-By` or other trailers.
- Done = tests run and passing output shown. Never weaken an assertion, skip a test or
  widen a tolerance to get a pass. New behaviour needs a test that fails without it.
- No secrets in code or logs; never read or commit `.env`.
- Comments explain a non-obvious why, never narrate what. No emoji in code. Flag any
  TODO/FIXME left behind.
- Use real symbols in docs: ° µs × ≤ ≥ → Ω. Code identifiers stay ASCII.
- After ~3 failed attempts on one approach, stop and report.
