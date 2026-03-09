#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

import pytest

from tests.checks.suites.policy.repo_policy_test_support import run_repo_policy, write_file
from tests.checks.suites.support.path_specs import (
    ENGINE_BACKEND_DIR,
    ENGINE_CONFIG_DIR,
    MODULES_QT_UI_DIR,
    RUNTIME_BACKEND_DIR,
    TESTS_UNIT_DIR,
    cpp_file,
)


@pytest.mark.parametrize(("path", "content", "expected"), [
    (cpp_file(ENGINE_BACKEND_DIR, "bad"), "using Alias = int;\n", "'using' is forbidden in C++ sources"),
    (cpp_file(ENGINE_CONFIG_DIR, "bad"), "namespace a::b {\n}\n", "nested namespace declaration (A::B) is forbidden"),
    (cpp_file(RUNTIME_BACKEND_DIR, "bad"), "namespace gravity_internal_bad {\n}\n", "gravity_internal_* namespace is forbidden"),
    (cpp_file(ENGINE_BACKEND_DIR, "bad_namespace"), "namespace {\nint g = 1;\n}\n", "unnamed namespace is forbidden in production paths"),
    (cpp_file(ENGINE_CONFIG_DIR, "bad_preprocessor_conditional"), "#ifdef _WIN32\nint f() { return 1; }\n#endif\n", "preprocessor conditionals are forbidden in C/C++ sources"),
])
def test_repo_policy_rejects_cpp_structure(tmp_path: Path, path: Path, content: str, expected: str) -> None:
    write_file(tmp_path / path, content)
    ok, errors, _ = run_repo_policy(tmp_path)
    assert not ok
    assert any(expected in error for error in errors)


@pytest.mark.parametrize(("path", "content", "expected"), [
    (cpp_file(ENGINE_BACKEND_DIR, "bad_goto"), "int f() { goto fail; fail: return 0; }\n", "Power of 10 rule 1 forbids goto"),
    (cpp_file(RUNTIME_BACKEND_DIR, "bad_longjmp"), "int f() { return longjmp(buf, 1); }\n", "Power of 10 rule 1 forbids setjmp/longjmp"),
    (cpp_file(ENGINE_CONFIG_DIR, "bad_do_while"), "int f() { do { return 1; } while (false); }\n", "Power of 10 rule 1 forbids do-while"),
    (cpp_file(RUNTIME_BACKEND_DIR, "bad_while_true"), "int f() { while (true) { return 1; } }\n", "Power of 10 rule 2 forbids open-ended while(true) loops"),
    (cpp_file(ENGINE_BACKEND_DIR, "bad_macro"), "#define BAD_LIMIT 8\n", "preprocessor macros are forbidden"),
    (cpp_file(RUNTIME_BACKEND_DIR, "bad_fn_ptr"), "typedef int (*BadFn)();\n", "Power of 10 rule 9 forbids function pointer typedefs"),
])
def test_repo_policy_rejects_power_of_10_cpp_patterns(tmp_path: Path, path: Path, content: str, expected: str) -> None:
    write_file(tmp_path / path, content)
    ok, errors, _ = run_repo_policy(tmp_path)
    assert not ok
    assert any(expected in error for error in errors)


@pytest.mark.parametrize(("stem", "content", "ok_expected"), [
    ("bad_layout", "void build() {\n    QVBoxLayout &layout = *new QVBoxLayout(this);\n}\n", False),
    ("good_layout", "void build() {\n    auto *layout = new QVBoxLayout(this);\n    layout->setSpacing(6);\n}\n", True),
])
def test_repo_policy_validates_qt_layout_ownership(tmp_path: Path, stem: str, content: str, ok_expected: bool) -> None:
    write_file(tmp_path / cpp_file(MODULES_QT_UI_DIR, stem), content)
    ok, errors, _ = run_repo_policy(tmp_path)
    assert ok is ok_expected
    assert (not errors) if ok_expected else any("Qt '*new + reference' ownership pattern is forbidden" in error for error in errors)


def test_repo_policy_warns_on_stale_allowlist_entry(tmp_path: Path) -> None:
    sample_path = cpp_file(TESTS_UNIT_DIR, "sample")
    write_file(tmp_path / sample_path, "int main() { return 0; }\n")
    allowlist = tmp_path / "allowlist.txt"
    allowlist.write_text(f"{sample_path.as_posix()}\n", encoding="utf-8")
    ok, _, warnings = run_repo_policy(tmp_path, allowlist)
    assert ok
    assert any("allowlist entry not needed anymore" in warning for warning in warnings)


def test_repo_policy_rejects_unnamed_namespace_in_tests_cpp(tmp_path: Path) -> None:
    write_file(tmp_path / cpp_file(TESTS_UNIT_DIR, "bad_namespace"), "namespace {\nint g = 1;\n}\n")
    ok, errors, _ = run_repo_policy(tmp_path)
    assert not ok
    assert any("unnamed namespace is forbidden" in error for error in errors)
