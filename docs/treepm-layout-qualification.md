# TreePM Layout Qualification

The reproducible runner is `scripts/benchmark_treepm_layout.py`. It compares:

- `linear`: CUB radix sort without gather;
- `gather_linear`: linear cell order with sorted position/mass buffers;
- `gather_morton`: Morton order with sorted position/mass buffers.

Every case uses a fixed seed and verifies the particle count, step count and
`faulted=0` marker from the headless executable. When available, the runner
records `nvidia-smi` snapshots before and after each case with P-state,
temperature, clocks, utilization, memory and power draw.

The runner checkpoints after every case. Add `--resume` to reuse only completed
successful cases from an existing JSON file; interrupted or failed cases are
rerun. Progress is emitted as `[matrix] start=i/n` and `[matrix] done=i/n`.

## Qualification commands

Fast smoke matrix:

```powershell
python scripts/benchmark_treepm_layout.py `
  --binary build-gui-release/blitzar-headless.exe `
  --sizes 100000 `
  --steps 10
```

Production-size matrix:

```powershell
python scripts/benchmark_treepm_layout.py `
  --binary build-gui-release/blitzar-headless.exe `
  --sizes 100000 1000000 5000000 10000000 `
  --steps 1000 `
  --timeout-s 3600
```

The large matrix is intentionally explicit because the 5M and 10M cases can
consume substantial VRAM and wall time. Nsight Compute counters for L2 hit rate,
memory throughput and SM activity require performance-counter permission on the
GPU; a missing permission is an instrumentation limitation, not a simulation
failure.

Automatic dispatch is the default when no layout override is present. It
performs one GPU radial mass histogram, computes `R80/Rbbox`, then caches either
`linear` or `gather_morton` for the rest of the run. It can also be selected
explicitly as a fourth layout:

```powershell
python scripts/benchmark_treepm_layout.py `
  --binary build-gui-release/blitzar-headless.exe `
  --layouts auto --sizes 100000 1000000 --steps 10
```

The default threshold is `0.35` and can be overridden with
`BLITZAR_TREEPM_AUTO_R80_THRESHOLD`. Explicit legacy flags remain available;
`BLITZAR_TREEPM_LAYOUT` takes precedence when set to `linear`,
`gather_linear`, `gather_morton` or `auto`.

## Nsight Compute protocol

Use a static CUDA path for counter collection. JIT module replay and CUDA Graph
replay are separate experiments and are not mixed with hardware-counter runs:

```powershell
$env:BLITZAR_CUDA_JIT = "0"
$env:BLITZAR_TREEPM_GRAPH = "0"
$env:BLITZAR_TREEPM_GATHER = "1"
$env:BLITZAR_TREEPM_MORTON = "1"
ncu --csv --page raw `
  --kernel-name 'regex:.*updateParticlesTreePmLocalGridKernel.*' `
  --launch-count 1 `
  --metrics 'gpu__time_duration.sum,dram__bytes_read.sum,dram__bytes_write.sum,lts__t_sector_hit_rate.pct,sm__maximum_warps_per_active_cycle_pct' `
  --log-file artifacts/ncu-treepm-force.csv `
  -- build-gui-release/blitzar-headless.exe `
    --config tests/data/scene_cosmology_preview.ini --run `
    --target-steps 1 --particle-count 100000 --solver octree_gpu `
    --treepm-enabled true --treepm-model local_grid `
    --init-mode cosmology --cosmology-enabled true --init-seed 42 `
    --deterministic true
```

The morphology must be selected with `--init-mode`; `--preset-structure` alone
is ignored by a detailed configuration. The current 100k counter sample on the
RTX 4070 Laptop GPU used 40 registers/thread and reported 100% for
`sm__maximum_warps_per_active_cycle_pct` on the local-grid force kernel. The measured L2 sector hit rates were
99.23% linear versus 98.99% gather+Morton for cosmology, and 98.07% linear
versus 92.54% gather+Morton for `plummer_sphere`. These are kernel-level
measurements, not end-to-end speedups.

Nsight Compute currently terminates the process with Windows status
`0xC0000005` when replaying `treePmGatherSortedParticlesKernel` directly. The
normal headless run remains successful, and the force-kernel profiles above are
valid. This limitation must be resolved before publishing gather-kernel
bandwidth or a complete per-kernel profile.

## Current evidence

On the RTX 4070 Laptop GPU, the 100k smoke matrix completed all nine cases. At
1M particles and ten steps, gather+Morton was best for `random_cloud` and
`cosmology`, while `plummer_sphere` preferred the linear layout. Therefore no
single layout is promoted globally until the full scale and morphology matrix
is completed.

The 5M and 10M ten-step smoke scaling also completed all 18 cases with
`actual_particles` equal to the requested count and `faulted=0`. Integration
times in milliseconds were:

| Particles | Morphology | Linear | Gather linear | Gather Morton |
| ---: | --- | ---: | ---: | ---: |
| 5M | `random_cloud` | 15688 | 4701 | 2404 |
| 5M | `plummer_sphere` | 356 | 354 | 601 |
| 5M | `cosmology` | 14318 | 4359 | 2398 |
| 10M | `random_cloud` | 32956 | 11064 | 5421 |
| 10M | `plummer_sphere` | 640 | 655 | 1115 |
| 10M | `cosmology` | 15193 | 5392 | 5449 |

These are ten-step smoke measurements, not 1000-step production TTS. The
observed crossover is stable for the concentrated Plummer profile, where
linear remains preferable, while Morton is strongly favorable for the diffuse
random cloud and cosmology cases. The full 1000-step matrix remains pending.

At 100k particles, the automatic selector measured `R80/Rbbox=0.7188` for
`random_cloud` and selected `gather_morton`, measured `0.5625` for `cosmology`
and selected `gather_morton`, and measured `0.0312` for `plummer_sphere` and
selected `linear`. All three runs completed with `faulted=0`.
