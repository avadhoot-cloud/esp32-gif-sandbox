#!/usr/bin/env python3
"""Optimize assets/sample.gif the same way as StatusBar backend (standalone)."""
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT.parent / "StatusBar" / "backend"))

from gif_optimizer import optimize_gif_bytes  # noqa: E402

def main() -> None:
    src = ROOT / "assets" / "sample.gif"
    out = ROOT / "assets" / "sample_optimized.gif"
    raw = src.read_bytes()
    opt = optimize_gif_bytes(raw)
    out.write_bytes(opt)
    print(f"{src.name}: {len(raw)} -> {len(opt)} bytes -> {out}")

if __name__ == "__main__":
    main()
