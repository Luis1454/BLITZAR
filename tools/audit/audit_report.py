"""Assemble and write the final repository qualification report."""

from __future__ import annotations

import json
import os
import pathlib
import shlex
import subprocess
import sys
from typing import Any

from tools.audit.audit_contract import commit_exists, git_output, load_json
from tools.audit.audit_scan import audit_files, structural_summary, workspace_summary


def finding_register(root: pathlib.Path, architecture: dict[str, Any]) -> dict[str, Any]:
    review_data = load_json(root / "plan" / "architecture_reviews.json")
    capability_data = load_json(root / "plan" / "capabilities.json")
    accepted = [
        entry
        for entry in review_data.get("reviews", [])
        if isinstance(entry, dict) and entry.get("status") == "accepted"
    ]
    deferred = [
        {
            "id": entry.get("id"),
            "state": entry.get("state"),
            "root": entry.get("root"),
            "decision": "plan/decisions/031-capability-contract.md",
        }
        for entry in capability_data.get("features", [])
        if isinstance(entry, dict) and entry.get("state") == "deferred"
    ]
    non_claimed = [
        {
            "id": entry.get("id"),
            "state": entry.get("state"),
            "evidence": entry.get("runtime_evidence", []),
        }
        for group in ("solvers", "backends")
        for entry in capability_data.get(group, [])
        if isinstance(entry, dict)
        and entry.get("state") in {"unsupported", "capability-gated"}
    ]
    return {
        "unresolved": [],
        "accepted_architecture_reviews": accepted,
        "deferred_capabilities": deferred,
        "unsupported_or_capability_gated": non_claimed,
        "architecture_review_count": len(architecture.get("review_required_paths", [])),
    }


def lane_records(contract: dict[str, Any]) -> list[dict[str, str]]:
    records: list[dict[str, str]] = []
    for lane in contract["required_lanes"]:
        variable = "BLITZAR_LANE_" + lane["id"].upper().replace("-", "_")
        records.append(
            {
                "id": lane["id"],
                "workflow_job": lane["workflow_job"],
                "scope": lane["scope"],
                "claim": lane["claim"],
                "state": os.environ.get(variable, "unreported"),
                "environment_variable": variable,
            }
        )
    return records


def lane_errors(lanes: list[dict[str, str]], strict: bool) -> list[str]:
    if not strict:
        return []
    return [
        f"required CI lane is not successful: {lane['id']}={lane['state']}"
        for lane in lanes
        if lane["state"] != "success"
    ]


def run_command(
    root: pathlib.Path, command: str, timeout: int, label: str
) -> dict[str, Any]:
    arguments = shlex.split(command)
    if arguments and arguments[0] in {"python", "python3"}:
        arguments[0] = sys.executable
    try:
        result = subprocess.run(
            arguments,
            cwd=root,
            capture_output=True,
            text=True,
            timeout=timeout,
            check=False,
        )
    except subprocess.TimeoutExpired as error:
        return {
            "label": label,
            "command": command,
            "state": "failed",
            "returncode": None,
            "stdout": error.stdout or "",
            "stderr": error.stderr or "",
        }
    except OSError as error:
        return {
            "label": label,
            "command": command,
            "state": "failed",
            "returncode": None,
            "stdout": "",
            "stderr": str(error),
        }
    return {
        "label": label,
        "command": command,
        "state": "passed" if result.returncode == 0 else "failed",
        "returncode": result.returncode,
        "stdout": result.stdout,
        "stderr": result.stderr,
    }


def milestone_records(root: pathlib.Path, contract: dict[str, Any]) -> tuple[list[dict[str, Any]], list[str]]:
    records: list[dict[str, Any]] = []
    errors: list[str] = []
    for issue in contract["milestone_issues"]:
        present = commit_exists(root, issue["commit"])
        records.append(
            {**issue, "commit_present": present, "status": "merged" if present else "missing"}
        )
        if not present:
            errors.append(f"implementation commit is missing for issue {issue['issue']}")
    return records, errors


def build_audit(
    root: pathlib.Path, contract: dict[str, Any], run_gates: bool, strict: bool
) -> tuple[dict[str, Any], dict[str, Any] | None]:
    from tools.audit.audit_contract import validate_contract

    errors = validate_contract(root, contract)
    matrix, matrix_errors = audit_files(root, contract)
    errors.extend(matrix_errors)
    architecture, architecture_errors = structural_summary(root)
    errors.extend(architecture_errors)
    workspace = workspace_summary(root)
    errors.extend(str(item) for item in workspace["violations"])
    milestone, milestone_errors = milestone_records(root, contract)
    errors.extend(milestone_errors)

    lanes = lane_records(contract)
    errors.extend(lane_errors(lanes, strict))
    status_output = git_output(root, ["status", "--porcelain"])
    tree_clean = status_output == ""
    if strict and not tree_clean:
        errors.append("repository tree is not clean")

    gate: dict[str, Any] | None = None
    if run_gates:
        gate = run_command(
            root,
            "python -m tools.gates.quality_gate --root . --group static",
            600,
            "static-quality",
        )
        if gate["state"] != "passed":
            errors.append("static quality gate failed")

    findings = finding_register(root, architecture)
    findings["unresolved"] = list(errors)
    complete = all(lane["state"] == "success" for lane in lanes)
    report = {
        "schema_version": 1,
        "status": "passed" if not errors else "failed",
        "qualification": "complete-ci" if complete else "partial-local",
        "revision": git_output(root, ["rev-parse", "HEAD"]),
        "plan_version": contract["plan_version"],
        "tree_clean": tree_clean,
        "file_count": len(matrix),
        "files": matrix,
        "architecture": architecture,
        "workspace": workspace,
        "milestone": {"issues": milestone},
        "lanes": lanes,
        "findings": findings,
        "non_claims": contract["non_claims"],
        "errors": errors,
    }
    return report, gate


def render_summary(report: dict[str, Any]) -> str:
    passed_lanes = sum(lane["state"] == "success" for lane in report["lanes"])
    lines = [
        "# BLITZAR Final Repository Qualification",
        "",
        f"- status: `{report['status']}`",
        f"- qualification: `{report['qualification']}`",
        f"- revision: `{report['revision']}`",
        f"- plan version: `{report['plan_version']}`",
        f"- tree clean: `{report['tree_clean']}`",
        f"- tracked files: `{report['file_count']}`",
        f"- CI lanes successful: `{passed_lanes}/{len(report['lanes'])}`",
        f"- unresolved findings: `{len(report['findings']['unresolved'])}`",
        "",
        "## Lane States",
        "",
        "| Lane | State | Scope | Claim |",
        "| --- | --- | --- | --- |",
    ]
    lines.extend(
        f"| {lane['id']} | {lane['state']} | {lane['scope']} | {lane['claim']} |"
        for lane in report["lanes"]
    )
    lines.extend(
        [
            "",
            "## Findings",
            "",
            f"- accepted architecture reviews: `{len(report['findings']['accepted_architecture_reviews'])}`",
            f"- deferred capabilities: `{len(report['findings']['deferred_capabilities'])}`",
            f"- unsupported or capability-gated entries: `{len(report['findings']['unsupported_or_capability_gated'])}`",
            "",
            "The complete file matrix and generated logs are stored in this external audit directory.",
        ]
    )
    return "\n".join(lines) + "\n"


def write_artifacts(
    output: pathlib.Path, report: dict[str, Any], gate: dict[str, Any] | None
) -> None:
    output.mkdir(parents=True, exist_ok=True)
    (output / "gates").mkdir(parents=True, exist_ok=True)
    matrix = report["files"]
    findings = report["findings"]
    audit = {key: value for key, value in report.items() if key not in {"files", "findings"}}
    (output / "audit.json").write_text(json.dumps(audit, indent=2) + "\n", encoding="utf-8")
    (output / "file_matrix.json").write_text(json.dumps(matrix, indent=2) + "\n", encoding="utf-8")
    (output / "findings.json").write_text(json.dumps(findings, indent=2) + "\n", encoding="utf-8")
    (output / "summary.md").write_text(render_summary(report), encoding="utf-8")
    if gate is not None:
        gate_text = gate["stdout"] + ("\n" + gate["stderr"] if gate["stderr"] else "")
        (output / "gates" / "static-quality.log").write_text(gate_text, encoding="utf-8")
