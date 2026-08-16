# @file tests/checks/suites/policy/test_repo_policy_artifacts.py
# @author Luis1454
# @project BLITZAR
# @brief Automated verification assets for BLITZAR artifact hygiene.

from __future__ import annotations

from pathlib import Path

from tests.checks.suites.policy.test_repo_policy import _run, _write


# @brief Documents the generated binary rejection contract.
# @param tmp_path Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Release binaries belong to workflow artifacts, not the source repository.
def test_repo_policy_rejects_generated_binary_artifact(tmp_path: Path) -> None:
    _write(tmp_path / "engine" / "include" / "unexpected.dll", "not a source artifact\n")
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert not ok
    assert any("generated binary artifact must not be committed" in error for error in errors)

