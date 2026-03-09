#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

from tests.checks.suites.policy.repo_policy_test_support import run_repo_policy, write_file
from tests.checks.suites.support.path_specs import ENGINE_BACKEND_DIR, RUNTIME_BACKEND_DIR, cpp_file


def test_repo_policy_rejects_while_true_in_prod_cpp(tmp_path: Path) -> None:
    write_file(tmp_path / cpp_file(RUNTIME_BACKEND_DIR, "bad_while_true"), "int f() { while (true) { return 1; } }\n")
    ok, errors, _ = run_repo_policy(tmp_path)
    assert not ok
    assert any("Power of 10 rule 2 forbids open-ended while(true) loops" in error for error in errors)


def test_repo_policy_rejects_non_structural_macro_in_prod_cpp(tmp_path: Path) -> None:
    write_file(tmp_path / cpp_file(ENGINE_BACKEND_DIR, "bad_macro"), "#define BAD_LIMIT 8\n")
    ok, errors, _ = run_repo_policy(tmp_path)
    assert not ok
    assert any("preprocessor macros are forbidden" in error for error in errors)


def test_repo_policy_accepts_include_guard_in_header(tmp_path: Path) -> None:
    write_file(
        tmp_path / "engine" / "include" / "ok.hpp",
        "#ifndef GRAVITY_ENGINE_INCLUDE_OK_HPP_\n"
        "#define GRAVITY_ENGINE_INCLUDE_OK_HPP_\n"
        "\n"
        "struct Ok {};\n"
        "\n"
        "#endif // GRAVITY_ENGINE_INCLUDE_OK_HPP_\n",
    )
    ok, errors, _ = run_repo_policy(tmp_path)
    assert ok
    assert not errors


def test_repo_policy_rejects_function_pointer_typedef_outside_abi_boundary(tmp_path: Path) -> None:
    write_file(tmp_path / cpp_file(RUNTIME_BACKEND_DIR, "bad_fn_ptr"), "typedef int (*BadFn)();\n")
    ok, errors, _ = run_repo_policy(tmp_path)
    assert not ok
    assert any("Power of 10 rule 9 forbids function pointer typedefs" in error for error in errors)


def test_repo_policy_accepts_function_pointer_typedef_in_explicit_abi_boundary(tmp_path: Path) -> None:
    write_file(
        tmp_path / "runtime" / "include" / "frontend" / "FrontendModuleApi.hpp",
        "#ifndef GRAVITY_RUNTIME_INCLUDE_FRONTEND_FRONTENDMODULEAPI_HPP_\n"
        "#define GRAVITY_RUNTIME_INCLUDE_FRONTEND_FRONTENDMODULEAPI_HPP_\n"
        "typedef int (*AllowedFn)();\n"
        "#endif // GRAVITY_RUNTIME_INCLUDE_FRONTEND_FRONTENDMODULEAPI_HPP_\n"
    )
    ok, errors, _ = run_repo_policy(tmp_path)
    assert ok
    assert not errors
