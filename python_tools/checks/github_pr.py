#!/usr/bin/env python3
from __future__ import annotations

import json
import urllib.request
from pathlib import Path

from python_tools.core.models import CheckResult


def load_event_payload(path_text: str, result: CheckResult) -> dict[str, object]:
    if not path_text:
        result.add_error("missing event payload path")
        return {}
    path = Path(path_text)
    if not path.exists():
        result.add_error(f"event payload not found: {path}")
        return {}
    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except Exception as exc:
        result.add_error(f"failed to parse event payload: {exc}")
        return {}
    if not isinstance(payload, dict):
        result.add_error("event payload root must be an object")
        return {}
    return payload


def fetch_github_list(
    url: str,
    token: str,
    result: CheckResult,
    error_label: str,
) -> list[dict[str, object]]:
    request = urllib.request.Request(
        url,
        headers={
            "Accept": "application/vnd.github+json",
            "Authorization": f"Bearer {token}",
            "X-GitHub-Api-Version": "2022-11-28",
        },
    )
    try:
        with urllib.request.urlopen(request) as response:
            payload = json.loads(response.read().decode("utf-8"))
    except Exception as exc:
        result.add_error(f"failed to query {error_label}: {exc}")
        return []
    if not isinstance(payload, list):
        result.add_error(f"unexpected {error_label} payload shape")
        return []
    return [item for item in payload if isinstance(item, dict)]

