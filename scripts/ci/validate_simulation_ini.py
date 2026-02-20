#!/usr/bin/env python3
"""Minimal validation for simulation.ini used by CI fast lane."""

from __future__ import annotations

import argparse
import pathlib
import sys


def parse_ini(path: pathlib.Path) -> dict[str, str]:
    data: dict[str, str] = {}
    for line_no, raw_line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        if "=" not in line:
            raise ValueError(f"{path}:{line_no}: expected key=value line")
        key, value = line.split("=", 1)
        key = key.strip()
        value = value.strip()
        if not key:
            raise ValueError(f"{path}:{line_no}: empty key is not allowed")
        data[key] = value
    return data


def expect_int(values: dict[str, str], key: str, minimum: int | None = None) -> list[str]:
    errors: list[str] = []
    raw = values.get(key, "")
    try:
        parsed = int(raw)
    except ValueError:
        errors.append(f"{key}: expected integer, got '{raw}'")
        return errors
    if minimum is not None and parsed < minimum:
        errors.append(f"{key}: expected >= {minimum}, got {parsed}")
    return errors


def expect_float(values: dict[str, str], key: str, minimum: float | None = None) -> list[str]:
    errors: list[str] = []
    raw = values.get(key, "")
    try:
        parsed = float(raw)
    except ValueError:
        errors.append(f"{key}: expected float, got '{raw}'")
        return errors
    if minimum is not None and parsed < minimum:
        errors.append(f"{key}: expected >= {minimum}, got {parsed}")
    return errors


def expect_enum(values: dict[str, str], key: str, options: set[str]) -> list[str]:
    raw = values.get(key, "")
    if raw not in options:
        return [f"{key}: expected one of {sorted(options)}, got '{raw}'"]
    return []


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--path", default="simulation.ini")
    args = parser.parse_args()

    path = pathlib.Path(args.path)
    if not path.exists():
        print(f"error: missing config file: {path}", file=sys.stderr)
        return 1

    try:
        values = parse_ini(path)
    except ValueError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    required_keys = {
        "particle_count",
        "dt",
        "solver",
        "integrator",
        "octree_theta",
        "octree_softening",
        "frontend_particle_cap",
        "ui_fps_limit",
        "export_format",
        "input_format",
        "init_config_style",
        "preset_structure",
        "init_mode",
        "sph_enabled",
    }
    missing = sorted(required_keys - values.keys())
    if missing:
        print(f"error: missing required keys: {', '.join(missing)}", file=sys.stderr)
        return 1

    errors: list[str] = []
    errors.extend(expect_int(values, "particle_count", minimum=1))
    errors.extend(expect_int(values, "frontend_particle_cap", minimum=1))
    errors.extend(expect_int(values, "ui_fps_limit", minimum=1))
    errors.extend(expect_int(values, "energy_measure_every_steps", minimum=1))
    errors.extend(expect_int(values, "energy_sample_limit", minimum=1))

    errors.extend(expect_float(values, "dt", minimum=0.0))
    errors.extend(expect_float(values, "octree_theta", minimum=0.0))
    errors.extend(expect_float(values, "octree_softening", minimum=0.0))
    errors.extend(expect_float(values, "default_zoom", minimum=0.0))
    errors.extend(expect_float(values, "default_luminosity", minimum=0.0))

    errors.extend(expect_enum(values, "solver", {"pairwise_cuda", "octree_gpu", "octree_cpu"}))
    errors.extend(expect_enum(values, "integrator", {"euler", "rk4"}))
    errors.extend(expect_enum(values, "export_format", {"vtk", "vtk_binary", "xyz", "bin"}))
    errors.extend(expect_enum(values, "input_format", {"auto", "vtk", "vtk_binary", "xyz", "bin"}))
    errors.extend(expect_enum(values, "init_config_style", {"preset", "detailed"}))
    errors.extend(expect_enum(values, "preset_structure", {"disk_orbit", "random_cloud", "file"}))
    errors.extend(expect_enum(values, "init_mode", {"disk_orbit", "random_cloud", "file"}))
    errors.extend(expect_enum(values, "sph_enabled", {"true", "false"}))

    if errors:
        print("error: simulation.ini validation failed:", file=sys.stderr)
        for err in errors:
            print(f"  - {err}", file=sys.stderr)
        return 1

    print(f"simulation.ini validation OK ({len(values)} keys parsed)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
