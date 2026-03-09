#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

from tests.checks.suites.policy.test_repo_policy import _run, _write
from tests.checks.suites.support.path_specs import ENGINE_BACKEND_DIR, RUNTIME_BACKEND_DIR, cpp_file


def test_repo_policy_rejects_while_true_in_prod_cpp(tmp_path: Path) -> None:
    _write(tmp_path / cpp_file(RUNTIME_BACKEND_DIR, "bad_while_true"), "int f() { while (true) { return 1; } }\n")
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert not ok
    assert any("Power of 10 rule 2 forbids open-ended while(true) loops" in error for error in errors)


def test_repo_policy_rejects_non_structural_macro_in_prod_cpp(tmp_path: Path) -> None:
    _write(tmp_path / cpp_file(ENGINE_BACKEND_DIR, "bad_macro"), "#define BAD_LIMIT 8\n")
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert not ok
    assert any("Power of 10 rule 8 forbids non-structural object-like macros" in error for error in errors)


def test_repo_policy_accepts_pragma_once_and_allowed_power_of_10_macro(tmp_path: Path) -> None:
    _write(
        tmp_path / "engine" / "include" / "ok.hpp",
        "#pragma once\n"
        "#define GRAVITY_HD GRAVITY_HD_HOST GRAVITY_HD_DEVICE\n",
    )
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert ok
    assert not errors


def test_repo_policy_rejects_function_pointer_typedef_outside_abi_boundary(tmp_path: Path) -> None:
    _write(tmp_path / cpp_file(RUNTIME_BACKEND_DIR, "bad_fn_ptr"), "typedef int (*BadFn)();\n")
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert not ok
    assert any("Power of 10 rule 9 forbids function pointer typedefs" in error for error in errors)


def test_repo_policy_accepts_function_pointer_typedef_in_explicit_abi_boundary(tmp_path: Path) -> None:
    _write(
        tmp_path / "runtime" / "include" / "frontend" / "FrontendModuleApi.hpp",
        "#pragma once\n"
        "typedef int (*AllowedFn)();\n"
    )
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert ok
    assert not errors
