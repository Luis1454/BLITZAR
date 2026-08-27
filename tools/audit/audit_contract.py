"""Load and validate the final repository qualification contract."""

from __future__ import annotations

import json
import pathlib
import re
import subprocess
from typing import Any


SHA_PATTERN = re.compile(r"^[0-9a-f]{40}$")
LANE_ID_PATTERN = re.compile(r"^[a-z0-9_-]+$")


def load_json(path: pathlib.Path) -> dict[str, Any]:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise ValueError(f"JSON object expected: {path}")
    return value


def tracked_paths(root: pathlib.Path) -> list[str]:
    result = subprocess.run(
        ["git", "-C", str(root), "ls-files", "-z"],
        check=False,
        capture_output=True,
    )
    if result.returncode != 0:
        raise RuntimeError(result.stderr.decode(errors="replace").strip())
    return sorted(item for item in result.stdout.decode().split("\0") if item)


def git_output(root: pathlib.Path, arguments: list[str]) -> str | None:
    try:
        result = subprocess.run(
            ["git", "-C", str(root), *arguments],
            check=False,
            capture_output=True,
            text=True,
        )
    except OSError:
        return None
    if result.returncode != 0:
        return None
    return result.stdout.strip()


def commit_exists(root: pathlib.Path, commit: str) -> bool:
    return git_output(root, ["cat-file", "-e", f"{commit}^{{commit}}"]) is not None


def is_inside(candidate: pathlib.Path, root: pathlib.Path) -> bool:
    return candidate == root or root in candidate.parents


def owner_for(relative: str, contract: dict[str, Any]) -> dict[str, str] | None:
    for entry in contract["root_files"]:
        if entry["path"] == relative:
            return entry
    matches = [
        entry
        for entry in contract["ownership_areas"]
        if relative.startswith(entry["prefix"])
    ]
    if not matches:
        return None
    return max(matches, key=lambda entry: len(entry["prefix"]))


def validate_entries(entries: Any, keys: tuple[str, ...], label: str) -> list[str]:
    if not isinstance(entries, list) or not entries:
        return [f"final audit {label} must be non-empty"]
    errors: list[str] = []
    for entry in entries:
        if not isinstance(entry, dict) or any(
            not isinstance(entry.get(key), str) or not entry[key] for key in keys
        ):
            errors.append(f"final audit {label} entry is incomplete")
    return errors


def validate_ownership(contract: dict[str, Any]) -> list[str]:
    errors = validate_entries(
        contract.get("root_files"), ("path", "owner", "category", "review_gate"), "root_files"
    )
    errors.extend(
        validate_entries(
            contract.get("ownership_areas"),
            ("prefix", "owner", "category", "review_gate"),
            "ownership_areas",
        )
    )
    roots = contract.get("root_files", [])
    areas = contract.get("ownership_areas", [])
    if isinstance(roots, list):
        paths = [entry.get("path") for entry in roots if isinstance(entry, dict)]
        if len(paths) != len(set(paths)):
            errors.append("final audit root file paths must be unique")
    if isinstance(areas, list):
        prefixes = [entry.get("prefix") for entry in areas if isinstance(entry, dict)]
        if len(prefixes) != len(set(prefixes)):
            errors.append("final audit ownership prefixes must be unique")
    return errors


def validate_lanes(lanes: Any) -> list[str]:
    errors = validate_entries(
        lanes, ("id", "workflow_job", "scope", "claim"), "required_lanes"
    )
    if not isinstance(lanes, list):
        return errors
    identifiers = [entry.get("id") for entry in lanes if isinstance(entry, dict)]
    if len(identifiers) != len(set(identifiers)):
        errors.append("final audit lane IDs must be unique")
    errors.extend(
        f"invalid final audit lane ID: {entry['id']}"
        for entry in lanes
        if isinstance(entry, dict)
        and isinstance(entry.get("id"), str)
        and LANE_ID_PATTERN.fullmatch(entry["id"]) is None
    )
    return errors


def validate_issues(issues: Any) -> list[str]:
    if not isinstance(issues, list) or not issues:
        return ["final audit milestone_issues must be non-empty"]
    errors: list[str] = []
    identifiers = [entry.get("issue") for entry in issues if isinstance(entry, dict)]
    if len(identifiers) != len(set(identifiers)):
        errors.append("final audit milestone issue IDs must be unique")
    for entry in issues:
        if not isinstance(entry, dict):
            errors.append("final audit milestone issue must be an object")
            continue
        if not isinstance(entry.get("issue"), int) or entry["issue"] <= 0:
            errors.append("final audit milestone issue needs a positive number")
        commit = entry.get("commit")
        if not isinstance(commit, str) or SHA_PATTERN.fullmatch(commit) is None:
            errors.append(f"invalid commit for issue {entry.get('issue')}")
        pull_request = entry.get("pull_request")
        if not isinstance(pull_request, str) or not pull_request.startswith("https://"):
            errors.append(f"issue {entry.get('issue')} needs a pull request URL")
        if not isinstance(entry.get("evidence"), list) or not entry["evidence"]:
            errors.append(f"issue {entry.get('issue')} needs evidence references")
    return errors


def validate_contract(root: pathlib.Path, contract: dict[str, Any]) -> list[str]:
    errors: list[str] = []
    manifest = load_json(root / "plan" / "manifest.json")
    if contract.get("schema_version") != 1:
        errors.append("final audit schema_version must be 1")
    if contract.get("runner") != "tools/audit/audit_final.py":
        errors.append("final audit runner is invalid")
    if contract.get("plan_version") != manifest.get("plan_version"):
        errors.append("final audit plan_version does not match the manifest")
    artifacts = contract.get("artifact_files")
    if not isinstance(artifacts, list) or not artifacts:
        errors.append("final audit artifact_files must be non-empty")
    elif len(artifacts) != len(set(artifacts)):
        errors.append("final audit artifact_files must be unique")
    errors.extend(validate_ownership(contract))
    errors.extend(validate_lanes(contract.get("required_lanes")))
    errors.extend(validate_issues(contract.get("milestone_issues")))
    non_claims = contract.get("non_claims")
    if not isinstance(non_claims, list) or not non_claims:
        errors.append("final audit non_claims must be non-empty")
    return errors
