#!/usr/bin/env python3
"""
Test USB upload reliability at multiple baud rates and chunk sizes.
Writes results/tools/results/upload_matrix.json

Requires: pip install pyserial pillow
Flash sandbox firmware first (default 115200). For other bauds, rebuild with:
  build_flags = -D SANDBOX_SERIAL_BAUD=921600
"""
from __future__ import annotations

import argparse
import json
import sys
import time
from pathlib import Path

try:
    import serial
except ImportError:
    print("Install pyserial: pip install pyserial")
    sys.exit(1)

ROOT = Path(__file__).resolve().parents[1]
RESULTS_DIR = ROOT / "tools" / "results"

BAUDS = [115200, 460800, 921600]
DEFAULT_CHUNK_SIZES = [1024]
SETTLE_S = 0.05
READ_TIMEOUT = 2.0


def read_json_line(ser: serial.Serial, deadline: float) -> dict | None:
    buf = b""
    while time.time() < deadline:
        chunk = ser.read(ser.in_waiting or 1)
        if not chunk:
            time.sleep(0.01)
            continue
        buf += chunk
        while b"\n" in buf:
            line, buf = buf.split(b"\n", 1)
            text = line.decode(errors="replace").strip()
            if not text or not text.startswith("{"):
                continue
            try:
                return json.loads(text)
            except json.JSONDecodeError:
                if "send chun" in text or "chunk rec" in text:
                    repaired = (
                        text.replace('"success,"', '"success",')
                        .replace("send chun", "send chunk")
                    )
                    try:
                        return json.loads(repaired)
                    except json.JSONDecodeError:
                        pass
                return {"_raw": text, "_corrupt": True}
    return None


def write_line(ser: serial.Serial, payload: dict) -> None:
    ser.write((json.dumps(payload, separators=(",", ":")) + "\n").encode())
    ser.flush()


def expect_message(ser: serial.Serial, message: str, timeout: float = 30.0) -> tuple[bool, str]:
    deadline = time.time() + timeout
    while time.time() < deadline:
        msg = read_json_line(ser, deadline)
        if not msg:
            continue
        if msg.get("_corrupt"):
            return False, f"corrupt:{msg.get('_raw', '')[:80]}"
        if msg.get("message") == message and msg.get("status") == "success":
            return True, "ok"
        if msg.get("status") == "error":
            return False, msg.get("message", "error")
    return False, "timeout"


def upload_file(ser: serial.Serial, data: bytes, chunk_size: int, remote_name: str) -> tuple[bool, str]:
    write_line(ser, {"cmd": "FORMAT_FS"})
    ok, detail = expect_message(ser, "formatted", 10)
    if not ok:
        return False, f"format:{detail}"

    write_line(ser, {"cmd": "START_UPLOAD", "file": remote_name, "size": len(data)})
    ok, detail = expect_message(ser, "ready for chunks", 10)
    if not ok:
        return False, f"start:{detail}"

    offset = 0
    while offset < len(data):
        chunk = data[offset : offset + chunk_size]
        write_line(ser, {"cmd": "CHUNK", "size": len(chunk)})
        ok, detail = expect_message(ser, "send chunk", 30)
        if not ok:
            return False, f"send_chunk@{offset}:{detail}"

        ser.write(chunk)
        ser.flush()
        time.sleep(SETTLE_S)

        ok, detail = expect_message(ser, "chunk received", 60)
        if not ok:
            return False, f"chunk_ack@{offset}:{detail}"

        offset += len(chunk)

    write_line(ser, {"cmd": "END_UPLOAD"})
    ok, detail = expect_message(ser, "upload complete", 15)
    if not ok:
        return False, f"end:{detail}"
    return True, "ok"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", default="COM9")
    parser.add_argument("--gif", default=str(ROOT / "assets" / "sample_240.gif"))
    parser.add_argument("--baud", type=int, default=0, help="Single baud (0 = use firmware default only)")
    parser.add_argument("--chunks", default="1024", help="Comma-separated chunk sizes to test")
    parser.add_argument("--no-play", action="store_true", help="Do not start GIF playback after each upload")
    args = parser.parse_args()
    try:
        chunk_sizes = [int(c.strip()) for c in args.chunks.split(",") if c.strip()]
    except ValueError:
        print(f"Bad --chunks value: {args.chunks}")
        return 1
    if not chunk_sizes:
        chunk_sizes = DEFAULT_CHUNK_SIZES

    gif_path = Path(args.gif)
    if not gif_path.is_file():
        fallback = ROOT / "assets" / "sample.gif"
        print(f"GIF not found: {gif_path}")
        print(f"Create it with: python tools/prepare_gif.py {fallback} {gif_path}")
        return 1

    data = gif_path.read_bytes()
    print(f"Loaded {gif_path.name}: {len(data)} bytes")

    RESULTS_DIR.mkdir(parents=True, exist_ok=True)
    results: list[dict] = []

    baud_list = [args.baud] if args.baud else [115200]

    for baud in baud_list:
        print(f"\n=== Testing at {baud} baud (firmware must be built for this baud) ===")
        try:
            ser = serial.Serial(args.port, baud, timeout=READ_TIMEOUT)
        except serial.SerialException as e:
            print(f"Cannot open {args.port}: {e}")
            return 1

        time.sleep(2)
        ser.reset_input_buffer()

        for chunk in chunk_sizes:
            print(f"  chunk={chunk} ... ", end="", flush=True)
            t0 = time.time()
            ok, detail = upload_file(ser, data, chunk, gif_path.name)
            elapsed = round(time.time() - t0, 1)
            bytes_per_second = round(len(data) / elapsed, 1) if elapsed else 0
            kbps = round((len(data) * 8) / elapsed / 1000, 1) if elapsed else 0
            if ok and not args.no_play:
                write_line(ser, {"cmd": "PLAY_GIF", "file": gif_path.name})
                play_ok, play_detail = expect_message(ser, "playing", 10)
                if not play_ok:
                    ok = False
                    detail = f"play:{play_detail}"
            row = {
                "baud": baud,
                "chunk_size": chunk,
                "ok": ok,
                "detail": detail,
                "seconds": elapsed,
                "bytes": len(data),
                "bytes_per_second": bytes_per_second,
                "kilobits_per_second": kbps,
            }
            results.append(row)
            rate = f"{bytes_per_second:.1f} B/s ({kbps:.1f} kbps)"
            print(("PASS" if ok else f"FAIL ({detail})") + f" in {elapsed}s, {rate}")

        ser.close()

    out = RESULTS_DIR / "upload_matrix.json"
    out.write_text(json.dumps(results, indent=2), encoding="utf-8")
    print(f"\nWrote {out}")

    passed = [r for r in results if r["ok"]]
    if passed:
        best = min(passed, key=lambda r: (r["baud"], -r["chunk_size"]))
        print(f"Recommended: {best['baud']} baud, chunk {best['chunk_size']}")
    return 0 if all(r["ok"] for r in results) else 1


if __name__ == "__main__":
    sys.exit(main())
