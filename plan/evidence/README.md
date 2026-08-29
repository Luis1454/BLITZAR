# Release Evidence

The scaling runner writes generated evidence outside the source tree. Use the
following command from the repository root after building
`blitzar_scaling_test`:

```text
python -m tools.evidence.release_evidence --root . --build-dir ../.blitzar-build/evidence --output ../.blitzar-evidence/run --strict
```

The output directory contains the exact metadata, one raw log per command,
machine-readable results, and a Markdown summary. The summary must distinguish
CPU, MPI local-rank, multi-node, HIP compile, HIP fallback, and HIP device
execution. Each result also contains the v2 wall/per-step timing, allocation,
memory, throughput, distribution, and oracle fields. Missing hardware or
launchers produce `skipped` or `unknown`, not a passing performance claim.

The separate layout qualification uses the same external-artifact rule:

```text
python -m tools.evidence.layout_evidence --root . --build-dir ../.blitzar-build/evidence --output ../.blitzar-evidence/layout --strict
```

It validates the complete Morton ordering and bounded SoA/AoSoA matrix from
`plan/layout.json`. Its cache-line and scan metrics are deterministic proxies;
they do not claim hardware performance-counter measurements.

The compensated-reduction qualification uses the same external-artifact rule:

```text
python -m tools.evidence.reduction_evidence --root . --build-dir ../.blitzar-build/evidence --output ../.blitzar-evidence/reduction --strict
```

It validates the Plain/Kahan/Neumaier matrix and the 4096-step KDK
conservation run from `plan/reduction.json`. Vectorization fields identify
ordered-loop eligibility; they do not claim hardware counter measurements.
