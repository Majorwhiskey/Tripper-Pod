# Notification fixtures

Real Google Maps navigation notifications captured by the step-1 listener.
These are the ground truth for parser tests; never hand-edit a captured value.

One JSON file per capture: `<maps-version>_<locale>_<nnn>.json`

```json
{
  "captured_at": "2026-11-26T12:00:00+05:30",
  "maps_version": "",
  "android_version": "",
  "locale": "en_IN",
  "extras": {
    "android.title": "",
    "android.text": "",
    "android.subText": ""
  },
  "large_icon_png": "<file name of icon saved alongside, or null>",
  "ground_truth": { "dist_m": 0, "instruction": "", "eta_min": 0 }
}
```

`ground_truth` is filled by hand from what the screen showed at capture time.
Strip anything personal (home address, contact names) before committing.
