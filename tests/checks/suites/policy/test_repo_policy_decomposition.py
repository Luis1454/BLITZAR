# File: tests/checks/suites/policy/test_repo_policy_decomposition.py
# Purpose: Verification coverage for the BLITZAR quality gate.

from __future__ import annotations

from pathlib import Path

from tests.checks.suites.policy.test_repo_policy import _run, _write
from tests.checks.suites.support.path_specs import ENGINE_SERVER_DIR, cpp_file


def test_repo_policy_warns_when_allowlisted_file_exceeds_strong_alert_threshold(tmp_path: Path) -> None:
    oversize = "{\n" + "\"k\":0,\n" * 305 + "\"end\":1\n}\n"
    rel = Path("docs/quality/oversize.json")
    _write(tmp_path / rel, oversize)
    allowlist = tmp_path / "allowlist.txt"
    allowlist.write_text(f"{rel.as_posix()}\n", encoding="utf-8")
    ok, errors, warnings = _run(tmp_path, allowlist)
    assert ok
    assert not errors
    assert any("strong file-size alert" in warning and rel.as_posix() in warning for warning in warnings)


def test_repo_policy_warns_on_oversized_implementation_function(tmp_path: Path) -> None:
    body = "".join(f"    value += {index};\n" for index in range(85))
    _write(
        tmp_path / cpp_file(ENGINE_SERVER_DIR, "large_function"),
        "int largeFunction() {\n"
        "    int value = 0;\n"
        f"{body}"
        "    return value;\n"
        "}\n",
    )
    ok, errors, warnings = _run(tmp_path, tmp_path / "allowlist.txt")
    assert ok
    assert not errors
    assert any("function-size warning" in warning and "large_function.cpp:1" in warning for warning in warnings)


def test_repo_policy_warns_on_multiple_substantial_functions_in_one_file(tmp_path: Path) -> None:
    function_block = "".join(f"    value += {index};\n" for index in range(38))
    content = (
        "int alpha() {\n"
        "    int value = 0;\n"
        f"{function_block}"
        "    return value;\n"
        "}\n\n"
        "int beta() {\n"
        "    int value = 0;\n"
        f"{function_block}"
        "    return value;\n"
        "}\n\n"
        "int gamma() {\n"
        "    int value = 0;\n"
        f"{function_block}"
        "    return value;\n"
        "}\n"
    )
    _write(tmp_path / cpp_file(ENGINE_SERVER_DIR, "clustered_functions"), content)
    ok, errors, warnings = _run(tmp_path, tmp_path / "allowlist.txt")
    assert ok
    assert not errors
    assert any("contains 3 substantial functions" in warning for warning in warnings)


def test_repo_policy_warns_on_excessive_function_count_in_one_file(tmp_path: Path) -> None:
    functions = []
    for index in range(9):
        functions.append(
            f"int func{index}() {{\n"
            "    return 1;\n"
            "}\n"
        )
    _write(tmp_path / cpp_file(ENGINE_SERVER_DIR, "many_functions"), "\n".join(functions))
    ok, errors, warnings = _run(tmp_path, tmp_path / "allowlist.txt")
    assert ok
    assert not errors
    assert any("function-count warning" in warning and "many_functions.cpp" in warning for warning in warnings)


def test_repo_policy_warns_on_elevated_branching_complexity(tmp_path: Path) -> None:
    conditions = "".join(
        f"    if (value == {index} && ready) {{\n        total += {index};\n    }}\n"
        for index in range(7)
    )
    _write(
        tmp_path / cpp_file(ENGINE_SERVER_DIR, "complex_function"),
        "int complexFunction(int value, bool ready) {\n"
        "    int total = 0;\n"
        f"{conditions}"
        "    return total;\n"
        "}\n",
    )
    ok, errors, warnings = _run(tmp_path, tmp_path / "allowlist.txt")
    assert ok
    assert not errors
    assert any("complexity warning" in warning and "complex_function.cpp:1" in warning for warning in warnings)
