#!/usr/bin/env python3
# @file tests/checks/suites/policy/test_cpp_api_documentation_policy.py
# @author Luis1454
# @project BLITZAR
# @brief Automated verification assets for BLITZAR quality gates.

from __future__ import annotations

from pathlib import Path

from python_tools.core.models import CheckContext
from python_tools.policies.cpp_api_documentation_policy import CppApiDocumentationCheck


# @brief Documents the write operation contract.
# @param path Input value used by this contract.
# @param content Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def _write(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


# @brief Documents the run operation contract.
# @param root Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def _run(root: Path) -> tuple[bool, list[str]]:
    result = CppApiDocumentationCheck().run(CheckContext(root=root, target_lines=200, hard_lines=300))
    return result.ok, result.errors


# @brief Documents the cpp api documentation policy accepts well documented public api operation contract.
# @param tmp_path Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def test_cpp_api_documentation_policy_accepts_well_documented_public_api(tmp_path: Path) -> None:
    _write(
        tmp_path / "modules" / "qt" / "include" / "ui" / "example.hpp",
        "/*\n"
        " * @file modules/qt/include/ui/example.hpp\n"
        " * @brief Example public API header.\n"
        " */\n"
        "#ifndef BLITZAR_MODULES_QT_INCLUDE_UI_EXAMPLE_HPP_\n"
        "#define BLITZAR_MODULES_QT_INCLUDE_UI_EXAMPLE_HPP_\n"
        "\n"
        "/*\n"
        " * @brief Example widget type contract.\n"
        " * @param None This contract does not take explicit parameters.\n"
        " * @return Not applicable; this block documents a type contract.\n"
        " * @note Keep ownership and runtime side effects explicit.\n"
        " */\n"
        "class ExampleWidget {\n"
        "public:\n"
        "    /*\n"
        "     * @brief Performs the public action.\n"
        "     * @param None This contract does not take explicit parameters.\n"
        "     * @return No return value.\n"
        "     * @note Keep side effects explicit.\n"
        "     */\n"
        "    void perform();\n"
        "};\n"
        "\n"
        "#endif // BLITZAR_MODULES_QT_INCLUDE_UI_EXAMPLE_HPP_\n",
    )
    ok, errors = _run(tmp_path)
    assert ok
    assert errors == []


# @brief Documents the cpp api documentation policy rejects missing file header operation contract.
# @param tmp_path Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def test_cpp_api_documentation_policy_rejects_missing_file_header(tmp_path: Path) -> None:
    _write(
        tmp_path / "modules" / "qt" / "include" / "ui" / "example.hpp",
        "#ifndef BLITZAR_MODULES_QT_INCLUDE_UI_EXAMPLE_HPP_\n"
        "#define BLITZAR_MODULES_QT_INCLUDE_UI_EXAMPLE_HPP_\n"
        "class ExampleWidget {\n"
        "public:\n"
        "    void perform();\n"
        "};\n"
        "#endif\n",
    )
    ok, errors = _run(tmp_path)
    assert not ok
    assert any("responsibility block containing @file/@brief" in error for error in errors)


# @brief Documents the cpp api documentation policy rejects missing class block operation contract.
# @param tmp_path Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def test_cpp_api_documentation_policy_rejects_missing_class_block(tmp_path: Path) -> None:
    _write(
        tmp_path / "modules" / "qt" / "include" / "ui" / "example.hpp",
        "/*\n"
        " * @file modules/qt/include/ui/example.hpp\n"
        " * @brief Example public API header.\n"
        " */\n"
        "#ifndef BLITZAR_MODULES_QT_INCLUDE_UI_EXAMPLE_HPP_\n"
        "#define BLITZAR_MODULES_QT_INCLUDE_UI_EXAMPLE_HPP_\n"
        "class ExampleWidget {\n"
        "public:\n"
        "    /*\n"
        "     * @brief Performs the public action.\n"
        "     * @param None This contract does not take explicit parameters.\n"
        "     * @return No return value.\n"
        "     * @note Keep side effects explicit.\n"
        "     */\n"
        "    void perform();\n"
        "};\n"
        "#endif\n",
    )
    ok, errors = _run(tmp_path)
    assert not ok
    assert any("class definition must be preceded by a @brief documentation block" in error for error in errors)


# @brief Documents the cpp api documentation policy rejects missing method block operation contract.
# @param tmp_path Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def test_cpp_api_documentation_policy_rejects_missing_method_block(tmp_path: Path) -> None:
    _write(
        tmp_path / "modules" / "qt" / "include" / "ui" / "example.hpp",
        "/*\n"
        " * @file modules/qt/include/ui/example.hpp\n"
        " * @brief Example public API header.\n"
        " */\n"
        "#ifndef BLITZAR_MODULES_QT_INCLUDE_UI_EXAMPLE_HPP_\n"
        "#define BLITZAR_MODULES_QT_INCLUDE_UI_EXAMPLE_HPP_\n"
        "/*\n"
        " * @brief Example widget type contract.\n"
        " * @param None This contract does not take explicit parameters.\n"
        " * @return Not applicable; this block documents a type contract.\n"
        " * @note Keep ownership and runtime side effects explicit.\n"
        " */\n"
        "class ExampleWidget {\n"
        "public:\n"
        "    void perform();\n"
        "};\n"
        "#endif\n",
    )
    ok, errors = _run(tmp_path)
    assert not ok
    assert any("public member function must be preceded by a @brief documentation block" in error for error in errors)
