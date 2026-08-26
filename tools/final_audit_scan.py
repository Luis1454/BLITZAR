"""Scan tracked files and existing repository qualification registries."""

from __future__ import annotations

import hashlib
import pathlib
from typing import Any

from architecture_report import build_repository_report
from architecture_sources import source_completeness_report
from final_audit_contract import load_json, owner_for, tracked_paths
from workspace_gate import generated_path, inventory, load_policy, policy_violations


def audit_file(
    root: pathlib.Path, relative: str, owner: dict[str, str]
) -> tuple[dict[str, Any], list[str]]:
    path = root / pathlib.PurePosixPath(relative)
    entry: dict[str, Any] = {
        "path": relative,
        "owner": owner["owner"],
        "category": owner["category"],
        "review_gate": owner["review_gate"],
        "review_status": "scanned",
    }
    errors: list[str] = []
    try:
        data = path.read_bytes()
    except OSError as error:
        errors.append(f"{relative}: cannot read tracked file: {error}")
        return entry, errors

    entry["byte_count"] = len(data)
    entry["sha256"] = hashlib.sha256(data).hexdigest()
    if b"\0" in data:
        entry["encoding"] = "binary"
        entry["line_count"] = None
        entry["final_newline"] = None
        return entry, errors

    try:
        text = data.decode("utf-8")
    except UnicodeDecodeError as error:
        entry["encoding"] = "invalid-utf8"
        errors.append(f"{relative}: invalid UTF-8: {error}")
        return entry, errors

    entry["encoding"] = "UTF-8"
    entry["line_count"] = len(text.splitlines())
    entry["final_newline"] = data.endswith(b"\n")
    if data.startswith(b"\xef\xbb\xbf"):
        errors.append(f"{relative}: UTF-8 BOM is forbidden")
    if b"\r\n" in data:
        errors.append(f"{relative}: CRLF line ending is forbidden")
    if not entry["final_newline"]:
        errors.append(f"{relative}: final newline is missing")
    return entry, errors


def audit_files(
    root: pathlib.Path, contract: dict[str, Any]
) -> tuple[list[dict[str, Any]], list[str]]:
    matrix: list[dict[str, Any]] = []
    errors: list[str] = []
    paths = tracked_paths(root)
    if len(paths) != len(set(paths)):
        errors.append("git tracked file inventory contains duplicate paths")
    for relative in paths:
        owner = owner_for(relative, contract)
        if owner is None:
            errors.append(f"tracked file has no owner or category: {relative}")
            continue
        entry, file_errors = audit_file(root, relative, owner)
        matrix.append(entry)
        errors.extend(file_errors)
    return matrix, errors


def review_errors(root: pathlib.Path, architecture: dict[str, Any]) -> list[str]:
    data = load_json(root / "plan" / "architecture_reviews.json")
    reviews = data.get("reviews", [])
    if not isinstance(reviews, list):
        return ["architecture review registry is not a list"]
    registered = {
        entry.get("path"): set(entry.get("signals", []))
        for entry in reviews
        if isinstance(entry, dict)
    }
    required = {
        entry["path"]: set(entry.get("signals", []))
        for entry in architecture.get("files", [])
        if isinstance(entry, dict) and entry.get("signals")
    }
    errors: list[str] = []
    if set(registered) != set(required):
        errors.append(
            "architecture review registry does not match current signals: "
            f"missing={sorted(set(required) - set(registered))}, "
            f"stale={sorted(set(registered) - set(required))}"
        )
    for path, signals in required.items():
        if registered.get(path) != signals:
            errors.append(f"architecture review signals mismatch: {path}")
    return errors


def workspace_summary(root: pathlib.Path) -> dict[str, Any]:
    policy = load_policy(root)
    paths = tracked_paths(root)
    violations = policy_violations(root, policy)
    violations.extend(
        f"tracked generated path is forbidden: {path}"
        for path in paths
        if generated_path(path, policy)
    )
    return {
        "status": "ok" if not violations else "invalid",
        "counts": inventory(root),
        "violations": violations,
    }


def structural_summary(root: pathlib.Path) -> tuple[dict[str, Any], list[str]]:
    architecture = build_repository_report(root)
    errors = review_errors(root, architecture)
    if architecture.get("unclassified_files"):
        errors.append("architecture profiles leave files unclassified")
    completeness = source_completeness_report(root)
    if completeness.get("status") != "ok":
        errors.append(
            "CMake source completeness is invalid: "
            f"missing={completeness.get('missing')}, stale={completeness.get('stale')}"
        )
    summary = {
        "repository_file_count": architecture.get("repository_file_count"),
        "profile_counts": {
            name: value.get("file_count")
            for name, value in architecture.get("profiles", {}).items()
        },
        "review_required_paths": architecture.get("review_required_paths", []),
        "source_completeness": completeness,
    }
    return summary, errors
