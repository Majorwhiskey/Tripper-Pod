# Decision log

One entry per decision. Status: open | decided | superseded.

| # | Topic | Status | Decision | Date |
|---|---|---|---|---|
| D1 | Display font (SF Mono licence vs free monospace) | decided | JetBrains Mono (OFL); revisit only if SF Mono licence is cleared | 2026-11-26 |
| D2 | Graphics library: LovyanGFX vs TFT_eSPI | decided | LovyanGFX | 2026-11-26 |
| D3 | Display size: 1.28″ vs 2.1″ | open (after sunlight test) | — | — |
| D4 | Mount: handlebar clamp vs cluster bracket | open | — | — |
| D5 | Protocol: icon chunk size, icon/text pairing, unknown-value sentinels | decided | 4 × 128 B chunks; `icon_seq:u8` on NAV_TEXT and NAV_ICON; all-ones = unknown; incomplete icon discarded | 2026-11-26 |
| D6 | BLE UUIDs for protocol v1 | decided | recorded in `protocol.md` | 2026-11-26 |
| D7 | Toolchain: Gradle wrapper, AGP, Kotlin plugin, AndroidX core; PlatformIO + Unity | decided | approved by owner | 2026-11-26 |
| D8 | BLE roles | decided | pod = GATT server; phone writes nav packets, pod notifies acks | 2026-11-26 |
| D9 | MCU board | provisional | ESP32-S3-DevKitC-1 N8R8 until the real board arrives; pins in `firmware/src/config.h` | 2026-11-26 |
| D10 | Local time | decided | phone sends `utc_offset_min:i16` in STATUS | 2026-11-26 |
