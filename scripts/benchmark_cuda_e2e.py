#!/usr/bin/env python3
"""Compare static and JIT CUDA paths with identical deterministic inputs."""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
from pathlib import Path

E2E_PATTERN = re.compile(
    r"\[cuda-e2e\] samples=(?P<samples>\d+) last_ms=(?P<last>[0-9.]+) "
    r"avg_ms=(?P<average>[0-9.]+)"
)
JIT_PATTERN = re.compile(r"\[cuda-jit\] family=(?P<family>\S+) backend=(?P<backend>\S+)")
GRAPH_PATTERN = re.compile(r"\[treepm\] cuda_graph=(?P<state>\S+)")
DONE_PATTERN = re.compile(
    r"\[headless\] done particles=(?P<particles>\d+) steps=(?P<steps>\d+) "
    r".*integrate_ms=(?P<integration>[0-9.]+).*time_ms=(?P<total>[0-9.]+)"
)


def run_case(binary: Path, config: Path, init_mode: str, particles: int, steps: int,
             solver: str, integrator: str, jit_enabled: bool, interval: int,
             substep_target_dt: float | None, treepm_enabled: bool, treepm_model: str,
             cosmology_enabled: bool) -> dict[str, object]:
    environment = os.environ.copy()
    environment["BLITZAR_CUDA_JIT"] = "1" if jit_enabled else "0"
    environment["BLITZAR_CUDA_E2E_PROFILE"] = "1"
    environment["BLITZAR_CUDA_E2E_PROFILE_INTERVAL"] = str(interval)
    command = [
        str(binary),
        "--run",
        "--config",
        str(config),
        "--particle-count",
        str(particles),
        "--target-steps",
        str(steps),
        "--solver",
        solver,
        "--integrator",
        integrator,
        "--init-mode",
        init_mode,
        "--cosmology-enabled",
        "true" if cosmology_enabled else "false",
        "--preset-structure",
        init_mode,
        "--treepm-enabled",
        "true" if treepm_enabled else "false",
        "--treepm-model",
        treepm_model,
        "--init-seed",
        "42",
        "--deterministic",
        "true",
        "--no-export-on-exit",
    ]
    if substep_target_dt is not None:
        command.extend(["--substep-target-dt", str(substep_target_dt)])
    completed = subprocess.run(command, env=environment, text=True, capture_output=True, check=False)
    output = completed.stdout + completed.stderr
    e2e_matches = list(E2E_PATTERN.finditer(output))
    done_match = DONE_PATTERN.search(output)
    jit_matches = list(JIT_PATTERN.finditer(output))
    graph_match = GRAPH_PATTERN.search(output)
    result: dict[str, object] = {
        "jit": jit_enabled,
        "returncode": completed.returncode,
        "e2e_samples": int(e2e_matches[-1].group("samples")) if e2e_matches else 0,
        "e2e_last_ms": float(e2e_matches[-1].group("last")) if e2e_matches else None,
        "e2e_average_ms": float(e2e_matches[-1].group("average")) if e2e_matches else None,
        "actual_particles": int(done_match.group("particles")) if done_match else None,
        "actual_steps": int(done_match.group("steps")) if done_match else None,
        "jit_variants": [
            {"family": match.group("family"), "backend": match.group("backend")}
            for match in jit_matches
        ],
        "treepm_graph": graph_match.group("state") if graph_match else "disabled",
        "integration_ms": float(done_match.group("integration")) if done_match else None,
        "total_ms": float(done_match.group("total")) if done_match else None,
    }
    if completed.returncode != 0:
        result["output_tail"] = output[-4000:]
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--config", type=Path,
                        default=Path("tests/data/scene_cosmology_preview.ini"))
    parser.add_argument("--init-mode", default="random_cloud")
    parser.add_argument("--particles", type=int, default=1_000_000)
    parser.add_argument("--steps", type=int, default=1000)
    parser.add_argument("--solver", default="pairwise_cuda")
    parser.add_argument("--integrator", default="euler")
    parser.add_argument("--interval", type=int, default=100)
    parser.add_argument("--substep-target-dt", type=float)
    parser.add_argument("--treepm-enabled", action=argparse.BooleanOptionalAction, default=False)
    parser.add_argument("--treepm-model", default="local_grid")
    parser.add_argument("--cosmology-enabled", action=argparse.BooleanOptionalAction, default=False)
    args = parser.parse_args()
    if args.particles <= 0 or args.steps <= 0 or args.interval <= 0:
        parser.error("particles, steps and interval must be positive")
    if not args.binary.is_file():
        parser.error(f"binary not found: {args.binary}")
    if not args.config.is_file():
        parser.error(f"config not found: {args.config}")

    results = [
        run_case(args.binary, args.config, args.init_mode, args.particles, args.steps, args.solver,
                 args.integrator, True, args.interval, args.substep_target_dt,
                 args.treepm_enabled, args.treepm_model, args.cosmology_enabled),
        run_case(args.binary, args.config, args.init_mode, args.particles, args.steps, args.solver,
                 args.integrator, False, args.interval, args.substep_target_dt,
                 args.treepm_enabled, args.treepm_model, args.cosmology_enabled),
    ]
    print(json.dumps({"config": vars(args), "results": results}, indent=2, default=str))
    return 0 if all(result["returncode"] == 0 for result in results) else 1


if __name__ == "__main__":
    raise SystemExit(main())
