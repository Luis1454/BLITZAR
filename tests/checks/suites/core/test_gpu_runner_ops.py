from __future__ import annotations

import json
from collections.abc import Mapping
from pathlib import Path
from typing import Any, Literal, cast

from python_tools.ci.gpu_runner_bootstrap import WindowsGpuRunnerBootstrap
from python_tools.ci.gpu_runner_inventory import GitHubGpuRunnerInventory


class _Response:
    def __init__(self, payload: Mapping[str, object]) -> None:
        self._payload = json.dumps(payload).encode("utf-8")

    def read(self) -> bytes:
        return self._payload

    def __enter__(self) -> _Response:
        return self

    def __exit__(self, exc_type, exc, tb) -> Literal[False]:
        return False


def test_gpu_runner_inventory_reports_disabled_lane() -> None:
    report = GitHubGpuRunnerInventory().collect(
        repo="owner/repo",
        labels=["self-hosted", "cuda"],
        enabled=False,
        min_online=1,
        min_idle=1,
    )

    assert report["monitoring"] == "disabled"
    assert report["ready"] is False
    assert "disabled" in str(report["reason"]).lower()


def test_gpu_runner_inventory_reports_degraded_without_token() -> None:
    report = GitHubGpuRunnerInventory().collect(
        repo="owner/repo",
        labels=["self-hosted", "cuda"],
        enabled=True,
        min_online=1,
        min_idle=1,
        token="",
    )

    assert report["monitoring"] == "degraded"
    assert report["ready"] is True
    assert "falling back" in str(report["reason"]).lower()


def test_gpu_runner_inventory_filters_matching_idle_runner() -> None:
    payload = {
        "runners": [
            {
                "name": "gpu-01",
                "status": "online",
                "busy": False,
                "labels": [{"name": "self-hosted"}, {"name": "windows"}, {"name": "x64"}, {"name": "cuda"}],
            },
            {
                "name": "cpu-01",
                "status": "online",
                "busy": False,
                "labels": [{"name": "self-hosted"}, {"name": "linux"}],
            },
        ]
    }
    collector = GitHubGpuRunnerInventory(urlopen=lambda _: _Response(payload))

    report = collector.collect(
        repo="owner/repo",
        labels=["self-hosted", "windows", "x64", "cuda"],
        enabled=True,
        min_online=1,
        min_idle=1,
        token="token",
    )

    assert report["monitoring"] == "active"
    assert report["ready"] is True
    capacity = cast(dict[str, Any], report["capacity"])
    assert capacity["matched"] == 1
    assert capacity["idle"] == 1


def test_gpu_runner_bootstrap_plan_requires_runner_files(tmp_path: Path) -> None:
    bootstrap = WindowsGpuRunnerBootstrap()

    try:
        bootstrap.plan(
            repo="owner/repo",
            runner_root=tmp_path,
            runner_name="gpu-runner-01",
            token_env="GPU_RUNNER_REG_TOKEN",
            labels=["self-hosted", "windows", "x64", "cuda"],
        )
    except FileNotFoundError as exc:
        assert "config.cmd" in str(exc)
    else:
        raise AssertionError("expected missing runner files to fail")


def test_gpu_runner_bootstrap_emits_commands(tmp_path: Path, monkeypatch) -> None:
    for file_name in ("config.cmd", "run.cmd", "svc.cmd"):
        (tmp_path / file_name).write_text("echo ok\n", encoding="utf-8")
    for tool in WindowsGpuRunnerBootstrap.REQUIRED_TOOLS:
        monkeypatch.setattr("shutil.which", lambda name, _tool=tool: "tool.exe")
    bootstrap = WindowsGpuRunnerBootstrap()

    plan = bootstrap.plan(
        repo="owner/repo",
        runner_root=tmp_path,
        runner_name="gpu-runner-01",
        token_env="GPU_RUNNER_REG_TOKEN",
        labels=["self-hosted", "windows", "x64", "cuda"],
    )

    commands = cast(dict[str, list[str]], plan["commands"])
    configure = commands["configure"]
    assert "--unattended" in configure
    assert "gpu-runner-01" in configure
    assert "%GPU_RUNNER_REG_TOKEN%" in configure
