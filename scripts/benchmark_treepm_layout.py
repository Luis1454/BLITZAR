#!/usr/bin/env python3
"""Run a reproducible TreePM layout and hardware qualification matrix."""

from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import subprocess
from pathlib import Path

DONE_RE = re.compile(
    r"\[headless\] done particles=(?P<particles>\d+) steps=(?P<steps>\d+) "
    r"faulted=(?P<faulted>[01]).*?integrate_ms=(?P<integrate>\d+) "
    r".*?time_ms=(?P<total>\d+)"
)
TREEPM_RE = re.compile(r"\[treepm\] enabled .*?gather=(?P<gather>[01]) morton=(?P<morton>[01])")
AUTO_RE = re.compile(
    r"\[treepm\] auto_layout r80_ratio=(?P<r80>[0-9.]+) "
    r"threshold=(?P<threshold>[0-9.]+) selection=(?P<selection>\w+)"
)

LAYOUTS = {
    "linear": ("0", "0"),
    "gather_linear": ("1", "0"),
    "gather_morton": ("1", "1"),
    "auto": None,
}


def hardware_snapshot() -> list[str] | None:
    if shutil.which("nvidia-smi") is None:
        return None
    query = (
        "index,name,pstate,temperature.gpu,clocks.gr,clocks.mem,"
        "utilization.gpu,memory.used,memory.total,power.draw"
    )
    result = subprocess.run(
        ["nvidia-smi", f"--query-gpu={query}", "--format=csv,noheader,nounits"],
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        return None
    return [line.strip() for line in result.stdout.splitlines() if line.strip()]


def run_case(args: argparse.Namespace, morphology: str, particles: int, layout: str) -> dict:
    environment = os.environ.copy()
    environment.pop("BLITZAR_TREEPM_LAYOUT", None)
    environment.pop("BLITZAR_TREEPM_GATHER", None)
    environment.pop("BLITZAR_TREEPM_MORTON", None)
    if layout == "auto":
        environment["BLITZAR_TREEPM_LAYOUT"] = "auto"
    else:
        environment["BLITZAR_TREEPM_LAYOUT"] = layout
    command = [
        str(args.binary),
        "--run",
        "--config",
        str(args.config),
        "--particle-count",
        str(particles),
        "--target-steps",
        str(args.steps),
        "--solver",
        "octree_gpu",
        "--integrator",
        args.integrator,
        "--init-mode",
        morphology,
        "--preset-structure",
        morphology,
        "--cosmology-enabled",
        "true" if morphology == "cosmology" else "false",
        "--init-seed",
        str(args.seed),
        "--deterministic",
        "true",
        "--treepm-enabled",
        "true",
        "--treepm-model",
        "local_grid",
        "--no-export-on-exit",
    ]
    before = hardware_snapshot()
    try:
        completed = subprocess.run(
            command,
            capture_output=True,
            text=True,
            check=False,
            timeout=args.timeout_s,
            env=environment,
        )
        transcript = completed.stdout + completed.stderr
        timeout = False
    except subprocess.TimeoutExpired as error:
        transcript = (error.stdout or "") + (error.stderr or "")
        completed = None
        timeout = True
    after = hardware_snapshot()
    done = DONE_RE.search(transcript)
    marker = TREEPM_RE.search(transcript)
    auto_marker = AUTO_RE.search(transcript)
    result = {
        "morphology": morphology,
        "particles_requested": particles,
        "steps_requested": args.steps,
        "layout_requested": layout,
        "returncode": None if completed is None else completed.returncode,
        "timeout": timeout,
        "hardware_before": before,
        "hardware_after": after,
        "treepm_marker": marker.groupdict() if marker else None,
        "treepm_auto": auto_marker.groupdict() if auto_marker else None,
        "actual_particles": int(done.group("particles")) if done else None,
        "actual_steps": int(done.group("steps")) if done else None,
        "faulted": bool(done and done.group("faulted") == "1"),
        "integration_ms": int(done.group("integrate")) if done else None,
        "total_ms": int(done.group("total")) if done else None,
    }
    result["status"] = (
        "ok"
        if completed is not None
        and completed.returncode == 0
        and not timeout
        and done is not None
        and int(done.group("particles")) == particles
        and int(done.group("steps")) == args.steps
        and done.group("faulted") == "0"
        else "failed"
    )
    if result["status"] != "ok":
        result["output_tail"] = transcript[-4000:]
    return result


def result_key(result: dict) -> tuple[str, int, str]:
    return (
        result["morphology"],
        result["particles_requested"],
        result["layout_requested"],
    )


def write_payload(args: argparse.Namespace, results: list[dict], complete: bool) -> None:
    payload = {
        "schema": "blitzar.treepm.layout-matrix.v1",
        "binary": str(args.binary.resolve()),
        "config": str(args.config.resolve()),
        "deterministic": True,
        "complete": complete,
        "layouts": LAYOUTS,
        "results": results,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--config", type=Path, default=Path("tests/data/scene_cosmology_preview.ini"))
    parser.add_argument("--output", type=Path, default=Path("artifacts/treepm-layout-matrix.json"))
    parser.add_argument("--sizes", type=int, nargs="+", default=[100_000, 1_000_000])
    parser.add_argument("--morphologies", nargs="+", default=["random_cloud", "plummer_sphere", "cosmology"])
    parser.add_argument(
        "--layouts",
        nargs="+",
        choices=sorted(LAYOUTS),
        default=["linear", "gather_linear", "gather_morton"],
    )
    parser.add_argument("--steps", type=int, default=10)
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--timeout-s", type=float, default=900.0)
    parser.add_argument("--integrator", choices=("euler", "leapfrog"), default="euler")
    parser.add_argument("--resume", action="store_true")
    args = parser.parse_args()
    if not args.binary.is_file() or not args.config.is_file():
        parser.error("binary and config must exist")
    if any(size <= 0 for size in args.sizes) or args.steps <= 0:
        parser.error("sizes and steps must be positive")
    if any(morphology == "cosmology" for morphology in args.morphologies):
        args.morphologies = list(dict.fromkeys(args.morphologies))

    expected = [
        (morphology, particles, layout)
        for morphology in args.morphologies
        for particles in args.sizes
        for layout in args.layouts
    ]
    results_by_key: dict[tuple[str, int, str], dict] = {}
    if args.resume and args.output.is_file():
        previous = json.loads(args.output.read_text(encoding="utf-8"))
        for result in previous.get("results", []):
            if result.get("status") == "ok":
                results_by_key[result_key(result)] = result
        print(f"[matrix] resumed={len(results_by_key)}", flush=True)

    total = len(expected)
    for index, (morphology, particles, layout) in enumerate(expected, start=1):
        key = (morphology, particles, layout)
        if key in results_by_key:
            print(f"[matrix] skip={index}/{total} case={key}", flush=True)
            continue
        print(f"[matrix] start={index}/{total} case={key}", flush=True)
        result = run_case(args, morphology, particles, layout)
        results_by_key[key] = result
        ordered = [results_by_key[item] for item in expected if item in results_by_key]
        write_payload(args, ordered, len(ordered) == total and all(r["status"] == "ok" for r in ordered))
        print(
            f"[matrix] done={index}/{total} status={result['status']} "
            f"integration_ms={result['integration_ms']}",
            flush=True,
        )

    results = [results_by_key[item] for item in expected if item in results_by_key]
    write_payload(args, results, len(results) == total and all(r["status"] == "ok" for r in results))
    failed = sum(result["status"] != "ok" for result in results)
    print(f"[matrix] output={args.output.resolve()}")
    print(f"[matrix] cases={len(results)} failed={failed}")
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
