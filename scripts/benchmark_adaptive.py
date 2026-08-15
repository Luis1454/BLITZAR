#!/usr/bin/env python3
"""Cross-audit fixed and dyadic adaptive stepping for every solver/model."""

from __future__ import annotations

import argparse
import json
import math
import re
import subprocess
from pathlib import Path


CASES = [
    ("pairwise_cuda", "off"),
    ("octree_gpu", "off"),
    ("octree_cpu", "off"),
    ("octree_gpu", "local_grid"),
    ("octree_gpu", "pm_only"),
    ("octree_gpu", "tree"),
    ("octree_gpu", "hybrid"),
    ("octree_gpu", "exact_tree"),
    ("octree_cpu", "hybrid"),
]

DONE_RE = re.compile(
    r"\[headless\] done particles=(\d+) steps=(\d+) faulted=(\d+) "
    r"simulated_time=([-+0-9.eE]+).*?solver=\S+ integrator=(\S+) backend=(\S+) "
    r"init_ms=(\d+) integrate_ms=(\d+) export_ms=(\d+) time_ms=(\d+)",
    re.DOTALL,
)


def run_case(exe: Path, config: Path, solver: str, model: str, particles: int,
             steps: int, dt: float, adaptive: bool, output: Path | None,
             timeout: float, force_adaptive: bool) -> dict:
    command = [
        str(exe), "--run", "--config", str(config), "--solver", solver,
        "--integrator", "euler", "--particle-count", str(particles), "--dt", str(dt),
        "--target-steps", str(steps), "--deterministic", "true",
        "--adaptive-time-steps", "true" if adaptive else "false",
        "--adaptive-max-level", "3", "--adaptive-eta", "0.25",
        "--adaptive-cost-guard", "false" if adaptive and force_adaptive else "true",
    ]
    if model == "off":
        command += ["--treepm-enabled", "false"]
    else:
        command += ["--treepm-enabled", "true", "--treepm-model", model]
    if output is None:
        command += ["--no-export-on-exit"]
    else:
        command += ["--export-on-exit", "true", "--export-format", "vtk",
                    "--export-path", str(output)]
    try:
        completed = subprocess.run(command, capture_output=True, text=True,
                                   check=False, timeout=timeout)
        transcript = completed.stdout + completed.stderr
        returncode = completed.returncode
    except subprocess.TimeoutExpired as error:
        transcript = (error.stdout or "") + (error.stderr or "")
        returncode = -1
    match = DONE_RE.search(transcript)
    result = {
        "solver": solver,
        "model": model,
        "adaptive": adaptive,
        "returncode": returncode,
        "status": "failed",
        "log_tail": transcript[-2000:],
    }
    if match is None:
        return result
    values = match.groups()
    particle_count = int(values[0])
    simulated_time = float(values[3])
    integration_ms = int(values[7])
    result.update({
        "status": "ok" if returncode == 0 and values[2] == "0" else "failed",
        "particle_count": particle_count,
        "steps": int(values[1]),
        "faulted": values[2] == "1",
        "simulated_time": simulated_time,
        "execution_backend": values[5],
        "integrator": values[4],
        "initialization_ms": int(values[6]),
        "integration_ms": integration_ms,
        "export_ms": int(values[8]),
        "elapsed_ms": int(values[9]),
        "dt_per_second": (simulated_time * 1000.0 / integration_ms
                           if integration_ms > 0 else None),
        "particle_updates_per_second": (particle_count * int(values[1]) * 1000.0 /
                                         integration_ms if integration_ms > 0 else None),
    })
    marker = re.search(r"\[adaptive\] backend=(\S+)", transcript)
    result["adaptive_backend"] = marker.group(1) if marker else "disabled"
    return result


def parse_vtk(path: Path) -> dict:
    lines = path.read_text(encoding="ascii").splitlines()

    points_line = next(i for i, line in enumerate(lines) if line.startswith("POINTS "))
    count = int(lines[points_line].split()[1])
    positions = [tuple(map(float, lines[points_line + 1 + i].split())) for i in range(count)]

    mass_line = next(i for i, line in enumerate(lines) if line == "SCALARS mass float 1")
    masses = [float(lines[mass_line + 2 + i]) for i in range(count)]
    velocity_line = next(i for i, line in enumerate(lines) if line == "VECTORS velocity float")
    velocities = [tuple(map(float, lines[velocity_line + 1 + i].split()))
                  for i in range(count)]
    return {"positions": positions, "velocities": velocities, "masses": masses}


def state_metrics(state: dict, softening: float) -> dict:
    positions = state["positions"]
    velocities = state["velocities"]
    masses = state["masses"]
    total_mass = sum(masses)
    kinetic = 0.0
    momentum = [0.0, 0.0, 0.0]
    center = [0.0, 0.0, 0.0]
    for position, velocity, mass in zip(positions, velocities, masses):
        kinetic += 0.5 * mass * sum(value * value for value in velocity)
        for axis in range(3):
            center[axis] += mass * position[axis]
            momentum[axis] += mass * velocity[axis]
    if total_mass > 0.0:
        center = [value / total_mass for value in center]
    potential = 0.0
    softening2 = softening * softening
    for i in range(len(positions)):
        for j in range(i + 1, len(positions)):
            distance2 = sum((positions[j][axis] - positions[i][axis]) ** 2 for axis in range(3))
            potential -= masses[i] * masses[j] / math.sqrt(distance2 + softening2)
    return {
        "kinetic_energy": kinetic,
        "potential_energy": potential,
        "total_energy": kinetic + potential,
        "center_of_mass": center,
        "momentum": momentum,
        "finite": all(math.isfinite(value) for vector in positions + velocities
                       for value in vector),
    }


def trajectory_error(reference: dict, candidate: dict) -> dict:
    position_squared = 0.0
    velocity_squared = 0.0
    max_position = 0.0
    max_velocity = 0.0
    count = len(reference["positions"])
    for expected, actual in zip(reference["positions"], candidate["positions"]):
        error = math.sqrt(sum((actual[i] - expected[i]) ** 2 for i in range(3)))
        position_squared += error * error
        max_position = max(max_position, error)
    for expected, actual in zip(reference["velocities"], candidate["velocities"]):
        error = math.sqrt(sum((actual[i] - expected[i]) ** 2 for i in range(3)))
        velocity_squared += error * error
        max_velocity = max(max_velocity, error)
    return {
        "position_rms_error": math.sqrt(position_squared / max(count, 1)),
        "position_max_error": max_position,
        "velocity_rms_error": math.sqrt(velocity_squared / max(count, 1)),
        "velocity_max_error": max_velocity,
    }


def add_accuracy(metrics: dict, initial: dict, final: dict, baseline: dict | None,
                 softening: float) -> None:
    initial_metrics = state_metrics(initial, softening)
    final_metrics = state_metrics(final, softening)
    initial_energy = initial_metrics["total_energy"]
    final_energy = final_metrics["total_energy"]
    energy_scale = max(abs(initial_energy), 1.0e-12)
    metrics["accuracy"] = {
        "initial": initial_metrics,
        "final": final_metrics,
        "energy_drift_pct": 100.0 * (final_energy - initial_energy) / energy_scale,
        "center_of_mass_drift": math.sqrt(sum(
            (final_metrics["center_of_mass"][axis] - initial_metrics["center_of_mass"][axis]) ** 2
            for axis in range(3))),
        "momentum_drift": math.sqrt(sum(
            (final_metrics["momentum"][axis] - initial_metrics["momentum"][axis]) ** 2
            for axis in range(3))),
    }
    if baseline is not None:
        metrics["accuracy"]["vs_fixed"] = trajectory_error(baseline, final)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", action="append", required=True, type=Path,
                        help="headless executable; repeat for GPU and CPU builds")
    parser.add_argument("--config", type=Path, default=Path("simulation.ini"))
    parser.add_argument("--output", type=Path, default=Path("artifacts/adaptive-audit.json"))
    parser.add_argument("--performance-particles", type=int, default=2048)
    parser.add_argument("--accuracy-particles", type=int, default=512)
    parser.add_argument("--steps", type=int, default=8)
    parser.add_argument("--dt", type=float, default=0.01)
    parser.add_argument("--timeout-s", type=float, default=180.0)
    parser.add_argument("--softening", type=float, default=2.5)
    parser.add_argument("--force-adaptive", action="store_true",
                        help="disable the production cost guard for native research timings")
    args = parser.parse_args()
    output = args.output.resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    all_results = []
    technical_failures = []

    for executable in args.exe:
        exe = executable.resolve()
        if not exe.is_file():
            raise SystemExit(f"missing executable: {exe}")
        label = exe.parent.name
        initial_path = output.parent / f"adaptive-{label}-initial.vtk"
        initial_run = run_case(exe, args.config.resolve(), "pairwise_cuda", "off",
                               args.accuracy_particles, 0, args.dt, False, initial_path,
                               args.timeout_s, args.force_adaptive)
        if initial_run["status"] != "ok":
            technical_failures.append(initial_run)
            continue
        initial_state = parse_vtk(initial_path)
        for solver, model in CASES:
            fixed = run_case(exe, args.config.resolve(), solver, model,
                             args.performance_particles, args.steps, args.dt, False, None,
                             args.timeout_s, args.force_adaptive)
            adaptive = run_case(exe, args.config.resolve(), solver, model,
                                args.performance_particles, args.steps, args.dt, True, None,
                                args.timeout_s, args.force_adaptive)
            row = {"executable": str(exe), "executable_label": label,
                   "solver": solver, "model": model, "fixed": fixed,
                   "adaptive": adaptive}
            if fixed["status"] != "ok" or adaptive["status"] != "ok":
                technical_failures.append(row)
            else:
                row["adaptive_gain_pct"] = 100.0 * (
                    adaptive["dt_per_second"] / max(fixed["dt_per_second"], 1.0e-30) - 1.0)
                fixed_path = output.parent / f"adaptive-{label}-{solver}-{model}-fixed.vtk"
                adaptive_path = output.parent / f"adaptive-{label}-{solver}-{model}-adaptive.vtk"
                fixed_accuracy = run_case(exe, args.config.resolve(), solver, model,
                                          args.accuracy_particles, args.steps, args.dt, False,
                                          fixed_path, args.timeout_s, args.force_adaptive)
                adaptive_accuracy = run_case(exe, args.config.resolve(), solver, model,
                                             args.accuracy_particles, args.steps, args.dt, True,
                                             adaptive_path, args.timeout_s, args.force_adaptive)
                if fixed_accuracy["status"] != "ok" or adaptive_accuracy["status"] != "ok":
                    technical_failures.append(row)
                else:
                    fixed_state = parse_vtk(fixed_path)
                    adaptive_state = parse_vtk(adaptive_path)
                    add_accuracy(fixed_accuracy, initial_state, fixed_state, None, args.softening)
                    add_accuracy(adaptive_accuracy, initial_state, adaptive_state,
                                 fixed_state, args.softening)
                    row["fixed_accuracy"] = fixed_accuracy["accuracy"]
                    row["adaptive_accuracy"] = adaptive_accuracy["accuracy"]
            all_results.append(row)

    audit = {
        "schema": "blitzar.adaptive.audit.v1",
        "deterministic": True,
        "dt": args.dt,
        "performance_particles": args.performance_particles,
        "accuracy_particles": args.accuracy_particles,
        "steps": args.steps,
        "adaptive_max_level": 3,
        "adaptive_eta": 0.25,
        "force_adaptive": args.force_adaptive,
        "softening_for_energy": args.softening,
        "results": all_results,
        "technical_failures": technical_failures,
    }
    output.write_text(json.dumps(audit, indent=2) + "\n", encoding="utf-8")
    rows = [row for row in all_results if row.get("adaptive", {}).get("status") == "ok"]
    print(f"[adaptive-audit] output={output}")
    print(f"[adaptive-audit] rows={len(all_results)} technical_failures={len(technical_failures)}")
    for row in rows:
        print(f"[adaptive-audit] {row['executable_label']} {row['solver']}/{row['model']} "
              f"fixed_dt_s={row['fixed']['dt_per_second']:.6g} "
              f"adaptive_dt_s={row['adaptive']['dt_per_second']:.6g} "
              f"gain_pct={row.get('adaptive_gain_pct', float('nan')):.3f}")
    return 1 if technical_failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
