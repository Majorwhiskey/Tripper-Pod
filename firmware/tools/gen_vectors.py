#!/usr/bin/env python3
"""Generates the shared BLE protocol test vectors.

Writes docs/test-vectors.json (for the Android encoder tests) and
firmware/test/test_vectors/vectors.h (checked by `pio test -e native`).
The bytes are produced by the encoder below, written from docs/protocol.md
independently of the firmware codec, so the firmware test catches either side
drifting from the spec.

    python3 firmware/tools/gen_vectors.py          # regenerate
    python3 firmware/tools/gen_vectors.py --check  # fail if outputs are stale
"""

import json
import sys
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
JSON_OUT = ROOT / "docs" / "test-vectors.json"
HEADER_OUT = ROOT / "firmware" / "test" / "test_vectors" / "vectors.h"

TYPES = {"NAV_TEXT": 0x01, "NAV_ICON": 0x02, "NAV_END": 0x03, "CALL": 0x10, "STATUS": 0x20}
MAX_INSTR, MAX_CALLER = 96, 48
UNK32, UNK16, UNK8 = 0xFFFFFFFF, 0xFFFF, 0xFF

# Right-turn arrow, same bitmap as docs/bench-test.md (64x64, 1 bpp, MSB = left).
ARROW = bytes.fromhex(
    "0000000000000000000000000000000000000000000000000000000000000000"
    "0000000000000000000000000000000000000000000000000000000000000000"
    "0000000000000000000000000000000000000000000000000000000000020000"
    "00000000000300000000000000038000000000000003C000000000000003E000"
    "000000000003F000000000000003F800000000000003FC00000000000003FE00"
    "0003FFFFFFFFFF000003FFFFFFFFFF800003FFFFFFFFFFC00003FFFFFFFFFFE0"
    "0003FFFFFFFFFFF00003FFFFFFFFFFF80003FFFFFFFFFFFC0003FFFFFFFFFFF8"
    "0003FFFFFFFFFFF00003FFFFFFFFFFE00003FFFFFFFFFFC00003FFFFFFFFFF80"
    "0003FFC00003FF000003FFC00003FE000003FFC00003FC000003FFC00003F800"
    "0003FFC00003F0000003FFC00003E0000003FFC00003C0000003FFC000038000"
    "0003FFC0000300000003FFC0000200000003FFC0000000000003FFC000000000"
    "0003FFC0000000000003FFC0000000000003FFC0000000000003FFC000000000"
    "0003FFC0000000000003FFC0000000000003FFC0000000000003FFC000000000"
    "0003FFC0000000000003FFC0000000000003FFC0000000000003FFC000000000"
    "0003FFC0000000000003FFC0000000000003FFC0000000000003FFC000000000"
    "0000000000000000000000000000000000000000000000000000000000000000"
)
assert len(ARROW) == 512

NOON_IST = int(datetime(2026, 11, 26, 6, 30, tzinfo=timezone.utc).timestamp())


def truncate_utf8(s, limit):
    b = s.encode("utf-8")
    if len(b) <= limit:
        return b
    cut = limit
    while cut > 0 and (b[cut] & 0xC0) == 0x80:
        cut -= 1
    return b[:cut]


def frame(type_name, payload, ver=1):
    return bytes([ver, TYPES[type_name]]) + len(payload).to_bytes(2, "little") + payload


def encode(type_name, f):
    le = lambda v, n, signed=False: v.to_bytes(n, "little", signed=signed)
    if type_name == "NAV_TEXT":
        p = (bytes([f["icon_seq"]]) + le(f["dist_m"], 4) + le(f["eta_min"], 2)
             + le(f["remaining_m"], 4) + truncate_utf8(f["instr"], MAX_INSTR))
    elif type_name == "NAV_ICON":
        p = bytes([f["icon_seq"], f["chunk_idx"], f["chunk_cnt"]]) + bytes.fromhex(f["data"])
    elif type_name == "NAV_END":
        p = b""
    elif type_name == "CALL":
        p = truncate_utf8(f["caller"], MAX_CALLER)
    else:
        p = bytes([f["phone_batt_pct"]]) + le(f["unix_time"], 4) + le(f["utc_offset_min"], 2, True)
    return frame(type_name, p)


def nav_text(seq, dist, eta, rem, instr):
    return {"icon_seq": seq, "dist_m": dist, "eta_min": eta, "remaining_m": rem, "instr": instr}


def status(batt, t, off):
    return {"phone_batt_pct": batt, "unix_time": t, "utc_offset_min": off}


# (name, type, fields, description). The phone must turn `fields` into exactly these bytes.
ENCODE = [
    ("status_ist", "STATUS", status(80, NOON_IST, 330), "12:00 IST, phone at 80 %"),
    ("status_negative_offset", "STATUS", status(5, NOON_IST, -210),
     "UTC-03:30; utc_offset_min is signed two's complement"),
    ("status_unknown", "STATUS", status(UNK8, 0, 330), "battery and time not available"),
    ("nav_text_basic", "NAV_TEXT", nav_text(1, 300, 12, 4200, "Turn left onto MG Road"), ""),
    ("nav_text_unknown_numbers", "NAV_TEXT", nav_text(2, UNK32, UNK16, UNK32, "Head north"),
     "instruction parsed, distance/ETA/remaining not"),
    ("nav_text_no_data", "NAV_TEXT", nav_text(0, UNK32, UNK16, UNK32, ""),
     "nothing parsed; pod shows its no-data screen"),
    ("nav_text_kannada", "NAV_TEXT", nav_text(3, 1200, 25, 18500, "ಬಲಕ್ಕೆ ತಿರುಗಿ"),
     "multi-byte UTF-8 instruction"),
    ("nav_text_max_len", "NAV_TEXT", nav_text(4, 50, 1, 50, "Continue onto " + "x" * 82),
     "instruction exactly 96 bytes, sent whole"),
    ("nav_text_truncated_utf8", "NAV_TEXT", nav_text(5, 800, 3, 900, "x" * 94 + "ಕ"),
     "97-byte instruction: cut to 94 bytes, never inside the 3-byte character"),
    ("nav_end", "NAV_END", {}, ""),
    ("call_ascii", "CALL", {"caller": "Amma"}, ""),
    ("call_kannada", "CALL", {"caller": "ಅಮ್ಮ"}, "multi-byte UTF-8 caller"),
    ("call_truncated_utf8", "CALL", {"caller": "y" * 47 + "é"},
     "49-byte caller: cut to 47 bytes, never inside the 2-byte character"),
] + [
    (f"nav_icon_arrow_chunk{i}", "NAV_ICON",
     {"icon_seq": 1, "chunk_idx": i, "chunk_cnt": 4, "data": ARROW[i * 128:(i + 1) * 128].hex().upper()},
     f"right-turn arrow, chunk {i} of 4")
    for i in range(4)
]

# (name, hex, expected decode result, type, expected fields or None, description).
# Bytes the phone must never send; they pin down how the pod reacts.
DECODE = [
    ("too_short", "010300", "TooShort", None, None, "fewer than 4 header bytes"),
    ("len_claims_more", "01010F000102", "LengthMismatch", None, None, "len 15, 2 bytes follow"),
    ("len_claims_less", "01030000AA", "LengthMismatch", None, None, "len 0, 1 byte follows"),
    ("bad_version", "02030000", "UnsupportedVersion", None, None, "ver 2: ignore, keep connection"),
    ("unknown_type", "0199010000", "UnknownType", None, None, "type 0x99: ignore, keep connection"),
    ("nav_text_short_payload", "010105000102030405", "BadPayload", None, None,
     "5 bytes, fixed fields need 11"),
    ("status_short_payload", "01200600320000000000", "BadPayload", None, None,
     "6 bytes, fixed fields need 7"),
    ("nav_icon_bad_count", frame("NAV_ICON", bytes([1, 0, 3]) + bytes(128)).hex().upper(),
     "BadPayload", None, None, "chunk_cnt 3"),
    ("nav_icon_bad_index", frame("NAV_ICON", bytes([1, 4, 4]) + bytes(128)).hex().upper(),
     "BadPayload", None, None, "chunk_idx 4"),
    ("nav_icon_short_data", frame("NAV_ICON", bytes([1, 0, 4]) + bytes(100)).hex().upper(),
     "BadPayload", None, None, "100 data bytes instead of 128"),
    ("status_trailing_bytes", "012008005001000000" "4A01EE", "Ok", "STATUS", status(80, 1, 330),
     "extra trailing byte ignored, so v1 can grow compatibly"),
    ("nav_text_overlong", frame("NAV_TEXT", bytes(11) + b"a" * 150).hex().upper(), "Ok", "NAV_TEXT",
     nav_text(0, 0, 0, 0, "a" * 96), "150-byte instruction truncated by the pod to 96"),
    ("call_invalid_utf8", frame("CALL", b"Am\xffma").hex().upper(), "Ok", "CALL",
     {"caller": "Am?ma"}, "invalid byte replaced with '?'"),
]


def build():
    vectors = []
    for name, t, fields, desc in ENCODE:
        vectors.append({"name": name, "kind": "encode", "type": t, "description": desc,
                        "fields": fields, "hex": encode(t, fields).hex().upper(), "result": "Ok"})
    for name, hx, result, t, fields, desc in DECODE:
        v = {"name": name, "kind": "decode", "type": t, "description": desc,
             "hex": hx, "result": result}
        if fields is not None:
            v["fields"] = fields
        vectors.append(v)
    return {
        "protocol": "docs/protocol.md, v1",
        "generated_by": "firmware/tools/gen_vectors.py; do not edit by hand",
        "notes": [
            "kind=encode: the phone must turn `fields` into exactly `hex`. "
            "Strings in `fields` are what the phone parsed, before truncation.",
            "kind=decode: bytes the phone must never send; `result` is the pod's "
            "decoder outcome and `fields` what it extracts when result is Ok.",
            "Integers are decimal; unknown values use the all-ones sentinels "
            "(4294967295, 65535, 255). NAV_ICON `data` is 128 bytes as hex.",
        ],
        "vectors": vectors,
    }


def c_str(s):
    out = []
    for b in s.encode("utf-8"):
        if 0x20 <= b < 0x7F and chr(b) not in '"\\?':
            out.append(chr(b))
        else:
            out.append(f"\\{b:03o}")
    return '"' + "".join(out) + '"'


def c_bytes(hx):
    b = bytes.fromhex(hx)
    return "{" + ", ".join(f"0x{x:02X}" for x in b) + "}" if b else "{0}"


def wire_text(t, f):
    if t == "NAV_TEXT":
        return truncate_utf8(f["instr"], MAX_INSTR).decode("utf-8")
    if t == "CALL":
        return truncate_utf8(f["caller"], MAX_CALLER).decode("utf-8")
    return ""


def header(doc):
    lines = [
        "// Generated by firmware/tools/gen_vectors.py from the same data as",
        "// docs/test-vectors.json. Do not edit by hand.",
        "#pragma once", "", "#include <stddef.h>", "#include <stdint.h>", "",
        '#include "proto.h"', "", "struct Vector {", "  const char* name;",
        "  bool encode;  // the firmware encoder must reproduce `bytes` from the fields",
        "  proto::Result result;", "  proto::Type type;  // meaningful only when result is Ok",
        "  const uint8_t* bytes;", "  size_t len;", "  uint8_t icon_seq;", "  uint32_t dist_m;",
        "  uint16_t eta_min;", "  uint32_t remaining_m;",
        "  const char* text;  // instr or caller as sent on the wire",
        "  uint8_t chunk_idx;", "  uint8_t chunk_cnt;", "  const uint8_t* icon_data;",
        "  uint8_t phone_batt_pct;", "  uint32_t unix_time;", "  int16_t utc_offset_min;", "};", "",
    ]
    rows = []
    for i, v in enumerate(doc["vectors"]):
        f = v.get("fields", {})
        t = v["type"] or "NAV_END"
        lines.append(f"static const uint8_t kBytes{i}[] = {c_bytes(v['hex'])};")
        icon = "nullptr"
        if t == "NAV_ICON" and "data" in f:
            lines.append(f"static const uint8_t kIcon{i}[] = {c_bytes(f['data'])};")
            icon = f"kIcon{i}"
        enum = {"NAV_TEXT": "NavText", "NAV_ICON": "NavIcon", "NAV_END": "NavEnd",
                "CALL": "Call", "STATUS": "Status"}[t]
        rows.append(
            f"    {{\"{v['name']}\", {'true' if v['kind'] == 'encode' else 'false'}, "
            f"proto::Result::{v['result']}, proto::Type::{enum}, kBytes{i}, "
            f"{len(bytes.fromhex(v['hex']))}, {f.get('icon_seq', 0)}, {f.get('dist_m', 0)}u, "
            f"{f.get('eta_min', 0)}, {f.get('remaining_m', 0)}u, {c_str(wire_text(t, f))}, "
            f"{f.get('chunk_idx', 0)}, {f.get('chunk_cnt', 0)}, {icon}, "
            f"{f.get('phone_batt_pct', 0)}, {f.get('unix_time', 0)}u, {f.get('utc_offset_min', 0)}}},"
        )
    lines += ["", "static const Vector kVectors[] = {"] + rows + ["};", ""]
    return "\n".join(lines)


def main():
    doc = build()
    outputs = {JSON_OUT: json.dumps(doc, ensure_ascii=False, indent=2) + "\n",
               HEADER_OUT: header(doc)}
    if "--check" in sys.argv:
        stale = [p for p, s in outputs.items() if not p.exists() or p.read_text("utf-8") != s]
        for p in stale:
            print(f"stale: {p.relative_to(ROOT)}; run firmware/tools/gen_vectors.py")
        return 1 if stale else 0
    for p, s in outputs.items():
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(s, "utf-8")
        print(f"wrote {p.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
