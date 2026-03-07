#!/usr/bin/env python3
from __future__ import annotations

from collections.abc import Mapping
from pathlib import Path

from python_tools.policies.deviation_register import DeviationRegister

DEFAULT_EVIDENCE_REFS = (
    "EVD_CI_RELEASE_LANE",
    "EVD_QLT_DEVIATION_REGISTER",
    "EVD_QLT_EVIDENCE_PACK_FORMAT",
    "EVD_QLT_MANIFEST",
    "EVD_QLT_README",
    "EVD_QLT_STANDARDS_PROFILE",
    "EVD_SCRIPT_RELEASE_PACKAGE_BUNDLE",
    "EVD_SCRIPT_RELEASE_PACKAGE_EVIDENCE",
)


def load_open_exceptions(root: Path) -> list[dict[str, object]]:
    rows, errors = DeviationRegister().load_open_with_errors(root)
    if errors:
        raise RuntimeError(errors[0])
    return [row for row in rows if isinstance(row, dict)]


def render_pack_readme(pack: Mapping[str, object]) -> str:
    requirement_ids = pack.get("requirement_ids")
    evidence_refs = pack.get("evidence_refs")
    open_exceptions = pack.get("open_exceptions")
    requirement_count = len(requirement_ids) if isinstance(requirement_ids, list) else 0
    evidence_count = len(evidence_refs) if isinstance(evidence_refs, list) else 0
    exception_count = len(open_exceptions) if isinstance(open_exceptions, list) else 0
    return (
        "# Release Evidence Pack\n\n"
        f"- Tag: `{pack.get('tag', '')}`\n"
        f"- Profile: `{pack.get('profile', '')}`\n"
        f"- Requirements covered: `{requirement_count}`\n"
        f"- Evidence refs bundled: `{evidence_count}`\n"
        f"- Open exceptions recorded: `{exception_count}`\n\n"
        "See `release_evidence_pack.json` for the machine-readable record.\n"
    )
