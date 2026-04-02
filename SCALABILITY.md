# SCALABILITY

## Auto-Tuning Summary

Policy applied in ParticleSystem:
- `leaf_capacity` default is `128`.
- It increases to `4096` only when VRAM estimate exceeds `6.5 GiB` (`6656 MiB`).

## Scalability Table (1M to 15M)

| N (Particles) | VRAM est. (MiB) | VRAM reelle (MiB) | Step Time (s) | Status |
| ---: | ---: | ---: | ---: | --- |
| 1,000,000 | 279 | 577 | 2.00 | PROJECTED |
| 2,000,000 | 558 | 841 | 2.01 | OBSERVED (leaf=auto) |
| 3,000,000 | 837 | 1,142 | 2.01 | PROJECTED |
| 4,000,000 | 1,115 | 1,442 | 2.00 | PROJECTED |
| 5,000,000 | 1,434 | 1,743 | 2.00 | OBSERVED (leaf=auto) |
| 6,000,000 | 1,673 | 1,906 | 2.00 | PROJECTED |
| 7,000,000 | 1,952 | 2,070 | 2.00 | PROJECTED |
| 8,000,000 | 2,231 | 2,233 | 2.00 | PROJECTED |
| 9,000,000 | 2,510 | 2,397 | 2.00 | PROJECTED |
| 10,000,000 | 2,789 | 2,560 | 2.01 | OBSERVED (leaf=auto) |
| 11,000,000 | 3,067 | 2,971 | 2.01 | PROJECTED |
| 12,000,000 | 3,346 | 3,382 | 2.01 | PROJECTED |
| 13,000,000 | 3,625 | 3,794 | 2.00 | PROJECTED |
| 14,000,000 | 3,904 | 4,205 | 2.00 | PROJECTED |
| 15,000,000 | 4,183 | 4,616 | 2.00 | OBSERVED (leaf=auto) |

## Method

- Observed rows come from `build-msvc-dev/stress-logs/scalability_report.json` (`leaf=auto` runs).
- `Step Time` is derived as `runtime_s / 120` on observed runs.
- Missing Ns are piecewise-linear projections between observed points.
- VRAM estimate uses the in-engine memory estimation trend and keeps the same order of magnitude as observed runs.

## Thermal Note

Thermal throttling risk threshold is set at **85C**. Sustained operation at or above this temperature can inflate step time and invalidate comparability across runs. Use active cooling or cap load to keep GPU below 85C during qualification runs.
