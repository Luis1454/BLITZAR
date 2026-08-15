# TreePM Model Selection

BLITZAR keeps the original TreePM paths and adds explicit model selection through the `treepm`
directive. The default `model=auto` preserves legacy files that only set `local_grid`.

| Model | PM field | Local path | Dense path |
| --- | --- | --- | --- |
| `pm_only` | enabled by cuFFT | disabled | none |
| `local_grid` | enabled by cuFFT | bounded direct correction | none |
| `tree` | enabled by cuFFT | none | existing global octree with cutoff |
| `hybrid` | enabled by cuFFT | sparse-cell direct correction | existing octree selected for dense cells |
| `exact_tree` | disabled | none | existing GPU octree without PM approximation |

The PM mesh always deposits all particles. `particle_limit` is retained only for compatibility with
older configuration files and is ignored by the physical solver: depositing a subset while evolving
all bodies violates mass conservation. `dense_cell_threshold` controls the hybrid dispatch threshold.

The CUDA PM field uses cuFFT with a zero-padded 3D domain, a Gaussian long-range Green function,
matched Gaussian-complement short-range forces, assignment-window compensation, and persistent
R2C/C2R plans. CPU and CUDA both solve one potential and derive its centered mesh gradient. The
effective runtime log prints `solver=fft`. The red-black finite-difference solver remains a fallback and prints
`solver=red_black` if cuFFT cannot initialize or execute.

The CPU path uses the same zero-padded mesh, assignment deposit/interpolation, Gaussian spectral
split, and deterministic FFT operator implemented with an internal radix-2 3D transform. Its runtime log
prints `solver=cpu_fft_fp32` or `solver=cpu_fft_fp64`; FFT buffers are retained by the
`ParticleSystem` instance. `precision=fp64` promotes the full CPU PM field path, including the
mesh, spectral kernel, FFT and interpolation, to `double`. CUDA remains explicitly `fp32` and
reports both the effective and requested precision. This keeps the CPU and CUDA TreePM models
comparable without requiring FFTW or a server process.

Presets are available as `pm_only`, `local_grid_fast`, `hybrid_balanced`, `hybrid_quality`, and
`tree_quality`. The quality tree preset selects `exact_tree`, while the PM models remain available
for explicitly approximate runs. A preset is applied first, so later fields on the same directive
override it:

```ini
treepm(preset=hybrid_balanced, assignment=tsc, particle_limit=2000000, dense_cell_threshold=48)
```

The same choices are exposed through `--treepm-preset`, `--treepm-model`, `--treepm-precision`,
`--treepm-assignment`, `--treepm-particle-limit`, and `--treepm-dense-cell-threshold`. The effective configuration printed
by `--inspect` is the source of truth for the selected case.

`treepm_assignment` controls both mesh deposition and field interpolation. `cic` is the fastest
8-point stencil, `tsc` uses a smoother 27-point stencil, and `pcs` uses a cubic 64-point stencil.
CPU and CUDA use the same weights; the selected assignment is printed in the effective
configuration and runtime marker.

In the Qt client, the `TreePM assignment` selector exposes the same three values. Changing it
updates the local configuration and sends `set_treepm_assignment` to the live server; the server
rebuilds the next run with the selected stencil. The CLI and GUI therefore use the same canonical
configuration path rather than separate implementations.

`solver=octree_cpu` executes the CPU TreePM operator for every PM model. `exact_tree` remains a
quality path, not a PM solve: it selects the existing octree directly and is therefore the
reproducible high-accuracy baseline on either backend. On CPU, combining `exact_tree` with
`precision=fp64` selects a double-accumulation pairwise reference operator. It is intended for
validation and server runs, not for large-N throughput: its cost is quadratic and its explicit
runtime marker is `solver=cpu_fp64_pairwise`.

The PM `fp64` mode improves arithmetic stability but does not remove CIC/mesh discretization
error. The audit must therefore report PM approximation error separately from the exact FP64
reference error.

## Reproducible Audit

The performance and force audit runs every model from the same deterministic generated state. It
uses `pairwise_cuda` as the one-step force reference and writes a versioned JSON report:

```powershell
python scripts/benchmark_treepm.py `
  --exe .\build-batch-audit\cpu-fix\blitzar-headless.exe `
  --config .\simulation.ini `
  --particle-count 2048 `
  --steps 20 `
  --output .\artifacts\treepm-audit.json
```

The report includes initialization/integration/export time, particle updates per second, peak
reported memory, fault status, non-finite force vectors, RMS and maximum force error, and relative
RMS error against the pairwise reference. The audit exports the acceleration vector explicitly as
`VECTORS acceleration float` in ASCII VTK snapshots.

The command exits non-zero when a non-reference model exceeds `25%` relative RMS force error.
This is intentional: a completed kernel run is not a precision pass. Use
`--max-relative-error` only when the scientific protocol defines another tolerance, and keep the
raw JSON report with the case provenance.
