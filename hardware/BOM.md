# Bill of materials — draft

Status: **draft, nothing ordered.** Part numbers were checked against datasheets and
distributor listings on 2026-11-26 (sources at the end); anything still marked
*verify* could not be confirmed. Prices are left
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
| P2 | Fuse | Eaton Bussmann ATM-1 (MINI blade, 1 A, 32 V) | 3 | 2 spares. Littelfuse's MINI 297 series starts at 2 A (0297002.WXNV, also acceptable for this load); the earlier "0297001" does not exist |
| P3 | Reverse-polarity diode | onsemi SS36 Schottky, 60 V 3 A, SMC (DO-214AB) | 1 | V<sub>F</sub> ≤ 0.75 V at 3 A, well under that at the ~150 mA load: irrelevant at 12 V. 60 V V<sub>RRM</sub> chosen over the 40 V SS34 for headroom against negative transients (ISO 7637-2 pulse 1). Second-source SS36s differ in V<sub>F</sub> and T<sub>J</sub> max: check the datasheet of the one bought |
| P4 | Load-dump TVS | Vishay SM8S24AHM3/I (unidirectional, AEC-Q101) | 1 | V<sub>WM</sub> 24 V, V<sub>BR</sub> ≥ 26.7 V, V<sub>C</sub> ≤ 38.9 V at 170 A, 6.6 kW (10/1000 µs). A 24 V jump start stays below V<sub>BR</sub> (2.7 V margin). Current datasheet gives the case as DO-218AC, older ones DO-218AB: check the footprint. Not the bidirectional SM8S24CA; P3 already blocks reverse battery |
| P5 | Bulk capacitor | 47 µF 63 V electrolytic, 105 °C | 1 | Rides through dips during electric start |
| P6 | Buck converter | Pololu D36V6F5 (#3792): 5 V ±4 %, 600 mA, 5.2–50 V in, I<sub>q</sub> < 0.1 mA | 1 | 50 V max leaves ~11 V over the TVS clamp; 13 × 10 × 3 mm. No reverse-voltage protection (P3 covers it). Pololu asks for ≥ 33 µF electrolytic at VIN: P5 satisfies it |
| P7 | Decoupling | 100 nF 50 V ceramic | 2 | Buck input and output |

## Electronics

| # | Part | Proposed part | Qty | Why / notes |
|---|---|---|---|---|
| E1 | MCU board | Espressif ESP32-S3-DevKitC-1-N8R8 | 1 (+1 spare) | Matches firmware target (D9); 8 MB PSRAM holds the framebuffer |
| E2 | Display | 1.28″ GC9A01 240×240 round IPS module | 2 | One for the sunlight test, one spare; buy a 2.1″ only after D3 |
| E3 | Divider resistors | 100 kΩ + 22 kΩ, 1 %, 0805 or through-hole | 2 sets | Bike voltage sense, step 7 |
| E4 | ADC clamp | BAT54S dual series Schottky, SOT-23 (Diodes BAT54S-7-F or Nexperia BAT54S,215) | 1 | 30 V, 200 mA, I<sub>R</sub> ≤ 2 µA at 25 V. Pin 1 → GND, pin 2 → 3V3, pin 3 → ADC node (standard pinout; *verify* against the datasheet drawing). Leakage rises with temperature and adds error on the 100 kΩ divider: calibrate warm |
| E5 | ADC filter cap | 100 nF ceramic | 1 | Across the 22 kΩ |
| E8 | ADC series resistor | 1 kΩ–10 kΩ | 1 | Between divider tap and the clamp/ADC pin; limits clamp current |
| E6 | Temperature sensor (optional) | DS18B20, waterproof probe version | 1 | Step 7, optional; mount outside the pod, away from the board's heat |
| E7 | Perfboard or protoboard | ~5 × 7 cm | 2 | Power stage + divider until a PCB is worth making |

## Wiring and connectors

| # | Part | Proposed part | Qty | Why / notes |
|---|---|---|---|---|
| W1 | Automotive wire | 20 AWG (0.5 mm²) TXL/GXL or FLRY, red + black | 3 m each | Thin-wall, oil and heat resistant |
| W2 | Locking waterproof connector | TE AMP Superseal 1.5 2-pos: housing 282104-1 (tab side) + 282080-1 (receptacle side) | 2 pairs | IP67 / IPX9K, locking for vibration. TE lists both housings as "not currently available": check distributor stock |
| W2a | Superseal terminals, 20 AWG | Tab 282404-1, receptacle 282403-1 (22–20 AWG, 1.2–1.6 mm insulation) | 4 + 4 | Not 282109-1/282110-1, which are for 0.75–1.5 mm² wire |
| W2b | Superseal wire seal | 281934-4 (green, 1.2–1.6 mm insulation) | 8 | Match to the actual wire's insulation OD |
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
| M5 | Conformal coating | MG Chemicals 422C-340G aerosol (acrylic-silicone) | 1 can | Salt air and condensation; mask the display connector. UV-traceable, solderable through, removable with MG 8309/8310A; −40–200 °C; full cure 24 h at room temperature |
| M6 | Desiccant | Small silica gel sachet | 3 | Absorbs condensation inside the sealed pod |
| M7 | Breather vent (optional) | Adhesive ePTFE vent | 1 | Stops pressure changes pumping moist air past the gasket |
| M8 | Handlebar clamp | Clamp for 22 mm (7/8″) bar | 1 | Hunter 350 bar is 22.2 mm per Hepco & Becker fitment and accessory sellers (no RE spec sheet found; measure before ordering). Depends on D4 (handlebar vs cluster bracket) |
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
3. Resolve the remaining *verify* note (E4 pinout) and measure the
   handlebar.

## Sources (checked 2026-11-26)

- P2: Eaton Bussmann ATM-1, https://www.digikey.co.uk/en/products/detail/eaton-bussmann-electrical-division/ATM-1/24768397; Littelfuse 297 series, https://littelfuse.com/products/fuses/automotive-passenger-car/blade-fuses/297.aspx
- P3: onsemi SS36, https://www.onsemi.com/pdf/datasheet/ssa36-d.pdf, https://www.digikey.com/en/products/detail/onsemi/SS36/1053019
- P4: Vishay datasheet 98647 (rev 16-Jan-2024), https://www.vishay.com/docs/98647/sm8s10ahm3thrusm8s43ahm3.pdf
- P6: https://www.pololu.com/product/3792
- E4: Diodes Inc DS11005, https://www.diodes.com/assets/Datasheets/ds11005.pdf
- M5: https://mgchemicals.com/products/conformal-coating/conformal-coating-spray/circuit-board-waterproofing/
- M8: https://www.hepco-becker.de/en/Offset-Handlebar-Riser-for-Royal-Enfield-Hunter-350-2023-30-mm-high-21-mm-back-black/42407635-00-01
- W2: https://www.te.com/en/product-282104-1.html, https://www.te.com/en/product-282080-1.html, https://www.te.com/en/product-282403-1.html, https://www.te.com/en/product-282404-1.html; seal: https://uk.farnell.com/amp-te-connectivity/281934-4/wire-seal-green/dp/150915
