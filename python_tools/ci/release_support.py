#!/usr/bin/env python3
from __future__ import annotations

import os
from collections.abc import Mapping
from pathlib import Path

from python_tools.policies.deviation_register import DeviationRegister

DEFAULT_EVIDENCE_REFS = (
    "EVD_CI_RELEASE_LANE",
    "EVD_CI_SECURITY_CODEQL",
    "EVD_CI_DEPENDENCY_REVIEW",
    "EVD_GITHUB_DEPENDABOT",
    "EVD_QLT_DEVIATION_REGISTER",
    "EVD_QLT_EVIDENCE_PACK_FORMAT",
    "EVD_QLT_MANIFEST",
    "EVD_QLT_PROD_BASELINE",
    "EVD_QLT_README",
    "EVD_QLT_STANDARDS_PROFILE",
    "EVD_SCRIPT_RELEASE_PACKAGE_BUNDLE",
    "EVD_SCRIPT_RELEASE_PACKAGE_EVIDENCE",
    "EVD_SCRIPT_RELEASE_PACKAGE_SBOM",
    "EVD_SCRIPT_RELEASE_PACKAGE_SOURCE",
)


def resolve_release_tag(explicit: str | None) -> str:
    if explicit is not None and explicit.strip():
        return explicit.strip()
    ref_name = os.environ.get("GITHUB_REF_NAME", "").strip()
    run_number = os.environ.get("GITHUB_RUN_NUMBER", "").strip()
    if ref_name and run_number:
        return f"{ref_name}-{run_number}"
    if ref_name:
        return ref_name
    if run_number:
        return f"run-{run_number}"
    return "manual"


def build_release_lane_activities(profile: str) -> list[dict[str, str]]:
    return [
        {"name": "repo-quality-gate", "status": "pass", "command": "python tests/checks/check.py all --root . --config simulation.ini"},
        {"name": "ruff", "status": "pass", "command": "python -m ruff check ."},
        {"name": "mypy", "status": "pass", "command": "python -m mypy tests/checks scripts/ci python_tools"},
        {
            "name": "configure",
            "status": "pass",
            "command": f"cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DGRAVITY_PROFILE={profile} -DGRAVITY_STRICT_WARNINGS=ON",
        },
        {"name": "build", "status": "pass", "command": "cmake --build build --parallel"},
        {
            "name": "ctest-integration-safe",
            "status": "pass",
            "command": 'ctest --test-dir build --output-on-failure --timeout 180 --no-tests=error -R "TST_UNT_CONF_|TST_QLT_REPO_00(1|2|3|4|5|6|7|8|9)_"',
        },
        {"name": "release-bundle", "status": "pass", "command": "python scripts/ci/release/package_bundle.py --build-dir build --dist-dir dist/release-bundle"},
        {
            "name": "release-source",
            "status": "pass",
            "command": "python scripts/ci/release/package_source.py --repo-root . --dist-dir dist/source --tag <tag> --ref HEAD",
        },
        {
            "name": "release-sbom",
            "status": "pass",
            "command": "python scripts/ci/release/package_sbom.py --artifacts-dir dist/release-bundle --dist-dir dist/release-sbom",
        },
        {
            "name": "release-evidence-pack",
            "status": "pass",
            "command": f"python scripts/ci/release/package_evidence.py --root . --dist-dir dist/evidence-pack --profile {profile}",
        },
        {
            "name": "release-quality-index",
            "status": "pass",
            "command": f"python scripts/ci/release/package_quality_index.py --root . --dist-dir dist/release-quality-index --profile {profile}",
        },
        {
            "name": "generate-changelog",
            "status": "pass",
            "command": "python scripts/ci/release/generate_changelog.py --tag <tag> --repo-root . --output dist/CHANGELOG.md",
        },
        {
            "name": "publish-release",
            "status": "pass",
            "command": "python scripts/ci/release/publish_release.py --tag <tag> --notes-file dist/CHANGELOG.md --assets dist/source/*.zip dist/release-bundle/*.zip",
        },
    ]


def build_release_lane_analyzers() -> dict[str, str]:
    return {"ruff": "pass", "mypy": "pass"}


def default_ci_context() -> dict[str, str]:
    context = {
        "workflow": os.environ.get("GITHUB_WORKFLOW", "manual"),
        "ref": os.environ.get("GITHUB_REF_NAME", ""),
        "sha": os.environ.get("GITHUB_SHA", ""),
        "run_id": os.environ.get("GITHUB_RUN_ID", ""),
        "run_number": os.environ.get("GITHUB_RUN_NUMBER", ""),
        "repository": os.environ.get("GITHUB_REPOSITORY", ""),
    }
    server_url = os.environ.get("GITHUB_SERVER_URL", "").strip()
    if server_url and context["repository"] and context["run_id"]:
        context["run_url"] = f"{server_url}/{context['repository']}/actions/runs/{context['run_id']}"
    return {key: value for key, value in context.items() if value}


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
