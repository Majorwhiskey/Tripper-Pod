# Bench test — first power-up

For the day the ESP32-S3 board arrives, before the Android app exists. Needs: the
board, a GC9A01 display, jumper wires, a USB-C cable, and the free **nRF Connect**
app on any phone.

## 1. Wire the display

Pins are in `firmware/src/config.h` (ESP32-S3-DevKitC-1 N8R8). If your board differs,
change only that file.

| GC9A01 | ESP32-S3 GPIO |
|---|---|
| VCC | 3V3 |
| GND | GND |
| SCL / SCLK | 12 |
| SDA / MOSI | 11 |
| CS | 10 |
| DC | 9 |
| RST | 14 |
| BL | 13 |

## 2. Flash and watch serial

```
cd firmware
pio run -e esp32s3 -t upload
pio device monitor
```

Expected: `READY hunter-tripper protocol v1`, and the display shows `--:--` with
"No phone" in amber. If the log shows `ERR framebuffer alloc failed`, the board has
no PSRAM or `board_build.arduino.memory_type` is wrong for it.

## 3. Send packets from nRF Connect

1. Scan, connect to **HunterTripper**. Serial prints `CONN …`.
2. Request MTU 247 (menu → Request MTU). Needed for the 135-byte icon packets.
3. Open the service `5211c17e-…`, characteristic `993d9a6c-…`, tap the write
   arrow, choose **Byte Array**, paste a row's hex and send.

Send in this order and check each expectation:

| Packet → expected result | Hex |
|---|---|
| STATUS 80 %, 12:00 IST → clock shows 12:00, "Phone 80%" | `01200700506823096B4A01` |
| NAV_TEXT 300 m, "Turn left onto MG Road" → nav screen, text only | `01012100012C0100000C00681000005475726E206C656674206F6E746F204D4720526F6164` |
| NAV_ICON seq 1, chunk 0 of 4 | `0102830001000400000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000002000000000000000300000000000000038000000000000003C000000000000003E000` |
| NAV_ICON seq 1, chunk 1 of 4 | `01028300010104000000000003F000000000000003F800000000000003FC00000000000003FE000003FFFFFFFFFF000003FFFFFFFFFF800003FFFFFFFFFFC00003FFFFFFFFFFE00003FFFFFFFFFFF00003FFFFFFFFFFF80003FFFFFFFFFFFC0003FFFFFFFFFFF80003FFFFFFFFFFF00003FFFFFFFFFFE00003FFFFFFFFFFC00003FFFFFFFFFF80` |
| NAV_ICON seq 1, chunk 2 of 4 | `010283000102040003FFC00003FF000003FFC00003FE000003FFC00003FC000003FFC00003F8000003FFC00003F0000003FFC00003E0000003FFC00003C0000003FFC0000380000003FFC0000300000003FFC0000200000003FFC0000000000003FFC0000000000003FFC0000000000003FFC0000000000003FFC0000000000003FFC000000000` |
| NAV_ICON seq 1, chunk 3 of 4 → arrow appears | `010283000103040003FFC0000000000003FFC0000000000003FFC0000000000003FFC0000000000003FFC0000000000003FFC0000000000003FFC0000000000003FFC0000000000003FFC0000000000003FFC0000000000003FFC0000000000003FFC0000000000000000000000000000000000000000000000000000000000000000000000000` |
| NAV_TEXT long instruction → wraps, ends with "..."; arrow stays (same icon_seq) | `01014F0001D2040000410020CB0000436F6E74696E7565206F6E746F20546869727576616E616E746861707572616D204B616E79616B756D617269204869676877617920746F7761726473204B6F76616C616D` |
| NAV_TEXT unparsed → "No nav data" | `01010B0000FFFFFFFFFFFFFFFFFFFF` |
| CALL "Amma" → call screen for 15 s | `01100400416D6D61` |
| NAV_END → clock | `01030000` |
| Bad version → serial `bad-version`, screen unchanged | `02030000` |
| Bad length → serial `len-mismatch`, screen unchanged | `01010F000102` |

Every write prints an `RX <µs> …` line; every redraw that follows a packet prints
`DRAW <µs> +<ms>`. The `+ms` figure is decode-to-screen time on the pod.

## 4. Step 4 check (latency ≤ 200 ms)

Needs the Android app. The serial `RX` timestamp marks arrival at the pod; the app
must log its send time for the same packet so the two can be compared over a turn
change. Record both logs in `docs/test-log.md`.

## 5. Step 5 check (readability)

Load the "Turn left onto MG Road" packet plus icon, take the board outside at midday,
and read it at arm's length. Note what was hard to read in `docs/test-log.md`; font
sizes are in `firmware/tools/fontgen.py` invocations (see the test log entry) and
layout positions in `firmware/src/ui.cpp`.
