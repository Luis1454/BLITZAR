#!/usr/bin/env python3
"""Run a reproducible TreePM performance and force-accuracy audit."""

from __future__ import annotations

import argparse
import json
import math
import re
import subprocess
import tempfile
from pathlib import Path

DONE_RE = re.compile(
    r"\[headless\] done particles=(?P<particles>\d+) steps=(?P<steps>\d+) "
    r"faulted=(?P<faulted>[01]).*?backend=(?P<backend>\S+) "
    r"init_ms=(?P<init>\d+) integrate_ms=(?P<integrate>\d+) "
    r"export_ms=(?P<export>\d+) time_ms=(?P<elapsed>\d+)"
)
MEMORY_RE = re.compile(r"\[info\] \[memory\] TOTAL: (?P<used>[0-9.]+) MB / (?P<total>[0-9.]+) MB")
CPU_TREEPM_FALLBACK_RE = re.compile(
    r"\[treepm\] requested model=(?P<model>\S+) but backend=octree_cpu"
)
TREEPM_SOLVER_RE = re.compile(r"\[treepm\] enabled solver=(?P<solver>\S+) model=")


def run_case(exe: Path, config: Path, solver: str, model: str, particles: int,
             steps: int, export_path: Path | None, timeout: float,
             treepm_options: list[str]) -> dict:
    command = [
        str(exe), "--run", "--config", str(config), "--solver", solver,
        "--integrator", "euler", "--particle-count", str(particles),
        "--target-steps", str(steps), "--deterministic", "true",
    ]
    if model == "off":
        command += ["--treepm-enabled", "false"]
    else:
        command += ["--treepm-enabled", "true", "--treepm-model", model]
        command += treepm_options
    if export_path is None:
        command += ["--no-export-on-exit"]
    else:
        command += ["--export-on-exit", "true", "--export-format", "vtk",
                    "--export-path", str(export_path)]
    try:
        completed = subprocess.run(command, capture_output=True, text=True,
                                   check=False, timeout=timeout)
        transcript = completed.stdout + completed.stderr
        returncode = completed.returncode
    except subprocess.TimeoutExpired as error:
        transcript = (error.stdout or "") + (error.stderr or "")
        returncode = -1
    done = DONE_RE.search(transcript)
    memories = MEMORY_RE.findall(transcript)
    result = {
        "solver_requested": solver,
        "treepm_requested": model,
        "returncode": returncode,
        "memory_mb": float(memories[-1][0]) if memories else None,
        "memory_total_mb": float(memories[-1][1]) if memories else None,
        "log_tail": transcript[-4000:],
    }
    fallback = CPU_TREEPM_FALLBACK_RE.search(transcript)
    solver_match = TREEPM_SOLVER_RE.search(transcript)
    result["treepm_solver"] = solver_match.group("solver") if solver_match else None
    if model == "off":
        result["effective_execution"] = "disabled"
    elif fallback is not None:
        result["effective_execution"] = "cpu_octree_fallback"
    elif model == "exact_tree":
        result["effective_execution"] = (
            "cuda_exact_octree" if solver == "octree_gpu" else "cpu_exact_octree"
        )
    elif solver_match is not None and solver_match.group("solver").startswith("cpu_fft"):
        result["effective_execution"] = "cpu_treepm_fft"
    elif solver == "octree_gpu":
        result["effective_execution"] = "cuda_treepm"
    elif solver == "octree_cpu":
        result["effective_execution"] = "cpu_treepm_fft"
    else:
        result["effective_execution"] = "backend_specific"
    if done is None:
        result["status"] = "failed"
        return result
    values = done.groupdict()
    integration_ms = int(values["integrate"])
    result.update({
        "status": "ok" if returncode == 0 and values["faulted"] == "0" else "failed",
        "particle_count": int(values["particles"]),
        "steps": int(values["steps"]),
        "faulted": values["faulted"] == "1",
        "execution_backend": values["backend"],
        "initialization_ms": int(values["init"]),
        "integration_ms": integration_ms,
        "export_ms": int(values["export"]),
        "elapsed_ms": int(values["elapsed"]),
        "particle_updates_per_second": (
            particles * steps * 1000.0 / integration_ms if integration_ms > 0 else None
        ),
    })
    return result


def read_accelerations(path: Path, count: int) -> list[tuple[float, float, float]]:
    lines = path.read_text(encoding="ascii").splitlines()
    try:
        marker = lines.index("VECTORS acceleration float")
    except ValueError as error:
        raise ValueError(f"missing acceleration vector field in {path}") from error
    vectors = []
    for line in lines[marker + 1:marker + 1 + count]:
        values = line.split()
        if len(values) != 3:
            raise ValueError(f"invalid acceleration vector in {path}: {line!r}")
        vectors.append((float(values[0]), float(values[1]), float(values[2])))
    if len(vectors) != count:
        raise ValueError(f"expected {count} accelerations in {path}, got {len(vectors)}")
    return vectors


def force_metrics(reference: list[tuple[float, float, float]],
                  candidate: list[tuple[float, float, float]]) -> dict:
    if len(reference) != len(candidate):
        raise ValueError("reference and candidate particle counts differ")
    squared_error = 0.0
    squared_reference = 0.0
    squared_candidate = 0.0
    max_error = 0.0
    nonfinite = 0
    for expected, actual in zip(reference, candidate):
        if not all(math.isfinite(value) for value in (*expected, *actual)):
            nonfinite += 1
            continue
        difference = math.sqrt(sum((actual[i] - expected[i]) ** 2 for i in range(3)))
        squared_error += difference * difference
        squared_reference += sum(value * value for value in expected)
        squared_candidate += sum(value * value for value in actual)
        max_error = max(max_error, difference)
    count = len(reference)
    rms_error = math.sqrt(squared_error / count) if count else None
    reference_rms = math.sqrt(squared_reference / count) if count else None
    candidate_rms = math.sqrt(squared_candidate / count) if count else None
    return {
        "particle_count": count,
        "nonfinite_vectors": nonfinite,
        "rms_force_error": rms_error,
        "reference_rms_force": reference_rms,
        "candidate_rms_force": candidate_rms,
        "relative_rms_force_error": (
            rms_error / max(reference_rms, 1.0e-12)
            if rms_error is not None and reference_rms is not None else None
        ),
        "max_force_error": max_error,
    }


def parser() -> argparse.ArgumentParser:
    result = argparse.ArgumentParser(description=__doc__)
    result.add_argument("--exe", required=True, type=Path)
    result.add_argument("--config", type=Path, default=Path("simulation.ini"))
    result.add_argument("--output", type=Path, default=Path("treepm-audit.json"))
    result.add_argument("--particle-count", type=int, default=2048)
    result.add_argument("--steps", type=int, default=20)
    result.add_argument("--timeout-s", type=float, default=180.0)
    result.add_argument("--grid-size", type=int)
    result.add_argument("--jacobi-iters", type=int)
    result.add_argument("--treepm-precision", choices=("fp32", "fp64"), default="fp32")
    result.add_argument("--treepm-assignment", choices=("cic", "tsc", "pcs"), default="cic")
    result.add_argument("--max-relative-error", type=float, default=0.25)
    return result


def main() -> int:
    args = parser().parse_args()
    exe = args.exe.resolve()
    config = args.config.resolve()
    if not exe.is_file() or not config.is_file():
        raise SystemExit(f"missing executable or config: {exe} {config}")
    cases = [("pairwise_cuda", "off"), ("octree_gpu", "off"),
             ("octree_cpu", "off"), ("octree_gpu", "local_grid"),
             ("octree_gpu", "pm_only"), ("octree_gpu", "tree"),
             ("octree_gpu", "hybrid"), ("octree_gpu", "exact_tree"),
             ("octree_cpu", "hybrid")]
    treepm_options = []
    if args.grid_size is not None:
        treepm_options += ["--treepm-grid-size", str(args.grid_size)]
    if args.jacobi_iters is not None:
        treepm_options += ["--treepm-jacobi-iters", str(args.jacobi_iters)]
    treepm_options += ["--treepm-precision", args.treepm_precision]
    treepm_options += ["--treepm-assignment", args.treepm_assignment]
    performance = []
    precision = []
    with tempfile.TemporaryDirectory(prefix="blitzar-treepm-") as directory:
        temp = Path(directory)
        reference = None
        for index, (solver, model) in enumerate(cases):
            performance.append(run_case(exe, config, solver, model, args.particle_count,
                                         args.steps, None, args.timeout_s, treepm_options))
            export = temp / f"case-{index}.vtk"
            result = run_case(exe, config, solver, model, args.particle_count, 1,
                              export, args.timeout_s, treepm_options)
            precision.append(result)
            if solver == "pairwise_cuda" and model == "off" and result.get("status") == "ok":
                reference = read_accelerations(export, args.particle_count)
            if result.get("status") == "ok" and reference is not None:
                result["force_metrics"] = force_metrics(
                    reference, read_accelerations(export, args.particle_count))
                result["precision_pass"] = (
                    result["force_metrics"]["relative_rms_force_error"] <= args.max_relative_error
                )
    audit = {
        "schema": "blitzar.treepm.audit.v1",
        "executable": str(exe),
        "config": str(config),
        "deterministic": True,
        "particle_count": args.particle_count,
        "performance_steps": args.steps,
        "treepm_options": treepm_options,
        "max_relative_error": args.max_relative_error,
        "performance": performance,
        "precision": precision,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(audit, indent=2) + "\n", encoding="utf-8")
    failures = [row for row in performance + precision if row.get("status") != "ok"]
    precision_failures = [row for row in precision if row.get("precision_pass") is False]
    print(f"[audit] output={args.output.resolve()}")
    print(f"[audit] cases={len(cases)} technical_failures={len(failures)} "
          f"precision_failures={len(precision_failures)}")
    return 1 if failures or precision_failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
