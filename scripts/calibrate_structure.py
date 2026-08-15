#!/usr/bin/env python3
"""Sweep cosmology parameters and select a stable Fourier-structure candidate."""

from __future__ import annotations

import argparse
import itertools
import json
import math
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from python_tools.analysis.structure_spectrum import analyze_snapshot, compare_structure

DONE_RE = re.compile(r"\[headless\] done particles=(\d+) steps=(\d+) faulted=(\d+)")


def _values(raw: str) -> list[float]:
    values = [float(value.strip()) for value in raw.split(",") if value.strip()]
    if not values or any(value <= 0.0 for value in values):
        raise ValueError(f"expected positive comma-separated values, got {raw!r}")
    return values


def _config_extent(path: Path) -> float:
    match = re.search(r"cosmology\([^\n]*box_half_extent=([-+0-9.eE]+)",
                      path.read_text(encoding="utf-8"))
    return float(match.group(1)) if match else 48.0


def _run(exe: Path, config: Path, output: Path, overrides: dict[str, object], steps: int,
         timeout: float, solver: str, integrator: str) -> dict[str, object]:
    command = [str(exe), "--run", "--config", str(config), "--solver", solver,
               "--integrator", integrator, "--deterministic", "true", "--target-steps",
               str(steps), "--export-on-exit", "true", "--export-format", "vtk",
               "--export-path", str(output)]
    for key, value in overrides.items():
        command.extend([key, str(value)])
    completed = subprocess.run(command, capture_output=True, text=True, check=False,
                               timeout=timeout)
    transcript = completed.stdout + completed.stderr
    match = DONE_RE.search(transcript)
    if completed.returncode != 0 or match is None or match.group(3) != "0":
        raise RuntimeError(f"simulation failed for {overrides}: {transcript[-1200:]}")
    return {"command": command, "particles": int(match.group(1)), "steps": int(match.group(2)),
            "stdout_tail": transcript[-1200:]}


def _score(metrics: dict[str, float], target_rms: float, max_rms: float) -> float:
    rms = max(metrics["final_delta_rms"], 1.0e-12)
    growth = max(metrics["nonzero_power_growth"], 0.0)
    target_penalty = math.exp(-abs(math.log(rms / target_rms)))
    collapse_penalty = min(1.0, max_rms / rms)
    return math.log1p(growth) * target_penalty * collapse_penalty


def _select_candidates(values: list[tuple[float, float, float, float]], limit: int) -> list[tuple[float, float, float, float]]:
    if len(values) <= limit:
        return values
    if limit == 1:
        return [values[0]]
    indices = [round(index * (len(values) - 1) / (limit - 1)) for index in range(limit)]
    return [values[index] for index in indices]


def _write_best_config(source: Path, destination: Path, parameters: dict[str, float]) -> None:
    content = source.read_text(encoding="utf-8")
    replacements = {
        "dt": parameters["dt"],
        "softening": parameters["softening"],
        "h0": parameters["h0"],
        "perturbation": parameters["perturbation"],
    }
    for key, value in replacements.items():
        content = re.sub(rf"(?<={key}=)[-+0-9.eE]+", f"{value:.12g}", content, count=1)
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(content, encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, type=Path, help="blitzar-headless executable")
    parser.add_argument("--config", type=Path, default=Path("tests/data/scene_cosmology_preview.ini"))
    parser.add_argument("--output", type=Path, default=Path("artifacts/structure-calibration.json"))
    parser.add_argument("--particle-count", type=int, default=1024)
    parser.add_argument("--steps", type=int, default=40)
    parser.add_argument("--solver", default="octree_cpu", choices=("octree_cpu", "octree_gpu", "pairwise_cuda"))
    parser.add_argument("--integrator", default="leapfrog", choices=("euler", "leapfrog", "rk4"))
    parser.add_argument("--grid-size", type=int, default=32)
    parser.add_argument("--max-runs", type=int, default=16)
    parser.add_argument("--timeout-s", type=float, default=180.0)
    parser.add_argument("--target-delta-rms", type=float, default=1.0)
    parser.add_argument("--max-delta-rms", type=float, default=8.0)
    parser.add_argument("--perturbations", default="0.01,0.02")
    parser.add_argument("--h0-values", default="0.001,0.003")
    parser.add_argument("--softening-values", default="0.04,0.08")
    parser.add_argument("--dt-values", default="0.00005,0.0001")
    args = parser.parse_args()
    if args.max_runs < 1 or args.particle_count < 2 or args.steps < 1:
        parser.error("max-runs, particle-count and steps must be positive")
    exe = args.exe.resolve()
    config = args.config.resolve()
    output = args.output.resolve()
    extent = _config_extent(config)
    all_candidates = list(itertools.product(_values(args.perturbations),
                                             _values(args.h0_values),
                                             _values(args.softening_values),
                                             _values(args.dt_values)))
    candidates = _select_candidates(all_candidates, args.max_runs)
    rows: list[dict[str, object]] = []
    for index, (perturbation, h0, softening, dt) in enumerate(candidates):
        parameters = {"perturbation": perturbation, "h0": h0, "softening": softening, "dt": dt}
        case_dir = output.parent / "structure-calibration" / f"case-{index:03d}"
        initial_path = case_dir / "initial.vtk"
        final_path = case_dir / "final.vtk"
        try:
            overrides = {"--particle-count": args.particle_count,
                         "--cosmology-perturbation": perturbation, "--cosmology-h0": h0,
                         "--octree-softening": softening, "--dt": dt}
            initial_run = _run(exe, config, initial_path, overrides, 0, args.timeout_s,
                               args.solver, args.integrator)
            final_run = _run(exe, config, final_path, overrides, args.steps, args.timeout_s,
                             args.solver, args.integrator)
            initial = analyze_snapshot(initial_path, args.grid_size, extent)
            final = analyze_snapshot(final_path, args.grid_size, extent)
            metrics = compare_structure(initial, final)
            row = {"id": index, "parameters": parameters, "initial_run": initial_run,
                   "final_run": final_run, "metrics": metrics,
                   "score": _score(metrics, args.target_delta_rms, args.max_delta_rms),
                   "status": "ok"}
        except (OSError, RuntimeError, ValueError, subprocess.TimeoutExpired) as error:
            row = {"id": index, "parameters": parameters, "status": "failed",
                   "error": str(error)}
        rows.append(row)
        print(f"[calibration] case={index:03d} status={row['status']} "
              f"score={row.get('score', 0.0):.6g}")
    valid = [row for row in rows if row["status"] == "ok"]
    if not valid:
        raise RuntimeError("no calibration candidate completed successfully")
    best = max(valid, key=lambda row: float(row["score"]))
    best_config = output.parent / "structure-calibration-best.ini"
    _write_best_config(config, best_config, best["parameters"])
    report = {"schema": "blitzar.structure-calibration.v1", "deterministic": True,
              "objective": {"name": "spectral_growth_with_rms_target", "target_delta_rms": args.target_delta_rms,
                             "max_delta_rms": args.max_delta_rms}, "grid_size": args.grid_size,
              "box_half_extent": extent, "runs": rows, "best": best,
              "best_config": str(best_config)}
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=2, allow_nan=False) + "\n", encoding="utf-8")
    print(f"[calibration] best_case={best['id']} score={best['score']:.6g}")
    print(f"[calibration] best_config={best_config}")
    print(f"[calibration] report={output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
