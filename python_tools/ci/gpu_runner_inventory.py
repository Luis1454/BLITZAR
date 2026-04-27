#!/usr/bin/env python3
# @file python_tools/ci/gpu_runner_inventory.py
# @author Luis1454
# @project BLITZAR
# @brief Python quality and automation support for BLITZAR governance.

from __future__ import annotations

import json
import urllib.error
import urllib.request
from collections.abc import Callable, Iterable
from pathlib import Path
from typing import Protocol


# @brief Defines the response like type contract.
# @param None This contract does not take explicit parameters.
# @note Keep construction and side effects explicit for deterministic quality gates.
class ResponseLike(Protocol):
    # @brief Documents the read operation contract.
    # @param None This contract does not take explicit parameters.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def read(self) -> bytes:
        ...

    # @brief Documents the enter operation contract.
    # @param None This contract does not take explicit parameters.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def __enter__(self) -> ResponseLike:
        ...

    # @brief Documents the exit operation contract.
    # @param exc_type Input value used by this contract.
    # @param exc Input value used by this contract.
    # @param tb Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def __exit__(self, exc_type: object, exc: object, tb: object) -> bool | None:
        ...


UrlOpen = Callable[[urllib.request.Request], ResponseLike]


# @brief Documents the normalize labels operation contract.
# @param labels Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def _normalize_labels(labels: Iterable[str]) -> list[str]:
    return [label.strip() for label in labels if label.strip()]


# @brief Defines the git hub gpu runner inventory type contract.
# @param None This contract does not take explicit parameters.
# @note Keep construction and side effects explicit for deterministic quality gates.
class GitHubGpuRunnerInventory:
    # @brief Documents the init operation contract.
    # @param urlopen Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def __init__(self, urlopen: UrlOpen | None = None) -> None:
        self._urlopen = urlopen or urllib.request.urlopen

    # @brief Documents the collect operation contract.
    # @param repo Input value used by this contract.
    # @param labels Input value used by this contract.
    # @param enabled Input value used by this contract.
    # @param min_online Input value used by this contract.
    # @param min_idle Input value used by this contract.
    # @param token Input value used by this contract.
    # @param api_url Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def collect(
        self,
        repo: str,
        labels: Iterable[str],
        enabled: bool,
        min_online: int,
        min_idle: int,
        token: str = "",
        api_url: str = "https://api.github.com",
    ) -> dict[str, object]:
        required = _normalize_labels(labels)
        if not enabled:
            return self._report(
                repo=repo,
                labels=required,
                ready=False,
                reason="GPU lanes disabled by HAS_GPU_RUNNER policy.",
                monitoring="disabled",
                runners=[],
                min_online=min_online,
                min_idle=min_idle,
            )
        if not token.strip():
            return self._report(
                repo=repo,
                labels=required,
                ready=True,
                reason="GPU runner monitoring token missing; falling back to HAS_GPU_RUNNER static enablement.",
                monitoring="degraded",
                runners=[],
                min_online=min_online,
                min_idle=min_idle,
            )
        try:
            runners = self._fetch_runners(repo, token.strip(), api_url.rstrip("/"))
        except (RuntimeError, urllib.error.URLError) as exc:
            return self._report(
                repo=repo,
                labels=required,
                ready=False,
                reason=f"failed to query GPU runner inventory: {exc}",
                monitoring="error",
                runners=[],
                min_online=min_online,
                min_idle=min_idle,
            )
        matched = [runner for runner in runners if self._matches_required_labels(runner, required)]
        online = [runner for runner in matched if runner["status"] == "online"]
        idle = [runner for runner in online if not runner["busy"]]
        ready = len(online) >= min_online and len(idle) >= min_idle
        if ready:
            reason = f"GPU runner capacity available: {len(idle)} idle / {len(online)} online."
        else:
            reason = f"GPU runner capacity unavailable: {len(idle)} idle / {len(online)} online."
        return self._report(
            repo=repo,
            labels=required,
            ready=ready,
            reason=reason,
            monitoring="active",
            runners=matched,
            min_online=min_online,
            min_idle=min_idle,
        )

    # @brief Documents the write operation contract.
    # @param report Input value used by this contract.
    # @param output_path Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def write(self, report: dict[str, object], output_path: str) -> None:
        Path(output_path).parent.mkdir(parents=True, exist_ok=True)
        with open(output_path, "w", encoding="utf-8") as handle:
            json.dump(report, handle, indent=2)

    # @brief Documents the fetch runners operation contract.
    # @param repo Input value used by this contract.
    # @param token Input value used by this contract.
    # @param api_url Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _fetch_runners(self, repo: str, token: str, api_url: str) -> list[dict[str, object]]:
        request = urllib.request.Request(
            f"{api_url}/repos/{repo}/actions/runners?per_page=100",
            headers={
                "Accept": "application/vnd.github+json",
                "Authorization": f"Bearer {token}",
                "User-Agent": "gravity-gpu-runner-health",
            },
        )
        with self._urlopen(request) as response:
            payload: object = json.loads(response.read().decode("utf-8"))
        if not isinstance(payload, dict) or not isinstance(payload.get("runners"), list):
            raise RuntimeError("unexpected runner inventory payload")
        runners: list[dict[str, object]] = []
        for raw in payload["runners"]:
            if not isinstance(raw, dict):
                continue
            labels: list[str] = []
            raw_labels: object = raw.get("labels", [])
            if isinstance(raw_labels, list):
                labels = [str(item.get("name", "")) for item in raw_labels if isinstance(item, dict)]
            runners.append(
                {
                    "name": str(raw.get("name", "")),
                    "status": str(raw.get("status", "")),
                    "busy": bool(raw.get("busy", False)),
                    "labels": labels,
                }
            )
        return runners

    @staticmethod
    # @brief Documents the matches required labels operation contract.
    # @param runner Input value used by this contract.
    # @param required Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _matches_required_labels(runner: dict[str, object], required: list[str]) -> bool:
        if not required:
            return True
        raw_labels = runner.get("labels", [])
        labels = {str(label) for label in raw_labels} if isinstance(raw_labels, list) else set()
        return all(label in labels for label in required)

    @staticmethod
    # @brief Documents the report operation contract.
    # @param repo Input value used by this contract.
    # @param labels Input value used by this contract.
    # @param ready Input value used by this contract.
    # @param reason Input value used by this contract.
    # @param monitoring Input value used by this contract.
    # @param runners Input value used by this contract.
    # @param min_online Input value used by this contract.
    # @param min_idle Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _report(
        repo: str,
        labels: list[str],
        ready: bool,
        reason: str,
        monitoring: str,
        runners: list[dict[str, object]],
        min_online: int,
        min_idle: int,
    ) -> dict[str, object]:
        online = [runner for runner in runners if runner["status"] == "online"]
        idle = [runner for runner in online if not runner["busy"]]
        busy = [runner for runner in online if runner["busy"]]
        return {
            "repo": repo,
            "required_labels": labels,
            "monitoring": monitoring,
            "ready": ready,
            "reason": reason,
            "capacity": {
                "matched": len(runners),
                "online": len(online),
                "idle": len(idle),
                "busy": len(busy),
                "min_online": min_online,
                "min_idle": min_idle,
            },
            "runners": runners,
        }
