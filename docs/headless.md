# Headless Workflow

The headless executable has three explicit commands. It never starts a calculation when no command
is supplied.

## Inspect

`--inspect` loads the case and prints the resolved configuration and initial-state plan. It does not
create a missing configuration file and does not start a server.

```bash
blitzar-headless --inspect --config cases/disk/simulation.ini
```

## Validate

`--validate` performs inspection plus scenario validation. Exit code `0` means the case is valid for
execution; exit code `4` means validation blocked execution.

This is not a simulation result: it does not advance a particle. Only `--run` proves that the
solver executed. A successful run prints `faulted=0` on its completion line and returns exit code
`0`.

```bash
blitzar-headless --validate --config cases/disk/simulation.ini
```

## Run

`--run` is the only command that starts the solver. Use an explicit output path for reproducible
artifacts instead of the timestamped fallback.

The headless runtime executes the `ParticleSystem` synchronously through the batch runner. It does
not start `SimulationServer`, create a command thread, publish GUI snapshots, or poll server state.
The GUI and daemon retain that interactive server adapter. A batch run therefore measures setup,
solver execution, explicit host synchronization for export, and export only.

The coarse-grained `densityNorm` visualization field is published by the server snapshot path; it
is intentionally not added to the physical particle mass or to the headless export state.

```bash
blitzar-headless --run \
  --config cases/disk/simulation.ini \
  --target-steps 1000 \
  --deterministic true \
  --export-format xyz \
  --export-on-exit true \
  --export-path cases/disk/outputs/final.xyz
```

Before the first step, the executable prints:

- the absolute configuration file path;
- whether the initial state is generated or loaded from a file;
- the selected profile, solver, integrator, seed, and effective configuration;
- the deterministic export path when one is supplied.

The completion line records `steps`, `simulated_time`, and `time_ms`. Physical duration is
`target_steps * dt`; substeps improve integration stability but do not change that duration.

Configuration files are not created implicitly by the headless executable. A missing file is an
error and returns exit code `3`.

## Individual Adaptive Time Steps

Adaptive stepping is opt-in. It uses a dyadic hierarchy inside each global `dt`: level `0` is the
slowest cadence (`dt`) and `adaptive_max_level` is the fastest cadence (`dt / 2^max_level`). Force updates use a synchronized
predictor-corrector step, so inactive bodies remain extrapolated to the current global boundary.

```bash
blitzar-headless --run --config cases/disk/simulation.ini \
  --adaptive-time-steps true \
  --adaptive-max-level 6 \
  --adaptive-eta 0.25 \
  --target-steps 1000
```

The same settings can be persisted as:

```text
adaptive(enabled=true, max_level=6, eta=0.25)
```

Use `--adaptive-time-steps false` to restore the original global-step path. The headless log prints
the effective adaptive settings before execution. The CUDA build currently uses the validated CPU
reference scheduler for this mode; this is explicit in the runtime log and avoids silently claiming
GPU adaptive scheduling before its device-side state is qualified.
