#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

from tests.checks.suites.policy.test_repo_policy import _run, _write


def test_repo_policy_rejects_function_definition_in_header(tmp_path: Path) -> None:
    _write(
        tmp_path / "runtime" / "include" / "protocol" / "bad.hpp",
        "class Bad {\n"
        "    public:\n"
        "        int value() {\n"
        "            return 1;\n"
        "        }\n"
        "};\n",
    )
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert not ok
    assert any("function definitions in headers are forbidden" in error for error in errors)


def test_repo_policy_accepts_declaration_only_header(tmp_path: Path) -> None:
    _write(
        tmp_path / "runtime" / "include" / "protocol" / "good.hpp",
        "#ifndef RUNTIME_INCLUDE_PROTOCOL_GOOD_HPP_\n"
        "#define RUNTIME_INCLUDE_PROTOCOL_GOOD_HPP_\n"
        "\n"
        "class Good {\n"
        "    public:\n"
        "        int value() const;\n"
        "};\n"
        "\n"
        "#endif // RUNTIME_INCLUDE_PROTOCOL_GOOD_HPP_\n",
    )
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert ok
    assert not errors


def test_repo_policy_accepts_documented_header_before_include_guard(tmp_path: Path) -> None:
    _write(
        tmp_path / "runtime" / "include" / "protocol" / "documented.hpp",
        "// File: runtime/include/protocol/documented.hpp\n"
        "// Purpose: Documents the protocol test header surface.\n"
        "#ifndef RUNTIME_INCLUDE_PROTOCOL_DOCUMENTED_HPP_\n"
        "#define RUNTIME_INCLUDE_PROTOCOL_DOCUMENTED_HPP_\n"
        "\n"
        "class Documented {\n"
        "    public:\n"
        "        int value() const;\n"
        "};\n"
        "\n"
        "#endif // RUNTIME_INCLUDE_PROTOCOL_DOCUMENTED_HPP_\n",
    )
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert ok
    assert not errors


def test_repo_policy_accepts_header_declaration_with_default_braced_arg(tmp_path: Path) -> None:
    _write(
        tmp_path / "runtime" / "include" / "protocol" / "good_default.hpp",
        "#ifndef RUNTIME_INCLUDE_PROTOCOL_GOOD_DEFAULT_HPP_\n"
        "#define RUNTIME_INCLUDE_PROTOCOL_GOOD_DEFAULT_HPP_\n"
        "\n"
        "#include <functional>\n"
        "\n"
        "class GoodDefault {\n"
        "    public:\n"
        "        bool run(const std::function<void()> &callback = {});\n"
        "};\n"
        "\n"
        "#endif // RUNTIME_INCLUDE_PROTOCOL_GOOD_DEFAULT_HPP_\n",
    )
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert ok
    assert not errors


def test_repo_policy_accepts_alignas_struct_declaration(tmp_path: Path) -> None:
    _write(
        tmp_path / "engine" / "include" / "physics" / "good_struct.hpp",
        "#ifndef ENGINE_INCLUDE_PHYSICS_GOOD_STRUCT_HPP_\n"
        "#define ENGINE_INCLUDE_PHYSICS_GOOD_STRUCT_HPP_\n"
        "\n"
        "struct alignas(16) GoodStruct {\n"
        "    float x;\n"
        "};\n"
        "\n"
        "#endif // ENGINE_INCLUDE_PHYSICS_GOOD_STRUCT_HPP_\n",
    )
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert ok
    assert not errors
