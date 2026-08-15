"""Regression tests for Fourier structure diagnostics."""

from __future__ import annotations

import json
from pathlib import Path

import numpy as np

from python_tools.analysis.structure_spectrum import analyze_snapshot, compare_structure, load_snapshot


def _write_vtk(path: Path, positions: np.ndarray, masses: np.ndarray) -> None:
    with path.open("w", encoding="ascii") as stream:
        stream.write("# vtk DataFile Version 3.0\nBLITZAR test\nASCII\nDATASET POLYDATA\n")
        stream.write(f"POINTS {len(positions)} float\n")
        for position in positions:
            stream.write(f"{position[0]:.8g} {position[1]:.8g} {position[2]:.8g}\n")
        stream.write(f"VERTICES {len(positions)} {len(positions) * 2}\n")
        for index in range(len(positions)):
            stream.write(f"1 {index}\n")
        stream.write(f"POINT_DATA {len(positions)}\nSCALARS mass float 1\nLOOKUP_TABLE default\n")
        for mass in masses:
            stream.write(f"{mass:.8g}\n")


def test_tst_unt_spectrum_001_fft_detects_non_uniform_density(tmp_path: Path) -> None:
    rng = np.random.default_rng(42)
    uniform = rng.uniform(-1.0, 1.0, size=(512, 3))
    clustered = np.concatenate((rng.normal(-0.45, 0.08, size=(256, 3)),
                                rng.normal(0.45, 0.08, size=(256, 3))))
    uniform_path = tmp_path / "uniform.vtk"
    clustered_path = tmp_path / "clustered.vtk"
    _write_vtk(uniform_path, uniform, np.ones(len(uniform)))
    _write_vtk(clustered_path, clustered, np.ones(len(clustered)))
    uniform_result = analyze_snapshot(uniform_path, grid_size=16, box_half_extent=1.0, window="none")
    clustered_result = analyze_snapshot(clustered_path, grid_size=16, box_half_extent=1.0, window="none")
    assert load_snapshot(clustered_path).positions.shape == (512, 3)
    assert clustered_result["spectrum"]["nonzero_power"] > uniform_result["spectrum"]["nonzero_power"]


def test_tst_unt_spectrum_002_growth_report_is_json_safe(tmp_path: Path) -> None:
    positions = np.linspace(-0.8, 0.8, 64, dtype=np.float64)[:, None] * np.array([1.0, 0.2, 0.1])
    path = tmp_path / "state.vtk"
    _write_vtk(path, positions, np.ones(len(positions)))
    initial = analyze_snapshot(path, grid_size=8, box_half_extent=1.0)
    final = analyze_snapshot(path, grid_size=8, box_half_extent=1.0)
    growth = compare_structure(initial, final)
    assert growth["delta_rms_growth"] == 1.0
    json.dumps({"initial": initial, "growth": growth}, allow_nan=False)
