"""Fourier-space diagnostics for particle-density structure formation."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import TypedDict

import numpy as np


@dataclass(frozen=True)
class Snapshot:
    positions: np.ndarray
    masses: np.ndarray


class Spectrum(TypedDict):
    k: list[float]
    power: list[float]
    peak_k: float
    peak_power: float
    nonzero_power: float


class StructureAnalysis(TypedDict):
    path: str
    particle_count: int
    grid_size: int
    box_half_extent: float
    assignment: str
    window: str
    density_mean: float
    delta_rms: float
    spectrum: Spectrum


def _read_values(stream, count: int) -> np.ndarray:
    values = np.empty(count, dtype=np.float64)
    offset = 0
    while offset < count:
        line = stream.readline()
        if not line:
            raise ValueError("unexpected end of VTK payload")
        row = np.fromstring(line, sep=" ", dtype=np.float64)
        if row.size == 0:
            continue
        length = min(row.size, count - offset)
        values[offset : offset + length] = row[:length]
        offset += length
    return values


def _load_ascii_vtk(path: Path) -> Snapshot:
    positions: np.ndarray | None = None
    masses: np.ndarray | None = None
    with path.open("r", encoding="ascii") as stream:
        for raw_line in stream:
            parts = raw_line.split()
            if not parts:
                continue
            if parts[0] == "POINTS":
                count = int(parts[1])
                positions = _read_values(stream, count * 3).reshape(count, 3)
            elif parts[0] == "VERTICES":
                _read_values(stream, int(parts[1]) * 2)
            elif parts[0] == "SCALARS":
                if positions is None:
                    raise ValueError("VTK scalar data precedes points")
                name = parts[1].lower()
                components = int(parts[3]) if len(parts) > 3 else 1
                stream.readline()
                payload = _read_values(stream, int(positions.shape[0]) * components)
                if name == "mass" and components == 1:
                    masses = payload
            elif parts[0] == "VECTORS":
                if positions is None:
                    raise ValueError("VTK vector data precedes points")
                _read_values(stream, int(positions.shape[0]) * 3)
    if positions is None or positions.shape[0] < 2:
        raise ValueError(f"VTK snapshot has no usable points: {path}")
    if masses is None:
        masses = np.ones(positions.shape[0], dtype=np.float64)
    if masses.shape[0] != positions.shape[0]:
        raise ValueError(f"mass count does not match point count in {path}")
    return Snapshot(positions, masses)


def _load_binary_snapshot(path: Path) -> Snapshot:
    header = np.fromfile(path, dtype=np.uint8, count=16)
    if header.size != 16 or bytes(header[:8]) != b"NBSIMBIN":
        raise ValueError(f"unsupported binary snapshot: {path}")
    version = int(np.frombuffer(header[8:12].tobytes(), dtype="<u4")[0])
    count = int(np.frombuffer(header[12:16].tobytes(), dtype="<u4")[0])
    if version != 1 or count < 2:
        raise ValueError(f"invalid binary snapshot header: {path}")
    dtype = np.dtype([("position", "<f4", (3,)), ("velocity", "<f4", (3,)),
                      ("mass", "<f4"), ("temperature", "<f4")])
    records = np.fromfile(path, dtype=dtype, offset=16, count=count)
    if records.shape[0] != count:
        raise ValueError(f"truncated binary snapshot: {path}")
    return Snapshot(records["position"].astype(np.float64), records["mass"].astype(np.float64))


def load_snapshot(path: Path) -> Snapshot:
    """Load BLITZAR ASCII VTK or native ``.bin`` snapshots."""
    if path.suffix.lower() == ".bin":
        return _load_binary_snapshot(path)
    with path.open("rb") as stream:
        header = stream.read(256)
    if b"\nBINARY\n" in header:
        raise ValueError("binary VTK is not supported; export format=bin or ASCII VTK")
    return _load_ascii_vtk(path)


def _deposit_ngp(positions: np.ndarray, masses: np.ndarray, grid_size: int,
                 box_half_extent: float) -> np.ndarray:
    scaled = (positions + box_half_extent) * grid_size / (2.0 * box_half_extent)
    indices = np.floor(scaled).astype(np.int64)
    valid = np.all((indices >= 0) & (indices < grid_size), axis=1)
    flat = ((indices[:, 0] * grid_size + indices[:, 1]) * grid_size + indices[:, 2])
    grid = np.zeros(grid_size**3, dtype=np.float64)
    np.add.at(grid, flat[valid], masses[valid])
    return grid.reshape((grid_size, grid_size, grid_size))


def _deposit_cic(positions: np.ndarray, masses: np.ndarray, grid_size: int,
                 box_half_extent: float) -> np.ndarray:
    scaled = (positions + box_half_extent) * grid_size / (2.0 * box_half_extent)
    base = np.floor(scaled).astype(np.int64)
    fraction = scaled - base
    valid = np.all((base >= 0) & (base < grid_size), axis=1)
    base = np.minimum(np.maximum(base, 0), grid_size - 1)
    grid = np.zeros(grid_size**3, dtype=np.float64)
    flat_grid = grid
    for ox in (0, 1):
        for oy in (0, 1):
            for oz in (0, 1):
                index = base + np.array([ox, oy, oz], dtype=np.int64)
                inside = valid & np.all(index < grid_size, axis=1)
                weight = ((fraction[:, 0] if ox else 1.0 - fraction[:, 0]) *
                          (fraction[:, 1] if oy else 1.0 - fraction[:, 1]) *
                          (fraction[:, 2] if oz else 1.0 - fraction[:, 2]))
                flat = ((index[:, 0] * grid_size + index[:, 1]) * grid_size + index[:, 2])
                np.add.at(flat_grid, flat[inside], masses[inside] * weight[inside])
    return grid.reshape((grid_size, grid_size, grid_size))


def _radial_spectrum(delta: np.ndarray, box_half_extent: float, bins: int) -> Spectrum:
    size = delta.shape[0]
    cell = 2.0 * box_half_extent / size
    transformed = np.fft.rfftn(delta)
    power = np.abs(transformed) ** 2 / delta.size
    axes = [np.fft.fftfreq(size, d=cell), np.fft.fftfreq(size, d=cell),
            np.fft.rfftfreq(size, d=cell)]
    kx, ky, kz = np.meshgrid(*(2.0 * np.pi * axis for axis in axes), indexing="ij")
    magnitude = np.sqrt(kx * kx + ky * ky + kz * kz)
    nonzero = magnitude > 0.0
    k_values = magnitude[nonzero]
    power_values = power[nonzero]
    edges = np.geomspace(max(k_values.min(), 1.0e-12), k_values.max(), bins + 1)
    bucket = np.clip(np.digitize(k_values, edges) - 1, 0, bins - 1)
    counts = np.bincount(bucket, minlength=bins)
    sums = np.bincount(bucket, weights=power_values, minlength=bins)
    means = np.divide(sums, counts, out=np.zeros(bins), where=counts > 0)
    centers = np.sqrt(edges[:-1] * edges[1:])
    peak = int(np.argmax(means))
    return {"k": centers.tolist(), "power": means.tolist(), "peak_k": float(centers[peak]),
            "peak_power": float(means[peak]), "nonzero_power": float(power_values.sum())}


def analyze_snapshot(path: Path, grid_size: int = 64, box_half_extent: float | None = None,
                     assignment: str = "cic", window: str = "hann", bins: int = 24) -> StructureAnalysis:
    snapshot = load_snapshot(path)
    extent = box_half_extent or float(np.max(np.abs(snapshot.positions)) * 1.001)
    extent = max(extent, 1.0e-12)
    if assignment == "cic":
        density = _deposit_cic(snapshot.positions, snapshot.masses, grid_size, extent)
    elif assignment == "ngp":
        density = _deposit_ngp(snapshot.positions, snapshot.masses, grid_size, extent)
    else:
        raise ValueError("assignment must be cic or ngp")
    mean_density = float(density.mean())
    if mean_density <= 0.0:
        raise ValueError("density grid is empty")
    delta = density / mean_density - 1.0
    delta_rms = float(np.sqrt(np.mean(delta * delta)))
    if window == "hann":
        taper = np.hanning(grid_size)
        delta = delta * taper[:, None, None] * taper[None, :, None] * taper[None, None, :]
    elif window != "none":
        raise ValueError("window must be hann or none")
    spectrum = _radial_spectrum(delta, extent, bins)
    return {"path": str(path), "particle_count": int(snapshot.positions.shape[0]),
            "grid_size": grid_size, "box_half_extent": extent, "assignment": assignment,
            "window": window, "density_mean": mean_density, "delta_rms": delta_rms,
            "spectrum": spectrum}


def compare_structure(initial: StructureAnalysis, final: StructureAnalysis) -> dict[str, float]:
    initial_rms = max(float(initial["delta_rms"]), 1.0e-12)
    initial_power = max(float(initial["spectrum"]["nonzero_power"]), 1.0e-12)
    final_rms = float(final["delta_rms"])
    final_power = float(final["spectrum"]["nonzero_power"])
    return {"delta_rms_growth": final_rms / initial_rms,
            "nonzero_power_growth": final_power / initial_power,
            "final_delta_rms": final_rms,
            "final_peak_k": float(final["spectrum"]["peak_k"]),
            "final_peak_power": float(final["spectrum"]["peak_power"])}
