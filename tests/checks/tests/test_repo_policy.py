#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

from python_tools.core.models import CheckContext
from python_tools.policies.repo_policy import RepoPolicyCheck


def _write(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def _run(root: Path, allowlist: Path) -> tuple[bool, list[str], list[str]]:
    context = CheckContext(root=root, allowlist=allowlist, target_lines=200, hard_lines=300)
    result = RepoPolicyCheck().run(context)
    return result.ok, result.errors, result.warnings


def test_repo_policy_rejects_using_in_cpp(tmp_path: Path) -> None:
    _write(tmp_path / "engine/src/backend/bad.cpp", "using Alias = int;\n")
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert not ok
    assert any("'using' is forbidden in C++ sources" in error for error in errors)


def test_repo_policy_rejects_nested_namespace_syntax(tmp_path: Path) -> None:
    _write(tmp_path / "engine/src/config/bad.cpp", "namespace a::b {\n}\n")
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert not ok
    assert any("nested namespace declaration (A::B) is forbidden" in error for error in errors)


def test_repo_policy_rejects_gravity_internal_namespace(tmp_path: Path) -> None:
    _write(tmp_path / "runtime/src/backend/bad.cpp", "namespace gravity_internal_bad {\n}\n")
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert not ok
    assert any("gravity_internal_* namespace is forbidden" in error for error in errors)


def test_repo_policy_warns_on_stale_allowlist_entry(tmp_path: Path) -> None:
    _write(tmp_path / "tests/unit/sample.cpp", "int main() { return 0; }\n")
    allowlist = tmp_path / "allowlist.txt"
    allowlist.write_text("tests/unit/sample.cpp\n", encoding="utf-8")
    ok, _, warnings = _run(tmp_path, allowlist)
    assert ok
    assert any("allowlist entry not needed anymore" in warning for warning in warnings)

