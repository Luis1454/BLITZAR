#!/usr/bin/env python3
from __future__ import annotations

import os


def resolve_release_tag(explicit: str | None) -> str:
    if explicit and explicit.strip():
        return explicit.strip()
    ref_name = os.environ.get("GITHUB_REF_NAME", "").strip()
    if ref_name:
        return ref_name
    run_number = os.environ.get("GITHUB_RUN_NUMBER", "").strip()
    if run_number:
        return f"manual-{run_number}"
    return "manual"
