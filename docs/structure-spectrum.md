# Structure Spectrum

BLITZAR can measure the formation of density structures without changing the
integrator. The analysis deposits particle mass on a cubic grid, computes the
three-dimensional real FFT of the density contrast, and averages the power into
radial `P(k)` shells. `cic` is the default assignment; `ngp` is available for a
cheaper diagnostic.

Install the pinned analysis dependency once in the Python environment:

```powershell
python -m pip install -r requirements-analysis.txt
```

## Analyze an Existing Run

Export an initial and final state with deterministic paths:

```powershell
& .\build-cpu-check\blitzar-headless.exe --run `
  --config .\tests\data\scene_cosmology_preview.ini `
  --target-steps 0 --deterministic true --export-format vtk `
  --export-on-exit true --export-path .\artifacts\spectrum\initial.vtk

& .\build-cpu-check\blitzar-headless.exe --run `
  --config .\tests\data\scene_cosmology_preview.ini `
  --target-steps 40 --deterministic true --export-format vtk `
  --export-on-exit true --export-path .\artifacts\spectrum\final.vtk

python scripts/analyze_structure.py `
  --initial .\artifacts\spectrum\initial.vtk `
  --input .\artifacts\spectrum\final.vtk `
  --box-half-extent 48 --grid-size 32 `
  --output .\artifacts\spectrum\report.json
```

The report contains `delta_rms`, the radial power spectrum, the dominant scale
`peak_k`, and growth ratios for the non-zero power and density variance. A
larger growth ratio means that the run amplified more structure; it is not by
itself proof of physical accuracy. Compare it with energy, convergence, and
resolution studies.

## GUI Correlation Spectrogram

The Qt client exposes the same density-contrast diagnostic in the `Structure
FFT` dock. Each received snapshot adds one horizontal spectrum row: old rows
advance from left to right, with the oldest snapshot at the left and the
newest at the right. The vertical axis is the spatial scale, from large scales
/ low `k` at the bottom to small scales / high `k` at the top. The GUI uses a
`64^3` density grid, CIC mass assignment and 32 radial shells for this
diagnostic.

The color scale is logarithmic over a 24 dB window. Dark blue means weak
correlation and a more homogeneous density field at that scale; orange means
stronger correlated structure. The displayed sample is the GUI draw sample,
so `GUI sample N` must not be interpreted as the total physical particle
count.

## Parameter Calibration

The calibrator runs each candidate from the same deterministic initial seed,
analyzes the initial and final snapshots, and writes both a complete report and
`structure-calibration-best.ini`. Its default objective maximizes spectral
growth, targets a final density RMS of `1`, and penalizes RMS values above `8`.
The candidate list is explicit and can be narrowed for a quick CPU run:

```powershell
python scripts/calibrate_structure.py `
  --exe .\build-cpu-check\blitzar-headless.exe `
  --config .\tests\data\scene_cosmology_preview.ini `
  --particle-count 1024 --steps 40 --grid-size 32 --max-runs 8 `
  --output .\artifacts\structure-calibration.json
```

The selected parameters are a reproducible numerical screening result, not a
cosmological fit. For a scientific claim, rerun the winning candidate at higher
particle count and grid resolution, then compare against a convergence baseline.
