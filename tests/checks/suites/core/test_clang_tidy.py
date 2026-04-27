# @file tests/checks/suites/core/test_clang_tidy.py
# @author Luis1454
# @project BLITZAR
# @brief Automated verification assets for BLITZAR quality gates.

from __future__ import annotations

import json
import subprocess
from pathlib import Path
from typing import Any, cast

from python_tools.ci.clang_tidy import ClangTidyCheck
from python_tools.core.models import CheckContext


# @brief Defines the fake runner type contract.
# @param None This contract does not take explicit parameters.
# @note Keep construction and side effects explicit for deterministic quality gates.
class _FakeRunner:
    # @brief Documents the init operation contract.
    # @param diff_output Input value used by this contract.
    # @param diff_rc Input value used by this contract.
    # @param timeout_analyzer_once Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def __init__(self, diff_output: str = "", diff_rc: int = 0, timeout_analyzer_once: bool = False) -> None:
        self.calls: list[tuple[list[str], Path | None]] = []
        self.diff_output = diff_output
        self.diff_rc = diff_rc
        self.timeout_analyzer_once = timeout_analyzer_once
        self._timed_out = False

    # @brief Documents the run operation contract.
    # @param args Input value used by this contract.
    # @param timeout Input value used by this contract.
    # @param cwd Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def run(
        self,
        args: list[str],
        timeout: int | None = None,
        cwd: Path | None = None,
    ) -> subprocess.CompletedProcess[str]:
        del timeout
        self.calls.append((args, cwd))
        if args[:3] == ["git", "diff", "--name-only"]:
            stderr = "bad rev" if self.diff_rc else ""
            return subprocess.CompletedProcess(args, self.diff_rc, stdout=self.diff_output, stderr=stderr)
        return subprocess.CompletedProcess(args, 0, stdout="tidy ok\n", stderr="")

    # @brief Documents the run with heartbeat operation contract.
    # @param args Input value used by this contract.
    # @param timeout Input value used by this contract.
    # @param cwd Input value used by this contract.
    # @param heartbeat_seconds Input value used by this contract.
    # @param on_heartbeat Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def run_with_heartbeat(
        self,
        args: list[str],
        timeout: int | None = None,
        cwd: Path | None = None,
        heartbeat_seconds: int = 30,
        on_heartbeat=None,
    ) -> subprocess.CompletedProcess[str]:
        del heartbeat_seconds, on_heartbeat
        self.calls.append((args, cwd))
        checks = next((arg for arg in args if arg.startswith("-checks=")), "")
        if self.timeout_analyzer_once and "clang-analyzer-" in checks and not self._timed_out:
            self._timed_out = True
            raise subprocess.TimeoutExpired(args, timeout if timeout is not None else 1)
        del timeout
        return subprocess.CompletedProcess(args, 0, stdout="tidy ok\n", stderr="")


# @brief Documents the write compile db operation contract.
# @param build_dir Input value used by this contract.
# @param files Input value used by this contract.
# @param compiler Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def _write_compile_db(build_dir: Path, files: list[Path], compiler: str = "clang++") -> None:
    build_dir.mkdir(parents=True, exist_ok=True)
    entries = [{"directory": str(path.parent), "command": f"{compiler} -c {path.name}", "file": str(path)} for path in files]
    (build_dir / "compile_commands.json").write_text(json.dumps(entries), encoding="utf-8")


# @brief Documents the context operation contract.
# @param root Input value used by this contract.
# @param build_dir Input value used by this contract.
# @param diff_base Input value used by this contract.
# @param diff_target Input value used by this contract.
# @param header_filter Input value used by this contract.
# @param file_timeout_sec Input value used by this contract.
# @param timeout_fallback_checks Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def _context(
    root: Path,
    build_dir: Path,
    diff_base: str = "",
    diff_target: str = "",
    header_filter: str = "",
    file_timeout_sec: int = 0,
    timeout_fallback_checks: str = "-*,bugprone-unused-return-value",
) -> CheckContext:
    return CheckContext(
        root=root,
        build_dir=build_dir,
        clang_tidy_binary="clang-tidy",
        clang_tidy_diff_base=diff_base,
        clang_tidy_diff_target=diff_target,
        clang_tidy_header_filter=header_filter,
        clang_tidy_file_timeout_sec=file_timeout_sec,
        clang_tidy_timeout_fallback_checks=timeout_fallback_checks,
        paths=("runtime/src/server",),
    )


# @brief Documents the make source operation contract.
# @param root Input value used by this contract.
# @param name Input value used by this contract.
# @param suffix Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def _make_source(root: Path, name: str, suffix: str = ".cpp") -> Path:
    path = root / "runtime" / "src" / "server" / f"{name}{suffix}"
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(f"int {name.replace('-', '_')}();\n", encoding="utf-8")
    return path


# @brief Documents the make check operation contract.
# @param monkeypatch Input value used by this contract.
# @param runner Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def _make_check(monkeypatch, runner: _FakeRunner | None = None) -> tuple[ClangTidyCheck, _FakeRunner]:
    fake_runner = runner or _FakeRunner()
    check = ClangTidyCheck()
    cast(Any, check)._runner = fake_runner
    monkeypatch.setattr("shutil.which", lambda binary: f"/usr/bin/{binary}")
    return check, fake_runner


# @brief Documents the tidy calls operation contract.
# @param runner Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def _tidy_calls(runner: _FakeRunner) -> list[list[str]]:
    return [args for args, _ in runner.calls if args[0].endswith("clang-tidy")]


# @brief Documents the test clang tidy runs only changed compile entries operation contract.
# @param monkeypatch Input value used by this contract.
# @param tmp_path Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def test_clang_tidy_runs_only_changed_compile_entries(monkeypatch, tmp_path: Path) -> None:
    changed = _make_source(tmp_path, "changed")
    unchanged = _make_source(tmp_path, "unchanged")
    build_dir = tmp_path / "build-quality"
    _write_compile_db(build_dir, [changed, unchanged])
    check, runner = _make_check(monkeypatch, _FakeRunner(diff_output="runtime/src/server/changed.cpp\n"))

    result = check.run(_context(tmp_path, build_dir, diff_base="origin/main", diff_target="HEAD", header_filter="project"))

    assert result.ok
    tidy_calls = _tidy_calls(runner)
    assert len(tidy_calls) == 1
    assert str(changed) in tidy_calls[0]
    assert "-header-filter=project" in tidy_calls[0]
    assert "clang-tidy-logs" in result.success_message


# @brief Documents the test clang tidy skips clean diffs operation contract.
# @param monkeypatch Input value used by this contract.
# @param tmp_path Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def test_clang_tidy_skips_clean_diffs(monkeypatch, tmp_path: Path) -> None:
    source = _make_source(tmp_path, "file")
    build_dir = tmp_path / "build-quality"
    _write_compile_db(build_dir, [source])
    check, _ = _make_check(monkeypatch, _FakeRunner(diff_output="docs/quality/quality-overview.md\n"))

    result = check.run(_context(tmp_path, build_dir, diff_base="origin/main"))

    assert result.ok
    assert result.success_message == "clang-tidy skipped (no matching changed files)"


# @brief Documents the test clang tidy expands scope for header only diffs operation contract.
# @param monkeypatch Input value used by this contract.
# @param tmp_path Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def test_clang_tidy_expands_scope_for_header_only_diffs(monkeypatch, tmp_path: Path) -> None:
    first = _make_source(tmp_path, "first")
    second = _make_source(tmp_path, "second")
    _make_source(tmp_path, "shared", ".hpp")
    build_dir = tmp_path / "build-quality"
    _write_compile_db(build_dir, [first, second])
    check, runner = _make_check(monkeypatch, _FakeRunner(diff_output="runtime/src/server/shared.hpp\n"))

    result = check.run(_context(tmp_path, build_dir, diff_base="origin/main"))

    assert result.ok
    assert len(_tidy_calls(runner)) == 2
    assert "clang-tidy skipped" not in result.success_message


# @brief Documents the test clang tidy reports git diff failures operation contract.
# @param monkeypatch Input value used by this contract.
# @param tmp_path Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def test_clang_tidy_reports_git_diff_failures(monkeypatch, tmp_path: Path) -> None:
    source = _make_source(tmp_path, "file")
    build_dir = tmp_path / "build-quality"
    _write_compile_db(build_dir, [source])
    check, _ = _make_check(monkeypatch, _FakeRunner(diff_rc=1))

    result = check.run(_context(tmp_path, build_dir, diff_base="origin/main"))

    assert not result.ok
    assert any("git diff failed: bad rev" in error for error in result.errors)


# @brief Documents the test clang tidy uses windows llvm fallback operation contract.
# @param monkeypatch Input value used by this contract.
# @param tmp_path Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def test_clang_tidy_uses_windows_llvm_fallback(monkeypatch, tmp_path: Path) -> None:
    source = _make_source(tmp_path, "file")
    build_dir = tmp_path / "build-quality"
    _write_compile_db(build_dir, [source])
    runner = _FakeRunner()
    check = ClangTidyCheck()
    cast(Any, check)._runner = runner
    fallback = tmp_path / "llvm" / "bin" / "clang-tidy.exe"
    fallback.parent.mkdir(parents=True, exist_ok=True)
    fallback.write_text("", encoding="utf-8")
    monkeypatch.setattr("shutil.which", lambda binary: None)
    monkeypatch.setattr("platform.system", lambda: "Windows")
    monkeypatch.setattr("python_tools.ci.clang_tidy.WINDOWS_LLVM_CLANG_TIDY_CANDIDATES", (fallback,))

    result = check.run(_context(tmp_path, build_dir))

    assert result.ok
    assert len([args for args, _ in runner.calls if args[0] == str(fallback)]) == 1


# @brief Documents the test clang tidy adds cl driver mode for msvc compile db operation contract.
# @param monkeypatch Input value used by this contract.
# @param tmp_path Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def test_clang_tidy_adds_cl_driver_mode_for_msvc_compile_db(monkeypatch, tmp_path: Path) -> None:
    source = _make_source(tmp_path, "file")
    build_dir = tmp_path / "build-quality"
    _write_compile_db(build_dir, [source], compiler="cl.exe")
    check, runner = _make_check(monkeypatch)

    result = check.run(_context(tmp_path, build_dir))

    assert result.ok
    tidy_calls = _tidy_calls(runner)
    assert len(tidy_calls) == 1
    assert "--extra-arg-before=--driver-mode=cl" in tidy_calls[0]


# @brief Documents the test clang tidy auto job cap is visible in progress operation contract.
# @param monkeypatch Input value used by this contract.
# @param tmp_path Input value used by this contract.
# @param capsys Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def test_clang_tidy_auto_job_cap_is_visible_in_progress(monkeypatch, tmp_path: Path, capsys) -> None:
    files = [_make_source(tmp_path, f"file_{index}") for index in range(10)]
    build_dir = tmp_path / "build-quality"
    _write_compile_db(build_dir, files)
    check, _ = _make_check(monkeypatch)
    monkeypatch.setattr("os.cpu_count", lambda: 32)

    result = check.run(_context(tmp_path, build_dir))

    captured = capsys.readouterr()
    assert result.ok
    assert "[clang-tidy] analyzing 10 file(s) with jobs=6" in captured.out


# @brief Documents the test clang tidy emits progress in terminal operation contract.
# @param monkeypatch Input value used by this contract.
# @param tmp_path Input value used by this contract.
# @param capsys Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def test_clang_tidy_emits_progress_in_terminal(monkeypatch, tmp_path: Path, capsys) -> None:
    source = _make_source(tmp_path, "file")
    build_dir = tmp_path / "build-quality"
    _write_compile_db(build_dir, [source])
    check, _ = _make_check(monkeypatch)

    result = check.run(_context(tmp_path, build_dir))

    captured = capsys.readouterr()
    assert result.ok
    assert "[clang-tidy] analyzing 1 file(s)" in captured.out
    assert "[clang-tidy] [1/1] start runtime" in captured.out
    assert "file.cpp" in captured.out
    assert "[clang-tidy] [1/1] done runtime" in captured.out


# @brief Documents the test clang tidy timeout falls back to light checks operation contract.
# @param monkeypatch Input value used by this contract.
# @param tmp_path Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def test_clang_tidy_timeout_falls_back_to_light_checks(monkeypatch, tmp_path: Path) -> None:
    source = _make_source(tmp_path, "file")
    build_dir = tmp_path / "build-quality"
    _write_compile_db(build_dir, [source])
    check, runner = _make_check(monkeypatch, _FakeRunner(timeout_analyzer_once=True))

    result = check.run(_context(tmp_path, build_dir, file_timeout_sec=1))

    assert result.ok
    assert any("analyzer timed out after 1s" in warning for warning in result.warnings)
    tidy_calls = _tidy_calls(runner)
    assert len(tidy_calls) == 2
    assert "-checks=-*,clang-analyzer-*,bugprone-unused-return-value" in tidy_calls[0]
    assert "-checks=-*,bugprone-unused-return-value" in tidy_calls[1]


# @brief Documents the test clang tidy timeout without fallback fails operation contract.
# @param monkeypatch Input value used by this contract.
# @param tmp_path Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def test_clang_tidy_timeout_without_fallback_fails(monkeypatch, tmp_path: Path) -> None:
    source = _make_source(tmp_path, "file")
    build_dir = tmp_path / "build-quality"
    _write_compile_db(build_dir, [source])
    check, _ = _make_check(monkeypatch, _FakeRunner(timeout_analyzer_once=True))

    result = check.run(_context(tmp_path, build_dir, file_timeout_sec=1, timeout_fallback_checks=""))

    assert not result.ok
    assert any("clang-tidy timed out after 1s" in error for error in result.errors)

