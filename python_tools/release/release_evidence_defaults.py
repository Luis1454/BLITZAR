#!/usr/bin/env python3
from __future__ import annotations

import os


def build_release_lane_activities(profile: str) -> list[dict[str, str]]:
    return [
        {"name": "repo-quality-gate", "status": "pass", "command": "python tests/checks/check.py all --root . --config simulation.ini"},
        {"name": "ruff", "status": "pass", "command": "python -m ruff check ."},
        {"name": "mypy", "status": "pass", "command": "python -m mypy tests/checks python_tools"},
        {"name": "pytest", "status": "pass", "command": "python -m pytest -q tests/checks/suites"},
        {
            "name": "configure",
            "status": "pass",
            "command": f"cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DGRAVITY_PROFILE={profile} -DGRAVITY_STRICT_WARNINGS=ON",
        },
        {"name": "build", "status": "pass", "command": "cmake --build build --parallel"},
        {
            "name": "ctest-integration-safe",
            "status": "pass",
            "command": 'ctest --test-dir build --output-on-failure --timeout 180 --no-tests=error -R "TST_UNT_CONF_|TST_QLT_REPO_00(1|2|3|4|5|6|7)_"',
        },
        {"name": "release-bundle", "status": "pass", "command": "python -m python_tools.release.cli package_bundle --build-dir build --dist-dir dist/release-bundle"},
        {
            "name": "release-evidence-pack",
            "status": "pass",
            "command": f"python -m python_tools.release.cli package_evidence --root . --dist-dir dist/evidence-pack --profile {profile}",
        },
        {
            "name": "release-quality-index",
            "status": "pass",
            "command": f"python -m python_tools.release.cli package_quality_index --root . --dist-dir dist/release-quality-index --profile {profile}",
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

