from __future__ import annotations

import json
import subprocess
from pathlib import Path
from typing import Any, cast

from python_tools.ci.clang_tidy import ClangTidyCheck
from python_tools.core.models import CheckContext


class _FakeRunner:
    def __init__(self, diff_output: str = "", diff_rc: int = 0) -> None:
        self.calls: list[tuple[list[str], Path | None]] = []
        self.diff_output = diff_output
        self.diff_rc = diff_rc

    def run(
        self,
        args: list[str],
        timeout: int | None = None,
        cwd: Path | None = None,
    ) -> subprocess.CompletedProcess[str]:
        del timeout
        self.calls.append((args, cwd))
        if args[:3] == ["git", "diff", "--name-only"]:
            return subprocess.CompletedProcess(args, self.diff_rc, stdout=self.diff_output, stderr="bad rev" if self.diff_rc else "")
        return subprocess.CompletedProcess(args, 0, stdout="tidy ok\n", stderr="")


def _write_compile_db(build_dir: Path, files: list[Path]) -> None:
    build_dir.mkdir(parents=True, exist_ok=True)
    entries = [
        {
            "directory": str(file_path.parent),
            "command": f"clang++ -c {file_path.name}",
            "file": str(file_path),
        }
        for file_path in files
    ]
    (build_dir / "compile_commands.json").write_text(json.dumps(entries), encoding="utf-8")


def _context(
    root: Path,
    build_dir: Path,
    diff_base: str = "",
    diff_target: str = "",
    header_filter: str = "",
) -> CheckContext:
    return CheckContext(
        root=root,
        build_dir=build_dir,
        clang_tidy_binary="clang-tidy",
        clang_tidy_diff_base=diff_base,
        clang_tidy_diff_target=diff_target,
        clang_tidy_header_filter=header_filter,
        paths=("runtime/src/backend",),
    )


def test_clang_tidy_runs_only_changed_compile_entries(monkeypatch, tmp_path: Path) -> None:
    changed = tmp_path / "runtime" / "src" / "backend" / "changed.cpp"
    unchanged = tmp_path / "runtime" / "src" / "backend" / "unchanged.cpp"
    changed.parent.mkdir(parents=True, exist_ok=True)
    changed.write_text("int changed();\n", encoding="utf-8")
    unchanged.write_text("int unchanged();\n", encoding="utf-8")
    build_dir = tmp_path / "build-quality"
    _write_compile_db(build_dir, [changed, unchanged])
    runner = _FakeRunner(diff_output="runtime/src/backend/changed.cpp\n")
    check = ClangTidyCheck()
    cast(Any, check)._runner = runner
    monkeypatch.setattr("shutil.which", lambda binary: f"/usr/bin/{binary}")

    result = check.run(_context(tmp_path, build_dir, diff_base="origin/main", diff_target="HEAD", header_filter="project"))

    assert result.ok
    tidy_calls = [args for args, _ in runner.calls if args[0] == "clang-tidy"]
    assert len(tidy_calls) == 1
    assert str(changed) in tidy_calls[0]
    assert "-header-filter=project" in tidy_calls[0]
    assert "clang-tidy-logs" in result.success_message


def test_clang_tidy_skips_clean_diffs(monkeypatch, tmp_path: Path) -> None:
    source = tmp_path / "runtime" / "src" / "backend" / "file.cpp"
    source.parent.mkdir(parents=True, exist_ok=True)
    source.write_text("int sample();\n", encoding="utf-8")
    build_dir = tmp_path / "build-quality"
    _write_compile_db(build_dir, [source])
    check = ClangTidyCheck()
    cast(Any, check)._runner = _FakeRunner(diff_output="docs/quality/README.md\n")
    monkeypatch.setattr("shutil.which", lambda binary: f"/usr/bin/{binary}")

    result = check.run(_context(tmp_path, build_dir, diff_base="origin/main"))

    assert result.ok
    assert result.success_message == "clang-tidy skipped (no matching changed files)"


def test_clang_tidy_reports_git_diff_failures(monkeypatch, tmp_path: Path) -> None:
    source = tmp_path / "runtime" / "src" / "backend" / "file.cpp"
    source.parent.mkdir(parents=True, exist_ok=True)
    source.write_text("int sample();\n", encoding="utf-8")
    build_dir = tmp_path / "build-quality"
    _write_compile_db(build_dir, [source])
    check = ClangTidyCheck()
    cast(Any, check)._runner = _FakeRunner(diff_rc=1)
    monkeypatch.setattr("shutil.which", lambda binary: f"/usr/bin/{binary}")

    result = check.run(_context(tmp_path, build_dir, diff_base="origin/main"))

    assert not result.ok
    assert any("git diff failed: bad rev" in error for error in result.errors)
