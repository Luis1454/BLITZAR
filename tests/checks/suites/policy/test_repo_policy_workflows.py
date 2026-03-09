#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

import pytest

from tests.checks.suites.policy.repo_policy_test_support import run_repo_policy, write_file


@pytest.mark.parametrize(("name", "content", "ok_expected", "error_substring"), [
    ("ok.hpp", "#ifndef GRAVITY_ENGINE_INCLUDE_OK_HPP_\n#define GRAVITY_ENGINE_INCLUDE_OK_HPP_\n\nstruct Ok {};\n\n#endif // GRAVITY_ENGINE_INCLUDE_OK_HPP_\n", True, ""),
    ("bad.hpp", "#pragma once\nstruct Bad {};\n", False, "#pragma once is forbidden"),
])
def test_repo_policy_validates_header_form(tmp_path: Path, name: str, content: str, ok_expected: bool, error_substring: str) -> None:
    write_file(tmp_path / "engine" / "include" / name, content)
    ok, errors, _ = run_repo_policy(tmp_path)
    assert ok is ok_expected
    assert (not errors) if ok_expected else any(error_substring in error for error in errors)


def test_repo_policy_rejects_json_above_hard_limit(tmp_path: Path) -> None:
    oversize = "{\n" + "\"k\":0,\n" * 305 + "\"end\":1\n}\n"
    write_file(tmp_path / "docs" / "quality" / "oversize.json", oversize)
    ok, errors, _ = run_repo_policy(tmp_path)
    assert not ok
    assert any("oversize.json" in error and "exceeds hard limit" in error for error in errors)


@pytest.mark.parametrize(("workflow", "content", "ok_expected", "error_substring"), [
    ("release-lane.yml", "jobs:\n  release:\n    steps:\n      - name: Configure\n        run: |\n          cmake -S . -B build -G Ninja \\\n            -DCMAKE_BUILD_TYPE=Release\n", False, "evidence configure command must include -DGRAVITY_PROFILE=prod"),
    ("pr-fast.yml", "jobs:\n  dev:\n    steps:\n      - name: Configure\n        run: |\n          cmake -S . -B build-dev-mod -G Ninja \\\n            -DGRAVITY_PROFILE=dev\n", True, ""),
    ("nightly-full.yml", "jobs:\n  nightly:\n    steps:\n      - name: Run tests\n        run: ctest --test-dir build -R \"TST_QLT_REPO_001_\"\n", False, "CI ctest command must include --no-tests=error"),
    ("release-lane.yml", "jobs:\n  release:\n    steps:\n      - name: Run tests\n        run: ctest --test-dir build --output-on-failure --timeout 180 --no-tests=error -R \"TST_QLT_REPO_001_\"\n", True, ""),
    ("nightly-full.yml", "jobs:\n  nightly:\n    steps:\n      - name: Run tests\n        run: ctest --test-dir build --output-on-failure --timeout 180 --no-tests=error -R \"ConfigArgsTest\\\\.TST_UNT_CONF_\"\n", False, "CI ctest selector must use normalized TST_* ids"),
    ("nightly-full.yml", "jobs:\n  nightly:\n    steps:\n      - name: Run tests\n        run: ctest --test-dir build --output-on-failure --timeout 180 --no-tests=error -R \"TST_UNT_CONF_|TST_QLT_REPO_001_\"\n", True, ""),
])
def test_repo_policy_validates_workflow_commands(tmp_path: Path, workflow: str, content: str, ok_expected: bool, error_substring: str) -> None:
    write_file(tmp_path / ".github" / "workflows" / workflow, content)
    ok, errors, _ = run_repo_policy(tmp_path)
    assert ok is ok_expected
    assert (not errors) if ok_expected else any(error_substring in error for error in errors)
