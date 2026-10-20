"""Run against a built station executable; no detector or document-folder writes."""
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

exe = Path(sys.argv[1]).resolve()
env = dict(os.environ, QT_QPA_PLATFORM="offscreen", QT_QUICK_BACKEND="software",
           QT_FORCE_STDERR_LOGGING="1")
with tempfile.TemporaryDirectory(prefix="raw-import-", dir=exe.parent) as tmp:
    root = Path(tmp)
    cases = [
        ("gray8", 1, "little-endian", bytes([0, 127, 255, 10, 20, 30]), {}, 0),
        ("gray16_le", 2, "little-endian", struct.pack("<6H", 0, 1, 255, 256, 32768, 65535), {}, 0),
        ("gray16_be", 2, "big-endian", struct.pack(">6H", 0, 1, 255, 256, 32768, 65535), {}, 0),
        ("truncated", 2, "little-endian", b"\0", {}, 4),
        ("fractional", 1, "little-endian", bytes(6), {"width": 3.5}, 4),
        ("oversized", 2, "little-endian", b"", {"width": 20000, "height": 20000}, 4),
        ("bad_endian", 2, "unknown", bytes(12), {}, 4),
        ("missing_source", 1, "little-endian", bytes(6), {"simulation": None}, 4),
        ("missing_json", 1, "little-endian", bytes(6), {}, 4),
        ("bad_json", 1, "little-endian", bytes(6), {}, 4),
    ]
    for name, bpp, order, data, overrides, expected in cases:
        raw = root / (name + ".raw")
        raw.write_bytes(data)
        info = dict(width=3, height=2, bytesPerPixel=bpp, byteOrder=order, simulation=True)
        info.update(overrides)
        if name != "missing_json":
            raw.with_suffix(".json").write_text("{" if name == "bad_json" else json.dumps(info), encoding="utf-8")
        result = subprocess.run([str(exe), "--smoke-test", "--raw", str(raw)],
                                env=env, capture_output=True, timeout=20)
        assert result.returncode == expected, (name, result.returncode, result.stderr)
        assert b"TypeError" not in result.stderr, (name, result.stderr)
        print("PASS", name)
