#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

from checks.tests.path_specs import (
    ENGINE_BACKEND_DIR,
    ENGINE_CONFIG_DIR,
    RUNTIME_BACKEND_DIR,
    TESTS_UNIT_DIR,
    cpp_file,
)
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
    _write(tmp_path / cpp_file(ENGINE_BACKEND_DIR, "bad"), "using Alias = int;\n")
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert not ok
    assert any("'using' is forbidden in C++ sources" in error for error in errors)


def test_repo_policy_rejects_nested_namespace_syntax(tmp_path: Path) -> None:
    _write(tmp_path / cpp_file(ENGINE_CONFIG_DIR, "bad"), "namespace a::b {\n}\n")
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert not ok
    assert any("nested namespace declaration (A::B) is forbidden" in error for error in errors)


def test_repo_policy_rejects_gravity_internal_namespace(tmp_path: Path) -> None:
    _write(tmp_path / cpp_file(RUNTIME_BACKEND_DIR, "bad"), "namespace gravity_internal_bad {\n}\n")
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert not ok
    assert any("gravity_internal_* namespace is forbidden" in error for error in errors)


def test_repo_policy_rejects_unnamed_namespace_in_prod_cpp(tmp_path: Path) -> None:
    _write(tmp_path / cpp_file(ENGINE_BACKEND_DIR, "bad_namespace"), "namespace {\nint g = 1;\n}\n")
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert not ok
    assert any("unnamed namespace is forbidden in production paths" in error for error in errors)


def test_repo_policy_rejects_goto_in_prod_cpp(tmp_path: Path) -> None:
    _write(tmp_path / cpp_file(ENGINE_BACKEND_DIR, "bad_goto"), "int f() { goto fail; fail: return 0; }\n")
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert not ok
    assert any("Power of 10 rule 1 forbids goto" in error for error in errors)


def test_repo_policy_rejects_setjmp_longjmp_in_prod_cpp(tmp_path: Path) -> None:
    _write(tmp_path / cpp_file(RUNTIME_BACKEND_DIR, "bad_longjmp"), "int f() { return longjmp(buf, 1); }\n")
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert not ok
    assert any("Power of 10 rule 1 forbids setjmp/longjmp" in error for error in errors)


def test_repo_policy_rejects_do_while_in_prod_cpp(tmp_path: Path) -> None:
    _write(tmp_path / cpp_file(ENGINE_CONFIG_DIR, "bad_do_while"), "int f() { do { return 1; } while (false); }\n")
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert not ok
    assert any("Power of 10 rule 1 forbids do-while" in error for error in errors)


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


def test_repo_policy_rejects_json_above_hard_limit(tmp_path: Path) -> None:
    oversize = "{\n" + "\"k\":0,\n" * 305 + "\"end\":1\n}\n"
    _write(tmp_path / "docs" / "quality" / "oversize.json", oversize)
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert not ok
    assert any("oversize.json" in error and "exceeds hard limit" in error for error in errors)


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
        tmp_path / ".github" / "workflows" / "release-lane.yml",
        "jobs:\n"
        "  release:\n"
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
