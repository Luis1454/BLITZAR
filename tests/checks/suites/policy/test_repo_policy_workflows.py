# File: tests/checks/suites/policy/test_repo_policy_workflows.py
# Purpose: Verification coverage for the BLITZAR quality gate.

from __future__ import annotations

from pathlib import Path

from python_tools.core.models import CheckContext
from python_tools.policies.repo_policy import RepoPolicyCheck


# Description: Executes the _write operation.
def _write(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


# Description: Executes the _run operation.
def _run(root: Path) -> tuple[bool, list[str], list[str]]:
    context = CheckContext(root=root, allowlist=root / "allowlist.txt", target_lines=200, hard_lines=300)
    result = RepoPolicyCheck().run(context)
    return result.ok, result.errors, result.warnings


# Description: Executes the test_repo_policy_rejects_floating_workflow_action_versions operation.
def test_repo_policy_rejects_floating_workflow_action_versions(tmp_path: Path) -> None:
    _write(
        tmp_path / ".github" / "workflows" / "pr-fast.yml",
        "jobs:\n"
        "  ci:\n"
        "    steps:\n"
        "      - uses: actions/checkout@v4\n",
    )
    ok, errors, _ = _run(tmp_path)
    assert not ok
    assert any("workflow actions must pin full commit SHAs" in error for error in errors)


# Description: Executes the test_repo_policy_accepts_sha_pinned_workflow_action_versions operation.
def test_repo_policy_accepts_sha_pinned_workflow_action_versions(tmp_path: Path) -> None:
    _write(
        tmp_path / ".github" / "workflows" / "pr-fast.yml",
        "jobs:\n"
        "  ci:\n"
        "    steps:\n"
        "      - uses: actions/checkout@de0fac2e4500dabe0009e67214ff5f5447ce83dd\n",
    )
    ok, errors, _ = _run(tmp_path)
    assert ok
    assert not errors


# Description: Executes the test_repo_policy_rejects_ad_hoc_workflow_pip_installs operation.
def test_repo_policy_rejects_ad_hoc_workflow_pip_installs(tmp_path: Path) -> None:
    _write(
        tmp_path / ".github" / "workflows" / "pr-fast.yml",
        "jobs:\n"
        "  ci:\n"
        "    steps:\n"
        "      - run: python -m pip install pytest ruff mypy\n",
    )
    ok, errors, _ = _run(tmp_path)
    assert not ok
    assert any("workflow pip installs must use .github/ci/requirements-py312.txt" in error for error in errors)


# Description: Executes the test_repo_policy_accepts_manifest_driven_workflow_pip_installs operation.
def test_repo_policy_accepts_manifest_driven_workflow_pip_installs(tmp_path: Path) -> None:
    _write(
        tmp_path / ".github" / "workflows" / "pr-fast.yml",
        "jobs:\n"
        "  ci:\n"
        "    steps:\n"
        "      - run: python -m pip install -r .github/ci/requirements-py312.txt\n",
    )
    ok, errors, _ = _run(tmp_path)
    assert ok
    assert not errors


# Description: Executes the test_repo_policy_rejects_release_lane_without_explicit_protocol_cli_physics_subset operation.
def test_repo_policy_rejects_release_lane_without_explicit_protocol_cli_physics_subset(tmp_path: Path) -> None:
    _write(
        tmp_path / ".github" / "workflows" / "release-lane.yml",
        "jobs:\n"
        "  release:\n"
        "    steps:\n"
        "      - run: ctest --test-dir build --no-tests=error -R \"TST_QLT_REPO_001_\"\n",
    )
    ok, errors, _ = _run(tmp_path)
    assert not ok
    assert any("release lane must exercise an explicit deterministic product subset containing TST_UNT_PROT_" in error for error in errors)
    assert any("release lane must exercise an explicit deterministic product subset containing TST_UNT_MODCLI_" in error for error in errors)
    assert any("release lane must exercise an explicit deterministic product subset containing TST_UNT_PHYS_" in error for error in errors)


# Description: Executes the test_repo_policy_rejects_workflow_build_failure_masking_with_shell_fallback operation.
def test_repo_policy_rejects_workflow_build_failure_masking_with_shell_fallback(tmp_path: Path) -> None:
    _write(
        tmp_path / ".github" / "workflows" / "pr-fast.yml",
        "jobs:\n"
        "  ci:\n"
        "    steps:\n"
        "      - run: cmake --build build-dev-mod --parallel --target gravityClientModuleQtInProc || echo \"skip\"\n",
    )
    ok, errors, _ = _run(tmp_path)
    assert not ok
    assert any("must not mask build/test command failures" in error for error in errors)


# Description: Executes the test_repo_policy_accepts_workflow_build_command_without_shell_fallback operation.
def test_repo_policy_accepts_workflow_build_command_without_shell_fallback(tmp_path: Path) -> None:
    _write(
        tmp_path / ".github" / "workflows" / "pr-fast.yml",
        "jobs:\n"
        "  ci:\n"
        "    steps:\n"
        "      - run: cmake --build build-dev-mod --parallel --target gravityClientModuleQtInProc\n",
    )
    ok, errors, _ = _run(tmp_path)
    assert ok
    assert not errors


# Description: Executes the test_repo_policy_accepts_release_lane_with_explicit_protocol_cli_physics_subset operation.
def test_repo_policy_accepts_release_lane_with_explicit_protocol_cli_physics_subset(tmp_path: Path) -> None:
    _write(
        tmp_path / ".github" / "workflows" / "release-lane.yml",
        "jobs:\n"
        "  release:\n"
        "    steps:\n"
        "      - run: ctest --test-dir build --no-tests=error -R \"TST_UNT_PROT_|TST_UNT_MODCLI_|TST_UNT_PHYS_\"\n",
    )
    ok, errors, _ = _run(tmp_path)
    assert ok
    assert not errors
