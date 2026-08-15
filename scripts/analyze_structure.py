#!/usr/bin/env python3
"""Compute a reproducible 3-D density power spectrum from a BLITZAR snapshot."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from python_tools.analysis.structure_spectrum import analyze_snapshot, compare_structure


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", required=True, type=Path, help="final VTK or native BIN snapshot")
    parser.add_argument("--initial", type=Path, help="initial snapshot used for growth metrics")
    parser.add_argument("--output", type=Path, default=Path("artifacts/structure-spectrum.json"))
    parser.add_argument("--grid-size", type=int, default=64)
    parser.add_argument("--box-half-extent", type=float)
    parser.add_argument("--assignment", choices=("cic", "ngp"), default="cic")
    parser.add_argument("--window", choices=("hann", "none"), default="hann")
    parser.add_argument("--bins", type=int, default=24)
    args = parser.parse_args()
    if args.grid_size < 4 or args.bins < 2:
        parser.error("grid-size must be >= 4 and bins must be >= 2")
    result = analyze_snapshot(args.input.resolve(), args.grid_size, args.box_half_extent,
                              args.assignment, args.window, args.bins)
    if args.initial:
        initial = analyze_snapshot(args.initial.resolve(), args.grid_size, args.box_half_extent,
                                   args.assignment, args.window, args.bins)
        result["initial"] = initial
        result["growth"] = compare_structure(initial, result)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2, allow_nan=False) + "\n", encoding="utf-8")
    growth = result.get("growth", {})
    print(f"[spectrum] particles={result['particle_count']} grid={args.grid_size} "
          f"delta_rms={result['delta_rms']:.6g} peak_k={result['spectrum']['peak_k']:.6g} "
          f"power={result['spectrum']['nonzero_power']:.6g}")
    if growth:
        print(f"[spectrum] delta_rms_growth={growth['delta_rms_growth']:.6g} "
              f"power_growth={growth['nonzero_power_growth']:.6g}")
    print(f"[spectrum] report={args.output.resolve()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
