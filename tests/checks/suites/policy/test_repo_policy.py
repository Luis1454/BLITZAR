from __future__ import annotations

from pathlib import Path

import pytest

from python_tools.core.models import CheckContext
from python_tools.policies.repo_policy import RepoPolicyCheck
from tests.checks.suites.support.path_specs import (
    ENGINE_CONFIG_DIR,
    ENGINE_SERVER_DIR,
    MODULES_QT_UI_DIR,
    RUNTIME_SERVER_DIR,
    TESTS_UNIT_DIR,
    cpp_file,
)


def _write(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def _run(root: Path, allowlist: Path) -> tuple[bool, list[str], list[str]]:
    context = CheckContext(root=root, allowlist=allowlist, target_lines=200, hard_lines=300)
    result = RepoPolicyCheck().run(context)
    return result.ok, result.errors, result.warnings


@pytest.mark.parametrize(
    ("path", "content", "expected"),
    [
        (cpp_file(ENGINE_SERVER_DIR, "bad"), "using Alias = int;\n", "'using' is forbidden in C++ sources"),
        (cpp_file(ENGINE_CONFIG_DIR, "bad"), "namespace a::b {\n}\n", "nested namespace declaration (A::B) is forbidden"),
        (cpp_file(RUNTIME_SERVER_DIR, "bad"), "namespace gravity_internal_bad {\n}\n", "gravity_internal_* namespace is forbidden"),
        (
            cpp_file(ENGINE_SERVER_DIR, "bad_namespace"),
            "namespace {\nint g = 1;\n}\n",
            "unnamed namespace is forbidden in production paths",
        ),
        (
            cpp_file(ENGINE_SERVER_DIR, "bad_goto"),
            "int f() { goto fail; fail: return 0; }\n",
            "Power of 10 rule 1 forbids goto",
        ),
        (
            cpp_file(RUNTIME_SERVER_DIR, "bad_longjmp"),
            "int f() { return longjmp(buf, 1); }\n",
            "Power of 10 rule 1 forbids setjmp/longjmp",
        ),
        (
            cpp_file(ENGINE_CONFIG_DIR, "bad_do_while"),
            "int f() { do { return 1; } while (false); }\n",
            "Power of 10 rule 1 forbids do-while",
        ),
        (
            cpp_file(ENGINE_CONFIG_DIR, "bad_preprocessor_conditional"),
            "#ifdef _WIN32\nint f() { return 1; }\n#endif\n",
            "preprocessor conditionals are forbidden in C/C++ sources",
        ),
    ],
)
def test_repo_policy_rejects_cpp_patterns(tmp_path: Path, path: Path, content: str, expected: str) -> None:
    _write(tmp_path / path, content)
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert not ok
    assert any(expected in error for error in errors)


@pytest.mark.parametrize(
    ("stem", "content", "should_pass"),
    [
        ("bad_layout", "void build() {\n    QVBoxLayout &layout = *new QVBoxLayout(this);\n}\n", False),
        ("good_layout", "void build() {\n    auto *layout = new QVBoxLayout(this);\n    layout->setSpacing(6);\n}\n", True),
    ],
)
def test_repo_policy_enforces_qt_layout_ownership(tmp_path: Path, stem: str, content: str, should_pass: bool) -> None:
    _write(tmp_path / cpp_file(MODULES_QT_UI_DIR, stem), content)
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert ok is should_pass
    if should_pass:
        assert not errors
        return
    assert any("Qt '*new + reference' ownership pattern is forbidden" in error for error in errors)


def test_repo_policy_warns_on_stale_allowlist_entry(tmp_path: Path) -> None:
    sample_path = cpp_file(TESTS_UNIT_DIR, "sample")
    _write(tmp_path / sample_path, "int main() { return 0; }\n")
    allowlist = tmp_path / "allowlist.txt"
    allowlist.write_text(f"{sample_path.as_posix()}\n", encoding="utf-8")
    ok, _, warnings = _run(tmp_path, allowlist)
    assert ok
    assert any("allowlist entry not needed anymore" in warning for warning in warnings)


def test_repo_policy_rejects_unnamed_namespace_in_tests_cpp(tmp_path: Path) -> None:
    _write(tmp_path / cpp_file(TESTS_UNIT_DIR, "bad_namespace"), "namespace {\nint g = 1;\n}\n")
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert not ok
    assert any("unnamed namespace is forbidden" in error for error in errors)


def test_repo_policy_accepts_header_include_guard(tmp_path: Path) -> None:
    _write(
        tmp_path / "engine" / "include" / "ok.hpp",
        "#ifndef GRAVITY_ENGINE_INCLUDE_OK_HPP_\n"
        "#define GRAVITY_ENGINE_INCLUDE_OK_HPP_\n"
        "\n"
        "struct Ok {};\n"
        "\n"
        "#endif // GRAVITY_ENGINE_INCLUDE_OK_HPP_\n",
    )
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert ok
    assert not errors


def test_repo_policy_rejects_pragma_once_in_header(tmp_path: Path) -> None:
    _write(tmp_path / "engine" / "include" / "bad.hpp", "#pragma once\nstruct Bad {};\n")
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert not ok
    assert any("#pragma once is forbidden" in error for error in errors)


def test_repo_policy_rejects_json_above_hard_limit(tmp_path: Path) -> None:
    oversize = "{\n" + "\"k\":0,\n" * 305 + "\"end\":1\n}\n"
    _write(tmp_path / "docs" / "quality" / "oversize.json", oversize)
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert not ok
    assert any("oversize.json" in error and "strong alert threshold" in error for error in errors)


def test_repo_policy_warns_when_file_exceeds_target_but_not_hard_limit(tmp_path: Path) -> None:
    content = "line\n" * 205
    _write(tmp_path / "docs" / "quality" / "target_warning.md", content)
    ok, errors, warnings = _run(tmp_path, tmp_path / "allowlist.txt")
    assert ok
    assert not errors
    assert any("target_warning.md: 205 lines exceeds target 200" in warning for warning in warnings)


def test_repo_policy_ignores_rust_target_but_not_unrelated_target_dir(tmp_path: Path) -> None:
    _write(tmp_path / "rust" / "target" / "generated.c", "int f(){return 0;}\n")
    _write(tmp_path / "target" / "bad.c", "int f(){return 0;}\n")
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert not ok
    assert any("target/bad.c" in error and "forbidden C/C++ extension '.c'" in error for error in errors)
    assert not any("rust/target/generated.c" in error for error in errors)


def test_repo_policy_rejects_evidence_workflow_without_prod_profile(tmp_path: Path) -> None:
    _write(
        tmp_path / ".github" / "workflows" / "release-lane.yml",
        "jobs:\n"
        "  release:\n"
        "    steps:\n"
        "      - name: Configure\n"
        "        run: |\n"
        "          cmake -S . -B build -G Ninja \\\n"
        "            -DCMAKE_BUILD_TYPE=Release\n",
    )
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert not ok
    assert any("evidence configure command must include -DGRAVITY_PROFILE=prod" in error for error in errors)


def test_repo_policy_ignores_non_evidence_dev_workflow(tmp_path: Path) -> None:
    _write(
        tmp_path / ".github" / "workflows" / "pr-fast.yml",
        "jobs:\n"
        "  dev:\n"
        "    steps:\n"
        "      - name: Configure\n"
        "        run: |\n"
        "          cmake -S . -B build-dev-mod -G Ninja \\\n"
        "            -DGRAVITY_PROFILE=dev\n",
    )
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert ok
    assert not errors


def test_repo_policy_rejects_evidence_ctest_without_no_tests_guard(tmp_path: Path) -> None:
    _write(
        tmp_path / ".github" / "workflows" / "nightly-full.yml",
        "jobs:\n"
        "  nightly:\n"
        "    steps:\n"
        "      - name: Run tests\n"
        "        run: ctest --test-dir build -R \"TST_QLT_REPO_001_\"\n",
    )
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert not ok
    assert any("CI ctest command must include --no-tests=error" in error for error in errors)


def test_repo_policy_accepts_evidence_ctest_with_no_tests_guard(tmp_path: Path) -> None:
    _write(
        tmp_path / ".github" / "workflows" / "nightly-full.yml",
        "jobs:\n"
        "  nightly:\n"
        "    steps:\n"
        "      - name: Run tests\n"
        "        run: ctest --test-dir build --output-on-failure --timeout 180 --no-tests=error -R \"TST_QLT_REPO_001_\"\n",
    )
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert ok
    assert not errors


def test_repo_policy_rejects_legacy_ctest_selector_prefix(tmp_path: Path) -> None:
    _write(
        tmp_path / ".github" / "workflows" / "nightly-full.yml",
        "jobs:\n"
        "  nightly:\n"
        "    steps:\n"
        "      - name: Run tests\n"
        "        run: ctest --test-dir build --output-on-failure --timeout 180 --no-tests=error -R \"ConfigArgsTest\\\\.TST_UNT_CONF_\"\n",
    )
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert not ok
    assert any("CI ctest selector must use normalized TST_* ids" in error for error in errors)


def test_repo_policy_accepts_normalized_ctest_selector_prefix(tmp_path: Path) -> None:
    _write(
        tmp_path / ".github" / "workflows" / "nightly-full.yml",
        "jobs:\n"
        "  nightly:\n"
        "    steps:\n"
        "      - name: Run tests\n"
        "        run: ctest --test-dir build --output-on-failure --timeout 180 --no-tests=error -R \"TST_UNT_CONF_|TST_QLT_REPO_001_\"\n",
    )
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert ok
    assert not errors


def test_repo_policy_ignores_venv_directory_and_cache_artifacts(tmp_path: Path) -> None:
    """Anti-recurrence test: verify .venv and cache dirs never enter policy scan."""
    _write(tmp_path / ".venv" / "lib" / "python3.12" / "site-packages" / "numpy" / "__init__.py", "# numpy\n")
    _write(tmp_path / ".tox" / "py312" / "lib" / "python3.12" / "site-packages" / "pytest" / "__init__.py", "# pytest\n")
    _write(tmp_path / ".pytest_cache" / "v" / ".gitkeep", "")
    _write(tmp_path / ".ruff_cache" / "ruff.db", "")
    _write(tmp_path / ".mypy_cache" / "json.data", "")
    _write(tmp_path / "engine" / "src" / "good.cpp", "int main() { return 0; }\n")
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert ok
    assert not errors
    assert not any(".venv" in error or ".tox" in error or "site-packages" in error for error in errors)


def test_repo_policy_explicitly_ignores_rust_target_path_not_naked_target(tmp_path: Path) -> None:
    """Anti-recurrence test: verify rust/target is skipped but naked target/ is not."""
    _write(tmp_path / "rust" / "target" / "release" / "artifact.lib", "")
    _write(tmp_path / "tools" / "target" / "bad_file.c", "int badFunc() { return 0; }\n")
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert not ok
    assert any("tools/target/bad_file.c" in error and "forbidden C/C++ extension" in error for error in errors)
    assert not any("rust/target" in error for error in errors)


def test_repo_policy_workflow_forbids_failure_masking_with_shell_alternative(tmp_path: Path) -> None:
    """Anti-recurrence test: verify || shell fallback on build commands is rejected."""
    _write(
        tmp_path / ".github" / "workflows" / "pr-fast.yml",
        "jobs:\n"
        "  build:\n"
        "    steps:\n"
        "      - name: Build with fallback\n"
        "        run: cmake --build build-dev-mod || echo 'build failed but ignoring'\n",
    )
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert not ok
    assert any("mask build/test command failures with shell fallbacks" in error for error in errors)

