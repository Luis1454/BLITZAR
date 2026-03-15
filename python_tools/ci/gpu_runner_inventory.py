#!/usr/bin/env python3
from __future__ import annotations

import json
import urllib.error
import urllib.request
from collections.abc import Callable, Iterable
from pathlib import Path
from typing import Protocol


class ResponseLike(Protocol):
    def read(self) -> bytes:
        ...

    def __enter__(self) -> ResponseLike:
        ...

    def __exit__(self, exc_type: object, exc: object, tb: object) -> bool | None:
        ...


UrlOpen = Callable[[urllib.request.Request], ResponseLike]


def _normalize_labels(labels: Iterable[str]) -> list[str]:
    return [label.strip() for label in labels if label.strip()]


class GitHubGpuRunnerInventory:
    def __init__(self, urlopen: UrlOpen | None = None) -> None:
        self._urlopen = urlopen or urllib.request.urlopen

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

    def write(self, report: dict[str, object], output_path: str) -> None:
        Path(output_path).parent.mkdir(parents=True, exist_ok=True)
        with open(output_path, "w", encoding="utf-8") as handle:
            json.dump(report, handle, indent=2)

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
    def _matches_required_labels(runner: dict[str, object], required: list[str]) -> bool:
        if not required:
            return True
        raw_labels = runner.get("labels", [])
        labels = {str(label) for label in raw_labels} if isinstance(raw_labels, list) else set()
        return all(label in labels for label in required)

    @staticmethod
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
