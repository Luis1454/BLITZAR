# Release Evidence

The scaling runner writes generated evidence outside the source tree. Use the
following command from the repository root after building
`blitzar_scaling_test`:

```text
python tools/release_evidence.py --root . --build-dir ../.blitzar-build/evidence --output ../.blitzar-evidence/run
```

The output directory contains the exact metadata, one raw log per command,
machine-readable results, and a Markdown summary. The summary must distinguish
CPU, MPI local-rank, multi-node, HIP compile, HIP fallback, and HIP device
execution. Missing hardware or launchers produce `skipped` or `unknown`, not a
passing performance claim.
