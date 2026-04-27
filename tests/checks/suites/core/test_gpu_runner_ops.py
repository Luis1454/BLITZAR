# @file tests/checks/suites/core/test_gpu_runner_ops.py
# @author Luis1454
# @project BLITZAR
# @brief Automated verification assets for BLITZAR quality gates.

from __future__ import annotations

import json
from collections.abc import Mapping
from pathlib import Path
from typing import Any, Literal, cast

from python_tools.ci.gpu_runner_bootstrap import WindowsGpuRunnerBootstrap
from python_tools.ci.gpu_runner_inventory import GitHubGpuRunnerInventory


# @brief Defines the response type contract.
# @param None This contract does not take explicit parameters.
# @note Keep construction and side effects explicit for deterministic quality gates.
class _Response:
    # @brief Documents the init operation contract.
    # @param payload Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def __init__(self, payload: Mapping[str, object]) -> None:
        self._payload = json.dumps(payload).encode("utf-8")

    # @brief Documents the read operation contract.
    # @param None This contract does not take explicit parameters.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def read(self) -> bytes:
        return self._payload

    # @brief Documents the enter operation contract.
    # @param None This contract does not take explicit parameters.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def __enter__(self) -> _Response:
        return self

    # @brief Documents the exit operation contract.
    # @param exc_type Input value used by this contract.
    # @param exc Input value used by this contract.
    # @param tb Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def __exit__(self, exc_type, exc, tb) -> Literal[False]:
        return False


# @brief Documents the test gpu runner inventory reports disabled lane operation contract.
# @param None This contract does not take explicit parameters.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
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


# @brief Documents the test gpu runner inventory reports degraded without token operation contract.
# @param None This contract does not take explicit parameters.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
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


# @brief Documents the test gpu runner inventory filters matching idle runner operation contract.
# @param None This contract does not take explicit parameters.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
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


# @brief Documents the test gpu runner bootstrap plan requires runner files operation contract.
# @param tmp_path Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
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


# @brief Documents the test gpu runner bootstrap emits commands operation contract.
# @param tmp_path Input value used by this contract.
# @param monkeypatch Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
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
