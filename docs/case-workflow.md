# BLITZAR Case Workflow

BLITZAR treats a simulation as a case directory. The configuration, generated data, execution log,
and provenance manifest remain together so a result can be inspected or rerun without guessing what
was loaded.

## 1. Create A Case

From the repository root, create a case from the checked-in reference configuration:

```bash
python scripts/blitzar_case.py init cases/disk-orbit
```

The layout is:

```text
cases/disk-orbit/
  simulation.ini
  outputs/
  logs/
  manifests/
```

Edit `simulation.ini` to define the case. The workflow never edits it during a run unless an explicit
headless option such as `--save-config` is passed through after `--`.

## 2. Resolve The Case

Inspect the effective configuration and the initial-state source before starting a solver:

```bash
python scripts/blitzar_case.py inspect --case cases/disk-orbit --deterministic
```

The output identifies the absolute configuration path, selected profile, generated or file-backed
initial state, seed, solver, and effective directive configuration. No server is started.

## 3. Validate Before Compute

Run the scenario validation gate:

```bash
python scripts/blitzar_case.py validate --case cases/disk-orbit
```

The command returns `0` for a runnable case and `4` when validation blocks execution. The transcript
is stored in `cases/disk-orbit/logs/validate.log`.

## 4. Execute A Controlled Run

The solver starts only in the `run` action. The step count, solver, integrator, determinism mode, and
output format are explicit:

```bash
python scripts/blitzar_case.py run \
  --case cases/disk-orbit \
  --steps 1000 \
  --particle-count 4096 \
  --init-seed 42 \
  --solver octree_cpu \
  --integrator leapfrog \
  --deterministic \
  --format vtk \
  --output final.vtk
```

The output is written to `outputs/final.vtk`. The log and manifest are written to `logs/run.log` and
`manifests/run.json`. The manifest records the command, configuration hash, return code, and output
hash.

## 5. Run Through Docker

The same case workflow can use the reproducible CPU image without changing the case contract:

```bash
docker build -f Dockerfile.cpu -t blitzar-cpu:local .
python scripts/blitzar_case.py inspect --case cases/disk-orbit --runtime docker
python scripts/blitzar_case.py validate --case cases/disk-orbit --runtime docker
python scripts/blitzar_case.py run \
  --case cases/disk-orbit \
  --runtime docker \
  --steps 1000 \
  --solver octree_cpu \
  --deterministic \
  --output final.xyz
```

The wrapper mounts the repository at `/workspace`; case paths and exported files therefore remain
visible on the host. Docker cases must be inside the repository tree.

## 6. Inspect Provenance

After a run, inspect the recorded execution contract and verify the current output hash:

```bash
python scripts/blitzar_case.py report --case cases/disk-orbit
```

This is the BLITZAR equivalent of a post-processing handoff: it reports what was executed and whether
the declared output still matches the recorded artifact.
