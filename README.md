# Hunter Tripper

A handlebar pod for a Royal Enfield Hunter 350 that mirrors Google Maps
turn-by-turn directions from an Android phone over BLE onto a round TFT.

```
Android phone (Google Maps) → companion app → BLE → ESP32-S3 + round TFT
```

Status: step 3 done. Firmware for steps 4–5 builds and passes host tests but awaits
the board (`docs/bench-test.md`). Step 1 (Android app) in progress on a separate branch. See `docs/test-log.md` for verified progress.

| Path | Contents |
|---|---|
| `android/` | Kotlin companion app |
| `firmware/` | ESP32-S3 firmware (PlatformIO) |
| `hardware/` | schematic, BOM (`hardware/BOM.md`, draft), enclosure |
| `docs/protocol.md` | BLE protocol v1 spec |
| `docs/decisions.md` | decision log |
| `docs/fixtures/` | captured Maps notifications |
| `docs/test-log.md` | build-step verification records |
| `docs/bench-test.md` | first power-up guide and test packets |

Design rules and build order live in `CLAUDE.md`.

## Limitations

- Android only. iOS does not expose Maps navigation notification content.
- Relies on the Maps notification layout, which Google can change at any time.
  The pod degrades to text-only or a "no data" screen rather than failing.
