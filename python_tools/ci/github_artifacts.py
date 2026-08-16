#!/usr/bin/env python3
# @file scripts/ci/audit_github_artifacts.py
# @author Luis1454
# @project BLITZAR
# @brief Inventory GitHub Actions artifacts and classify their retention authority.

from __future__ import annotations

import json
from dataclasses import dataclass
from datetime import datetime
from typing import Any
from urllib.request import Request, urlopen

RETENTION_DAYS = {
    "pull-request": 7,
    "nightly-evidence": 14,
    "gpu-health": 7,
    "release-staging": 30,
    "github-pages": 0,
    "unclassified": 0,
}


@dataclass(frozen=True)
class Artifact:
    artifact_id: int
    name: str
    size: int
    created_at: datetime
    expired: bool


def classify_artifact(name: str) -> str:
    if name.startswith(("pr-", "tool-qualification-pr-")):
        return "pull-request"
    if name.startswith(("nightly-", "release-lane-logs-")):
        return "nightly-evidence"
    if name.startswith("gpu-runner-health-"):
        return "gpu-health"
    if name.startswith(("release-", "desktop-installer-")):
        return "release-staging"
    if name.startswith("tool-qualification-release-"):
        return "release-staging"
    if name == "github-pages":
        return "github-pages"
    return "unclassified"


def parse_artifact(payload: dict[str, Any]) -> Artifact:
    created = datetime.fromisoformat(str(payload["created_at"]).replace("Z", "+00:00"))
    return Artifact(
        artifact_id=int(payload["id"]),
        name=str(payload["name"]),
        size=int(payload["size_in_bytes"]),
        created_at=created,
        expired=bool(payload.get("expired", False)),
    )


def fetch_artifacts(repo: str, token: str) -> list[Artifact]:
    artifacts: list[Artifact] = []
    page = 1
    while True:
        url = f"https://api.github.com/repos/{repo}/actions/artifacts?per_page=100&page={page}"
        request = Request(url, headers={"Accept": "application/vnd.github+json", "Authorization": f"Bearer {token}"})
        with urlopen(request, timeout=30) as response:
            payload = json.load(response)
        page_items = payload.get("artifacts", [])
        if not isinstance(page_items, list) or not page_items:
            return artifacts
        artifacts.extend(parse_artifact(item) for item in page_items if isinstance(item, dict))
        if len(page_items) < 100:
            return artifacts
        page += 1


def delete_expired_artifacts(repo: str, token: str, artifacts: list[Artifact]) -> tuple[int, list[int]]:
    deleted = 0
    failures: list[int] = []
    for artifact in artifacts:
        if not artifact.expired:
            continue
        url = f"https://api.github.com/repos/{repo}/actions/artifacts/{artifact.artifact_id}"
        request = Request(
            url,
            headers={"Accept": "application/vnd.github+json", "Authorization": f"Bearer {token}"},
            method="DELETE",
        )
        try:
            with urlopen(request, timeout=30):
                deleted += 1
        except Exception:
            failures.append(artifact.artifact_id)
    return deleted, failures


def build_report(artifacts: list[Artifact], now: datetime) -> dict[str, Any]:
    classes: dict[str, dict[str, Any]] = {}
    stale_artifacts: list[dict[str, Any]] = []
    unclassified_artifacts: list[dict[str, Any]] = []
    for artifact in artifacts:
        category = classify_artifact(artifact.name)
        entry = classes.setdefault(
            category,
            {"count": 0, "bytes": 0, "expired": 0, "stale": 0, "oldest": None},
        )
        entry["count"] = int(entry["count"]) + 1
        entry["bytes"] = int(entry["bytes"]) + artifact.size
        if artifact.expired:
            entry["expired"] = int(entry["expired"]) + 1
        oldest = entry["oldest"]
        if oldest is None or artifact.created_at.isoformat() < str(oldest):
            entry["oldest"] = artifact.created_at.isoformat()
        if category == "unclassified":
            unclassified_artifacts.append(
                {
                    "id": artifact.artifact_id,
                    "name": artifact.name,
                    "size": artifact.size,
                    "created_at": artifact.created_at.isoformat(),
                    "expired": artifact.expired,
                }
            )
        retention = RETENTION_DAYS[category]
        age_days = (now - artifact.created_at).total_seconds() / 86400.0
        if not artifact.expired and retention > 0 and age_days > retention:
            entry["stale"] = int(entry["stale"]) + 1
            stale_artifacts.append(
                {
                    "id": artifact.artifact_id,
                    "name": artifact.name,
                    "category": category,
                    "size": artifact.size,
                    "created_at": artifact.created_at.isoformat(),
                }
            )
    return {
        "generated_at": now.isoformat(),
        "retention_days": RETENTION_DAYS,
        "total_count": len(artifacts),
        "total_bytes": sum(item.size for item in artifacts),
        "expired_count": sum(1 for item in artifacts if item.expired),
        "classes": classes,
        "stale_artifacts": stale_artifacts,
        "unclassified_artifacts": unclassified_artifacts,
    }

