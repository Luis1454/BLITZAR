#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

from tests.checks.suites.policy.repo_policy_test_support import run_repo_policy, write_file


def test_repo_policy_rejects_function_definition_in_header(tmp_path: Path) -> None:
    write_file(
        tmp_path / "runtime" / "include" / "protocol" / "bad.hpp",
        "class Bad {\n"
        "    public:\n"
        "        int value() {\n"
        "            return 1;\n"
        "        }\n"
        "};\n",
    )
    ok, errors, _ = run_repo_policy(tmp_path)
    assert not ok
    assert any("function definitions in headers are forbidden" in error for error in errors)


def test_repo_policy_accepts_declaration_only_header(tmp_path: Path) -> None:
    write_file(
        tmp_path / "runtime" / "include" / "protocol" / "good.hpp",
        "#ifndef GRAVITY_RUNTIME_INCLUDE_PROTOCOL_GOOD_HPP_\n"
        "#define GRAVITY_RUNTIME_INCLUDE_PROTOCOL_GOOD_HPP_\n"
        "class Good {\n"
        "    public:\n"
        "        int value() const;\n"
        "};\n"
        "#endif // GRAVITY_RUNTIME_INCLUDE_PROTOCOL_GOOD_HPP_\n",
    )
    ok, errors, _ = run_repo_policy(tmp_path)
    assert ok
    assert not errors


def test_repo_policy_accepts_header_declaration_with_default_braced_arg(tmp_path: Path) -> None:
    write_file(
        tmp_path / "runtime" / "include" / "protocol" / "good_default.hpp",
        "#ifndef GRAVITY_RUNTIME_INCLUDE_PROTOCOL_GOOD_DEFAULT_HPP_\n"
        "#define GRAVITY_RUNTIME_INCLUDE_PROTOCOL_GOOD_DEFAULT_HPP_\n"
        "#include <functional>\n"
        "class GoodDefault {\n"
        "    public:\n"
        "        bool run(const std::function<void()> &callback = {});\n"
        "};\n"
        "#endif // GRAVITY_RUNTIME_INCLUDE_PROTOCOL_GOOD_DEFAULT_HPP_\n",
    )
    ok, errors, _ = run_repo_policy(tmp_path)
    assert ok
    assert not errors


def test_repo_policy_accepts_license_comment_before_include_guard(tmp_path: Path) -> None:
    write_file(
        tmp_path / "runtime" / "include" / "protocol" / "licensed.hpp",
        "// Copyright 2026\n"
        "// Mission-critical component\n"
        "#ifndef GRAVITY_RUNTIME_INCLUDE_PROTOCOL_LICENSED_HPP_\n"
        "#define GRAVITY_RUNTIME_INCLUDE_PROTOCOL_LICENSED_HPP_\n"
        "class Licensed final {\n"
        "    public:\n"
        "        int value() const;\n"
        "};\n"
        "#endif // GRAVITY_RUNTIME_INCLUDE_PROTOCOL_LICENSED_HPP_\n",
    )
    ok, errors, _ = run_repo_policy(tmp_path)
    assert ok
    assert not errors
