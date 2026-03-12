#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import shutil
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

from python_tools.core.base_check import BaseCheck
from python_tools.core.io import PathSpec, ProcessRunner
from python_tools.core.models import CheckContext, CheckResult

DEFAULT_PATHS = (
    "tests/unit",
    "tests/int",
    "tests/support",
    "engine/src/config",
    "runtime/src/frontend",
    "runtime/src/protocol",
    "runtime/src/backend",
)
HEADER_LIKE_SUFFIXES = (".h", ".hh", ".hpp", ".hxx", ".inl")


class ClangTidyCheck(BaseCheck):
    name = "clang_tidy"
    failure_title = "clang-tidy check failed:"

    def __init__(self) -> None:
        self._runner = ProcessRunner()

    def _execute(self, context: CheckContext, result: CheckResult) -> None:
        if shutil.which(context.clang_tidy_binary) is None:
            result.add_error(f"clang-tidy executable not found: {context.clang_tidy_binary}")
            return
        files = self._load_files(context, result)
        if not files:
            result.add_error("clang-tidy check failed: no matching files found in compile database")
            return
        files = self._filter_diff_files(files, context, result)
        if result.errors:
            return
        if not files:
            result.success_message = "clang-tidy skipped (no matching changed files)"
            return
        self._run_tidy(files, context, result)
        if result.ok and not result.success_message:
            result.success_message = f"clang-tidy check passed ({len(files)} files)"

    def _load_files(self, context: CheckContext, result: CheckResult) -> list[Path]:
        build_dir = context.build_dir
        if build_dir is None:
            result.add_error("build directory is required")
            return []
        db = build_dir / "compile_commands.json"
        if not db.exists():
            result.add_error(f"compile database not found: {db}")
            return []
        try:
            entries = json.loads(db.read_text(encoding="utf-8"))
        except json.JSONDecodeError as exc:
            result.add_error(str(exc))
            return []
        path_spec = PathSpec(context.root)
        allowed_abs = [path_spec.resolve(rel) for rel in (context.paths if context.paths else DEFAULT_PATHS)]
        files: list[Path] = []
        seen: set[Path] = set()
        for entry in entries:
            file_path = Path(entry["file"]).resolve()
            if not path_spec.is_under(file_path, context.root):
                continue
            if not any(path_spec.is_under(file_path, allowed) for allowed in allowed_abs):
                continue
            if file_path in seen:
                continue
            seen.add(file_path)
            files.append(file_path)
        return files

    def _run_tidy(self, files: list[Path], context: CheckContext, result: CheckResult) -> None:
        assert context.build_dir is not None
        build_dir = context.build_dir
        files = sorted(files, key=lambda path: str(path))
        log_dir = self._resolve_log_dir(context, build_dir)
        jobs = self._resolve_jobs(context.clang_tidy_jobs, len(files))
        errors: list[str] = []

        if jobs <= 1 or len(files) <= 1:
            for file_path in files:
                ok, message = self._run_single(file_path, context, log_dir)
                if not ok:
                    errors.append(message)
        else:
            with ThreadPoolExecutor(max_workers=jobs) as executor:
                futures = {executor.submit(self._run_single, file_path, context, log_dir): file_path for file_path in files}
                for future in as_completed(futures):
                    ok, message = future.result()
                    if not ok:
                        errors.append(message)

        for message in errors:
            result.add_error(message)
        if result.ok:
            result.success_message = f"clang-tidy check passed ({len(files)} files) logs: {log_dir}"

    def _resolve_jobs(self, jobs: int, file_count: int) -> int:
        if file_count <= 0:
            return 1
        if jobs <= 0:
            jobs = os.cpu_count() or 4
        return max(1, min(jobs, file_count))

    def _resolve_log_dir(self, context: CheckContext, build_dir: Path) -> Path:
        if context.clang_tidy_log_dir is not None:
            log_dir = context.clang_tidy_log_dir
        else:
            log_dir = build_dir / "clang-tidy-logs"
        log_dir.mkdir(parents=True, exist_ok=True)
        return log_dir

    def _run_single(self, file_path: Path, context: CheckContext, log_dir: Path) -> tuple[bool, str]:
        cmd = [
            context.clang_tidy_binary,
            f"-p={context.build_dir}",
            f"-checks={context.clang_tidy_checks}",
            "--warnings-as-errors=*",
            "--quiet",
            str(file_path),
        ]
        if context.clang_tidy_header_filter:
            cmd.append(f"-header-filter={context.clang_tidy_header_filter}")
        completed = self._runner.run(cmd)
        log_path = self._log_path_for(file_path, context, log_dir)
        output = f"{completed.stdout}{completed.stderr}"
        try:
            log_path.write_text(output, encoding="utf-8", errors="ignore")
        except OSError as exc:
            return False, f"[{file_path}] clang-tidy failed; log write error: {exc}"
        if completed.returncode != 0:
            return False, f"[{file_path}] clang-tidy failed (see {log_path})"
        return True, ""

    def _log_path_for(self, file_path: Path, context: CheckContext, log_dir: Path) -> Path:
        rel_path: Path | None = None
        try:
            rel_path = file_path.relative_to(context.root)
        except ValueError:
            rel_path = None
        if rel_path is not None:
            rel_str = str(rel_path)
        else:
            rel_str = str(file_path)
        safe_name = rel_str.replace("\\", "__").replace("/", "__").replace(":", "_")
        return log_dir / f"{safe_name}.log"

    def _filter_diff_files(self, files: list[Path], context: CheckContext, result: CheckResult) -> list[Path]:
        if not context.clang_tidy_diff_base:
            return files
        target = context.clang_tidy_diff_target or "HEAD"
        cmd = [
            "git",
            "diff",
            "--name-only",
            context.clang_tidy_diff_base,
            target,
        ]
        completed = self._runner.run(cmd, cwd=context.root)
        if completed.returncode != 0:
            output = f"{completed.stdout}{completed.stderr}".strip()
            result.add_error(f"git diff failed: {output}")
            return []
        rel_paths = [line.strip() for line in completed.stdout.splitlines() if line.strip()]
        if not rel_paths:
            return []
        if self._contains_header_like_diff(rel_paths):
            return files
        diff_paths = {Path(context.root / rel).resolve() for rel in rel_paths}
        return [path for path in files if path in diff_paths]

    def _contains_header_like_diff(self, rel_paths: list[str]) -> bool:
        for rel_path in rel_paths:
            suffix = Path(rel_path).suffix.lower()
            if suffix in HEADER_LIKE_SUFFIXES:
                return True
        return False

