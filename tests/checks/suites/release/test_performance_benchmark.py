# File: tests/checks/suites/release/test_performance_benchmark.py
# Purpose: Verification coverage for the BLITZAR quality gate.

from __future__ import annotations

import json
from pathlib import Path

from python_tools.ci.performance_benchmark import PerformanceBenchmarkCampaign


# Description: Executes the _write_campaign operation.
def _write_campaign(root: Path) -> None:
    payload = {
        "profiles": {
            "gpu-nightly": {
                "runs": [
                    {
                        "id": "disk-fast",
                        "workload": "disk_orbit",
                        "solver": "octree_gpu",
                        "integrator": "rk4",
                        "dt": 0.002,
                        "particle_count": 2048,
                        "steps": 120,
                        "seed": 12345,
                        "checks": {"min_steps_per_second": 30.0, "max_average_step_ms": 40.0},
                        "baseline": {"steps_per_second": 40.0},
                        "regressions": {"steps_per_second": 25.0},
                    },
                    {
                        "id": "cloud-steady",
                        "workload": "random_cloud",
                        "solver": "octree_cpu",
                        "integrator": "euler",
                        "dt": 0.004,
                        "particle_count": 4096,
                        "steps": 60,
                        "seed": 7,
                        "checks": {"min_steps_per_second": 8.0},
                    },
                ]
            }
        }
    }
    path = root / "docs/quality/manifest/performance_campaign.json"
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2), encoding="utf-8")


# Description: Executes the _runner operation.
def _runner(outputs: dict[tuple[str, ...], str]):
    # Description: Executes the run operation.
    def run(command: list[str]) -> str:
        key = tuple(command)
        if key not in outputs:
            raise RuntimeError(f"missing mock output for {' '.join(command)}")
        return outputs[key]

    return run


# Description: Executes the test_tst_unt_perf_001_performance_campaign_passes_when_baselines_hold operation.
def test_tst_unt_perf_001_performance_campaign_passes_when_baselines_hold(tmp_path: Path) -> None:
    _write_campaign(tmp_path)
    tool = tmp_path / "fake-tool"
    campaign = PerformanceBenchmarkCampaign(
        _runner(
            {
                (str(tool), "--workload", "disk_orbit", "--solver", "octree_gpu", "--integrator", "rk4", "--dt", "0.002", "--particle-count", "2048", "--steps", "120", "--seed", "12345"): "workload=disk_orbit\nsolver=octree_gpu\nintegrator=rk4\ndt=0.002\nparticle_count=2048\nsteps=120\nseed=12345\nwall_seconds=3.0\nsteps_per_second=40.0\nparticles_per_second=81920.0\naverage_step_ms=25.0\n",
                (str(tool), "--workload", "random_cloud", "--solver", "octree_cpu", "--integrator", "euler", "--dt", "0.004", "--particle-count", "4096", "--steps", "60", "--seed", "7"): "workload=random_cloud\nsolver=octree_cpu\nintegrator=euler\ndt=0.004\nparticle_count=4096\nsteps=60\nseed=7\nwall_seconds=6.0\nsteps_per_second=10.0\nparticles_per_second=40960.0\naverage_step_ms=100.0\n",
            }
        )
    )
    _, report = campaign.run(tmp_path, tmp_path / "dist/perf", "gpu-nightly", tool)
    assert report["status"] == "passed"
    assert report["failures"] == []


# Description: Executes the test_tst_unt_perf_002_performance_campaign_fails_on_threshold_and_regression operation.
def test_tst_unt_perf_002_performance_campaign_fails_on_threshold_and_regression(tmp_path: Path) -> None:
    _write_campaign(tmp_path)
    tool = tmp_path / "fake-tool"
    campaign = PerformanceBenchmarkCampaign(
        _runner(
            {
                (str(tool), "--workload", "disk_orbit", "--solver", "octree_gpu", "--integrator", "rk4", "--dt", "0.002", "--particle-count", "2048", "--steps", "120", "--seed", "12345"): "workload=disk_orbit\nsolver=octree_gpu\nintegrator=rk4\ndt=0.002\nparticle_count=2048\nsteps=120\nseed=12345\nwall_seconds=6.0\nsteps_per_second=20.0\nparticles_per_second=40960.0\naverage_step_ms=50.0\n",
                (str(tool), "--workload", "random_cloud", "--solver", "octree_cpu", "--integrator", "euler", "--dt", "0.004", "--particle-count", "4096", "--steps", "60", "--seed", "7"): "workload=random_cloud\nsolver=octree_cpu\nintegrator=euler\ndt=0.004\nparticle_count=4096\nsteps=60\nseed=7\nwall_seconds=10.0\nsteps_per_second=6.0\nparticles_per_second=24576.0\naverage_step_ms=166.0\n",
            }
        )
    )
    _, report = campaign.run(tmp_path, tmp_path / "dist/perf", "gpu-nightly", tool)
    assert report["status"] == "failed"
    assert isinstance(report["failures"], list)
    assert len(report["failures"]) == 4


# Description: Executes the test_tst_unt_perf_003_performance_campaign_writes_summary_archive operation.
def test_tst_unt_perf_003_performance_campaign_writes_summary_archive(tmp_path: Path) -> None:
    _write_campaign(tmp_path)
    tool = tmp_path / "fake-tool"
    campaign = PerformanceBenchmarkCampaign(
        _runner(
            {
                (str(tool), "--workload", "disk_orbit", "--solver", "octree_gpu", "--integrator", "rk4", "--dt", "0.002", "--particle-count", "2048", "--steps", "120", "--seed", "12345"): "workload=disk_orbit\nsolver=octree_gpu\nintegrator=rk4\ndt=0.002\nparticle_count=2048\nsteps=120\nseed=12345\nwall_seconds=3.0\nsteps_per_second=40.0\nparticles_per_second=81920.0\naverage_step_ms=25.0\n",
                (str(tool), "--workload", "random_cloud", "--solver", "octree_cpu", "--integrator", "euler", "--dt", "0.004", "--particle-count", "4096", "--steps", "60", "--seed", "7"): "workload=random_cloud\nsolver=octree_cpu\nintegrator=euler\ndt=0.004\nparticle_count=4096\nsteps=60\nseed=7\nwall_seconds=6.0\nsteps_per_second=10.0\nparticles_per_second=40960.0\naverage_step_ms=100.0\n",
            }
        )
    )
    archive, _ = campaign.run(tmp_path, tmp_path / "dist/perf", "gpu-nightly", tool)
    assert archive.exists()
    readme = (tmp_path / "dist/perf/README.md").read_text(encoding="utf-8")
    assert "# Performance Benchmark Report" in readme
    assert "| `disk-fast` | 40.00 | 25.000 |" in readme
