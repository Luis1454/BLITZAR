# GPU Runner Operations

This note defines the self-hosted GPU runner baseline for optional CUDA evidence lanes.

## Required Labels

- `self-hosted`
- `windows`
- `x64`
- `cuda`

## Required Host Tools

- NVIDIA driver exposed through `nvidia-smi`
- CUDA Toolkit exposed through `nvcc`
- Python
- `cmake`
- `ninja`

## Bootstrap

Use the extracted `actions-runner` directory on the Windows host, then run:

```powershell
python scripts/ci/gpu/bootstrap_windows_runner.py `
  --repo Luis1454/BLITZAR `
  --runner-root C:\actions-runner `
  --runner-name gravity-gpu-01 `
  --output dist/gpu-runner/bootstrap-plan.json `
  --emit-script dist/gpu-runner/bootstrap-runner.ps1
```

The script validates host prerequisites, emits the exact `config.cmd` and `svc.cmd` commands, and can execute them with `--execute` when `GPU_RUNNER_REG_TOKEN` is present in the environment.

## Monitoring

- Workflow: `.github/workflows/gpu-runner-health.yml`
- Scheduled cadence: daily
- Inventory source: GitHub repository self-hosted runner API
- Secret: `GPU_RUNNER_ADMIN_TOKEN`

When the monitoring token is unavailable, the report degrades gracefully to static policy mode using `HAS_GPU_RUNNER` instead of failing the whole repository.

## Fallback Policy

- `nightly-full` and `release-lane` call the GPU readiness workflow before any self-hosted GPU job.
- If readiness is `false`, the CUDA lane is skipped explicitly and a hosted fallback job writes the reason to the workflow summary.
- If readiness is `true`, the existing GPU test jobs run unchanged on the self-hosted runner.

## Operational Expectations

- Keep at least one idle runner matching `self-hosted,windows,x64,cuda`.
- Review `gpu-runner-health-*` artifacts for capacity drift or repeated degraded monitoring.
- Treat runner image, driver, CUDA, or label changes as standards-impacting and update the quality baseline documents in the same change.
