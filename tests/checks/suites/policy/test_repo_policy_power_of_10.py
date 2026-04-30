#!/usr/bin/env python3
# @file tests/checks/suites/policy/test_repo_policy_power_of_10.py
# @author Luis1454
# @project BLITZAR
# @brief Automated verification assets for BLITZAR quality gates.

from __future__ import annotations

from pathlib import Path

from tests.checks.suites.policy.test_repo_policy import _run, _write
from tests.checks.suites.support.path_specs import ENGINE_SERVER_DIR, RUNTIME_SERVER_DIR, cpp_file


# @brief Documents the test repo policy rejects while true in prod cpp operation contract.
# @param tmp_path Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def test_repo_policy_rejects_while_true_in_prod_cpp(tmp_path: Path) -> None:
    _write(tmp_path / cpp_file(RUNTIME_SERVER_DIR, "bad_while_true"), "int f() { while (true) { return 1; } }\n")
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert not ok
    assert any("Power of 10 rule 2 forbids open-ended while(true) loops" in error for error in errors)


# @brief Documents the test repo policy rejects non structural macro in prod cpp operation contract.
# @param tmp_path Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def test_repo_policy_rejects_non_structural_macro_in_prod_cpp(tmp_path: Path) -> None:
    _write(tmp_path / cpp_file(ENGINE_SERVER_DIR, "bad_macro"), "#define BAD_LIMIT 8\n")
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert not ok
    assert any("preprocessor macros are forbidden" in error for error in errors)


# @brief Documents the test repo policy accepts include guard in header operation contract.
# @param tmp_path Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def test_repo_policy_accepts_include_guard_in_header(tmp_path: Path) -> None:
    _write(
        tmp_path / "engine" / "include" / "ok.hpp",
        "#ifndef BLITZAR_ENGINE_INCLUDE_OK_HPP_\n"
        "#define BLITZAR_ENGINE_INCLUDE_OK_HPP_\n"
        "\n"
        "struct Ok {};\n"
        "\n"
        "#endif // BLITZAR_ENGINE_INCLUDE_OK_HPP_\n",
    )
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert ok
    assert not errors


# @brief Documents the test repo policy rejects function pointer typedef outside abi boundary operation contract.
# @param tmp_path Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def test_repo_policy_rejects_function_pointer_typedef_outside_abi_boundary(tmp_path: Path) -> None:
    _write(tmp_path / cpp_file(RUNTIME_SERVER_DIR, "bad_fn_ptr"), "typedef int (*BadFn)();\n")
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert not ok
    assert any("Power of 10 rule 9 forbids function pointer typedefs" in error for error in errors)


# @brief Documents the test repo policy accepts function pointer typedef in explicit abi boundary operation contract.
# @param tmp_path Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def test_repo_policy_accepts_function_pointer_typedef_in_explicit_abi_boundary(tmp_path: Path) -> None:
    _write(
        tmp_path / "runtime" / "include" / "client" / "ClientModuleApi.hpp",
        "#ifndef BLITZAR_RUNTIME_INCLUDE_CLIENT_CLIENTMODULEAPI_HPP_\n"
        "#define BLITZAR_RUNTIME_INCLUDE_CLIENT_CLIENTMODULEAPI_HPP_\n"
        "typedef int (*AllowedFn)();\n"
        "#endif // BLITZAR_RUNTIME_INCLUDE_CLIENT_CLIENTMODULEAPI_HPP_\n"
    )
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert ok
    assert not errors
