# Bill of materials — draft

Status: **draft, nothing ordered.** Part numbers marked *verify* are from memory and
must be checked against the manufacturer datasheet before ordering. Prices are left
blank; fill from the supplier you buy from (Robu.in, Evelta, Mouser India, DigiKey).

## Power stage

```
Bike ignition-switched 12 V
  → 1 A blade fuse (as close to the tap point as possible)
  → reverse-polarity diode
  → TVS to ground (load dump)
  → bulk capacitor
  → buck 12 V → 5 V
  → ESP32-S3 dev board 5 V pin (its own regulator makes 3.3 V)
```

Power budget: ESP32-S3 with BLE ≈ 100 mA peak, display + backlight ≈ 40 mA, total
< 300 mA at 5 V, so < 150 mA from the bike. The 1 A fuse protects the wiring, not
the electronics.

Every part downstream of the TVS must tolerate its clamp voltage (≈ 39 V for the
SM8S24A at rated current). That is why the buck below is rated to 50 V input, not
the common 36 V.

| # | Part | Proposed part | Qty | Why / notes |
|---|---|---|---|---|
| P1 | Inline fuse holder, waterproof | Mini blade (ATM) holder with cap, 18–20 AWG leads | 1 | Sealed type for monsoon spray |
| P2 | Fuse | Littelfuse 0297001 mini blade 1 A *verify* | 3 | 2 spares |
| P3 | Reverse-polarity diode | SS34 Schottky, 40 V 3 A | 1 | ~0.4 V drop is irrelevant at 12 V |
| P4 | Load-dump TVS | Vishay/Littelfuse SM8S24A *verify* | 1 | Standard part for 12 V vehicle load dump; survives a 24 V jump start |
| P5 | Bulk capacitor | 47 µF 63 V electrolytic, 105 °C | 1 | Rides through dips during electric start |
| P6 | Buck converter | Pololu D36V6F5 (5 V 600 mA, ~4.5–50 V in) *verify* | 1 | Input range covers cranking dips and TVS clamp; small enough for the pod |
| P7 | Decoupling | 100 nF 50 V ceramic | 2 | Buck input and output |

## Electronics

| # | Part | Proposed part | Qty | Why / notes |
|---|---|---|---|---|
| E1 | MCU board | Espressif ESP32-S3-DevKitC-1-N8R8 | 1 (+1 spare) | Matches firmware target (D9); 8 MB PSRAM holds the framebuffer |
| E2 | Display | 1.28″ GC9A01 240×240 round IPS module | 2 | One for the sunlight test, one spare; buy a 2.1″ only after D3 |
| E3 | Divider resistors | 100 kΩ + 22 kΩ, 1 %, 0805 or through-hole | 2 sets | Bike voltage sense, step 7 |
| E4 | ADC clamp | BAT54S dual Schottky *verify* | 1 | Clamps the ADC pin to the 3.3 V rail |
| E5 | ADC filter cap | 100 nF ceramic | 1 | Across the 22 kΩ |
| E6 | Temperature sensor (optional) | DS18B20, waterproof probe version | 1 | Step 7, optional; mount outside the pod, away from the board's heat |
| E7 | Perfboard or protoboard | ~5 × 7 cm | 2 | Power stage + divider until a PCB is worth making |

## Wiring and connectors

| # | Part | Proposed part | Qty | Why / notes |
|---|---|---|---|---|
| W1 | Automotive wire | 20 AWG (0.5 mm²) TXL/GXL or FLRY, red + black | 3 m each | Thin-wall, oil and heat resistant |
| W2 | Locking waterproof connector | Superseal 1.0 2-pin, male + female | 2 pairs | Lets the pod come off the bike; locking for vibration |
| W3 | Ring terminal | M6, 20 AWG, insulated | 2 | Ground to frame or battery negative |
| W4 | Tap connector | Add-a-circuit or solder + heatshrink at the ignition-switched wire | 1 | Tap point not chosen yet: find a wire that is 12 V only with ignition on, using a multimeter. Do not cut into the ECU harness. |
| W5 | Adhesive-lined heatshrink | 3:1, assorted | 1 pack | Seals every joint |
| W6 | Cable gland | PG7, IP68, nylon | 2 | Cable entry into the pod |
| W7 | Spiral wrap / P-clips | for handlebar run | 1 | Keeps the cable clear of steering lock and the throttle |

## Enclosure and mount

| # | Part | Proposed part | Qty | Why / notes |
|---|---|---|---|---|
| M1 | Pod housing | 3D printed in ASA (not PLA or PETG) | 1 | Round tripper shape; ASA survives sun and heat; PLA softens in a parked bike's sun |
| M2 | Gasket / O-ring | Nitrile or silicone O-ring sized to the lid groove | 3 | IP65 seal; spares because they compress |
| M3 | Cover glass | Anti-glare (AG-etched) glass, round, sized to display | 2 | Plain glass is unreadable in sun; cracks happen |
| M4 | Optical bonding (optional) | Clear double-sided OCA film | 1 | Removes the air gap that causes reflections; try after the sunlight test |
| M5 | Conformal coating | MG Chemicals 422C silicone *verify* | 1 can | Salt air and condensation; mask the display connector |
| M6 | Desiccant | Small silica gel sachet | 3 | Absorbs condensation inside the sealed pod |
| M7 | Breather vent (optional) | Adhesive ePTFE vent | 1 | Stops pressure changes pumping moist air past the gasket |
| M8 | Handlebar clamp | Clamp for 22 mm (7/8″) bar *verify Hunter 350 bar diameter* | 1 | Depends on D4 (handlebar vs cluster bracket) |
| M9 | Fasteners | Stainless M3 screws + nylock nuts, threadlocker (medium) | 1 set | Ghat-road vibration loosens plain nuts |
| M10 | Vibration isolation | Small rubber grommets / foam pad for the board | 1 set | Protects solder joints and the display flex cable |

## Test equipment (if not already owned)

| Item | Used for |
|---|---|
| Multimeter | Finding the ignition tap, calibrating the voltage divider (step 7) |
| USB-C data cable | Flashing and serial logs |
| Garden hose | Step 6 water ingress test |

## Before ordering

1. Decide D4 (mount): changes M1 and M8.
2. Order E1 + E2 first and run `docs/bench-test.md` and the sunlight test; that
   settles D3 (display size) before the enclosure is designed around it.
3. Check every *verify* part against its datasheet.
