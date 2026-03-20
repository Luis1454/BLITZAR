#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import platform
import shutil
import subprocess
import time
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
    "runtime/src/client",
    "runtime/src/protocol",
    "runtime/src/server",
)
HEADER_LIKE_SUFFIXES = (".h", ".hh", ".hpp", ".hxx", ".inl")
WINDOWS_LLVM_CLANG_TIDY_CANDIDATES = [
    Path(os.environ.get("PROGRAMFILES", r"C:\Program Files")) / "LLVM/bin/clang-tidy.exe",
    Path(os.environ.get("PROGRAMFILES(X86)", r"C:\Program Files (x86)")) / "LLVM/bin/clang-tidy.exe",
]
DEFAULT_AUTO_JOB_CAP = 6
HEARTBEAT_SECONDS = 30


class ClangTidyCheck(BaseCheck):
    name = "clang_tidy"
    failure_title = "clang-tidy check failed:"

    def __init__(self) -> None:
        self._runner = ProcessRunner()

    def _execute(self, context: CheckContext, result: CheckResult) -> None:
        binary = self._resolve_binary(context.clang_tidy_binary)
        if binary is None:
            result.add_error(f"clang-tidy executable not found: {context.clang_tidy_binary}")
            return
        entries = self._load_compile_database(context, result)
        if result.errors:
            return
        files = self._load_files(context, entries)
        if not files:
            result.add_error("clang-tidy check failed: no matching files found in compile database")
            return
        files = self._filter_diff_files(files, context, result)
        if result.errors:
            return
        if not files:
            result.success_message = "clang-tidy skipped (no matching changed files)"
            return
        self._run_tidy(files, context, binary, entries, result)
        if result.ok and not result.success_message:
            result.success_message = f"clang-tidy check passed ({len(files)} files)"

    def _load_compile_database(self, context: CheckContext, result: CheckResult) -> list[dict[str, object]]:
        build_dir = context.build_dir
        if build_dir is None:
            result.add_error("build directory is required")
            return []
        db = build_dir / "compile_commands.json"
        if not db.exists():
            result.add_error(f"compile database not found: {db}")
            return []
        try:
            loaded = json.loads(db.read_text(encoding="utf-8"))
        except json.JSONDecodeError as exc:
            result.add_error(str(exc))
            return []
        if not isinstance(loaded, list):
            result.add_error(f"invalid compile database payload: {db}")
            return []
        return loaded

    def _load_files(self, context: CheckContext, entries: list[dict[str, object]]) -> list[Path]:
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

    def _run_tidy(
        self,
        files: list[Path],
        context: CheckContext,
        binary: str,
        entries: list[dict[str, object]],
        result: CheckResult,
    ) -> None:
        assert context.build_dir is not None
        build_dir = context.build_dir
        files = sorted(files, key=lambda path: str(path))
        log_dir = self._resolve_log_dir(context, build_dir)
        jobs = self._resolve_jobs(context.clang_tidy_jobs, len(files))
        extra_args = self._resolve_extra_args(entries)
        errors: list[str] = []
        warnings: list[str] = []
        self._print_progress(
            f"[clang-tidy] analyzing {len(files)} file(s) with jobs={jobs} logs={log_dir}"
        )

        if jobs <= 1 or len(files) <= 1:
            for index, file_path in enumerate(files, start=1):
                ok, message, warning = self._run_single(index, len(files), file_path, context, binary, extra_args, log_dir)
                if not ok:
                    errors.append(message)
                if warning:
                    warnings.append(warning)
        else:
            with ThreadPoolExecutor(max_workers=jobs) as executor:
                futures = {
                    executor.submit(
                        self._run_single,
                        index,
                        len(files),
                        file_path,
                        context,
                        binary,
                        extra_args,
                        log_dir,
                    ): file_path
                    for index, file_path in enumerate(files, start=1)
                }
                for future in as_completed(futures):
                    ok, message, warning = future.result()
                    if not ok:
                        errors.append(message)
                    if warning:
                        warnings.append(warning)

        for message in errors:
            result.add_error(message)
        for message in warnings:
            result.add_warning(message)
        if result.ok:
            result.success_message = f"clang-tidy check passed ({len(files)} files) logs: {log_dir}"

    def _resolve_jobs(self, jobs: int, file_count: int) -> int:
        if file_count <= 0:
            return 1
        if jobs <= 0:
            cpu_count = os.cpu_count() or 4
            jobs = max(1, min(DEFAULT_AUTO_JOB_CAP, cpu_count // 2 if cpu_count > 1 else 1))
        return max(1, min(jobs, file_count))

    def _resolve_log_dir(self, context: CheckContext, build_dir: Path) -> Path:
        if context.clang_tidy_log_dir is not None:
            log_dir = context.clang_tidy_log_dir
        else:
            log_dir = build_dir / "clang-tidy-logs"
        log_dir.mkdir(parents=True, exist_ok=True)
        return log_dir

    def _run_single(
        self,
        index: int,
        total: int,
        file_path: Path,
        context: CheckContext,
        binary: str,
        extra_args: list[str],
        log_dir: Path,
    ) -> tuple[bool, str, str]:
        display_path = self._display_path(file_path, context)
        timeout_seconds: int | None = context.clang_tidy_file_timeout_sec if context.clang_tidy_file_timeout_sec > 0 else None
        cmd = self._make_command(binary, context, extra_args, file_path, context.clang_tidy_checks)
        start = time.monotonic()
        self._print_progress(f"[clang-tidy] [{index}/{total}] start {display_path}")
        try:
            completed = self._runner.run_with_heartbeat(
                cmd,
                timeout=timeout_seconds,
                cwd=context.root,
                heartbeat_seconds=HEARTBEAT_SECONDS,
                on_heartbeat=lambda: self._print_progress(
                    f"[clang-tidy] [{index}/{total}] still running {display_path} ({int(time.monotonic() - start)}s)"
                ),
            )
        except subprocess.TimeoutExpired as exc:
            elapsed = time.monotonic() - start
            log_path = self._log_path_for(file_path, context, log_dir)
            timeout_output = f"{exc.output or ''}{exc.stderr or ''}"
            timeout_message = (
                f"[clang-tidy] [{index}/{total}] timeout {display_path} ({elapsed:.1f}s) "
                f"limit={timeout_seconds}s"
            )
            self._print_progress(timeout_message)
            fallback_checks = self._resolve_timeout_fallback_checks(
                context.clang_tidy_checks,
                context.clang_tidy_timeout_fallback_checks,
            )
            if not fallback_checks:
                log_error = self._write_log(
                    log_path,
                    f"{timeout_message}\n\n{timeout_output}",
                )
                if log_error:
                    return False, f"[{file_path}] clang-tidy timed out and log write failed: {log_error}", ""
                return False, f"[{file_path}] clang-tidy timed out after {timeout_seconds}s (see {log_path})", ""
            fallback_cmd = self._make_command(binary, context, extra_args, file_path, fallback_checks)
            fallback_start = time.monotonic()
            self._print_progress(
                f"[clang-tidy] [{index}/{total}] retry {display_path} with fallback checks={fallback_checks}"
            )
            fallback_completed = self._runner.run_with_heartbeat(
                fallback_cmd,
                timeout=timeout_seconds,
                cwd=context.root,
                heartbeat_seconds=HEARTBEAT_SECONDS,
                on_heartbeat=lambda: self._print_progress(
                    f"[clang-tidy] [{index}/{total}] still running fallback {display_path} ({int(time.monotonic() - fallback_start)}s)"
                ),
            )
            fallback_output = f"{fallback_completed.stdout}{fallback_completed.stderr}"
            log_error = self._write_log(
                log_path,
                (
                    f"{timeout_message}\n"
                    f"fallback_checks={fallback_checks}\n\n"
                    f"{fallback_output}"
                ),
            )
            if log_error:
                return False, f"[{file_path}] clang-tidy fallback log write failed: {log_error}", ""
            fallback_elapsed = time.monotonic() - fallback_start
            if fallback_completed.returncode != 0:
                self._print_progress(
                    f"[clang-tidy] [{index}/{total}] fail fallback {display_path} ({fallback_elapsed:.1f}s) log={log_path}"
                )
                return False, f"[{file_path}] clang-tidy fallback failed after timeout (see {log_path})", ""
            self._print_progress(
                f"[clang-tidy] [{index}/{total}] done fallback {display_path} ({fallback_elapsed:.1f}s)"
            )
            warning = (
                f"{display_path}: analyzer timed out after {timeout_seconds}s; "
                f"fallback checks applied ({fallback_checks})"
            )
            return True, "", warning
        log_path = self._log_path_for(file_path, context, log_dir)
        output = f"{completed.stdout}{completed.stderr}"
        log_error = self._write_log(log_path, output)
        if log_error:
            return False, f"[{file_path}] clang-tidy failed; {log_error}", ""
        elapsed = time.monotonic() - start
        if completed.returncode != 0:
            self._print_progress(f"[clang-tidy] [{index}/{total}] fail {display_path} ({elapsed:.1f}s) log={log_path}")
            return False, f"[{file_path}] clang-tidy failed (see {log_path})", ""
        self._print_progress(f"[clang-tidy] [{index}/{total}] done {display_path} ({elapsed:.1f}s)")
        return True, "", ""

    def _make_command(
        self,
        binary: str,
        context: CheckContext,
        extra_args: list[str],
        file_path: Path,
        checks: str,
    ) -> list[str]:
        cmd = [
            binary,
            f"-p={context.build_dir}",
            f"-checks={checks}",
            "--warnings-as-errors=*",
            "--quiet",
            *extra_args,
            str(file_path),
        ]
        if context.clang_tidy_header_filter:
            cmd.append(f"-header-filter={context.clang_tidy_header_filter}")
        return cmd

    def _resolve_timeout_fallback_checks(self, checks: str, fallback_checks: str) -> str:
        if "clang-analyzer-" not in checks:
            return ""
        trimmed = fallback_checks.strip()
        if not trimmed:
            return ""
        if trimmed == checks:
            return ""
        return trimmed

    def _write_log(self, log_path: Path, content: str) -> str:
        try:
            log_path.write_text(content, encoding="utf-8", errors="ignore")
            return ""
        except OSError as exc:
            return f"log write error: {exc}"

    def _resolve_binary(self, configured_binary: str) -> str | None:
        resolved = shutil.which(configured_binary)
        if resolved is not None:
            return resolved
        if platform.system() != "Windows":
            return None
        normalized = configured_binary.lower()
        if normalized not in {"clang-tidy", "clang-tidy.exe"}:
            return None
        for candidate in WINDOWS_LLVM_CLANG_TIDY_CANDIDATES:
            if candidate.exists():
                return str(candidate)
        return None

    def _resolve_extra_args(self, entries: list[dict[str, object]]) -> list[str]:
        if self._uses_msvc_driver(entries):
            return ["--extra-arg-before=--driver-mode=cl"]
        return []

    def _uses_msvc_driver(self, entries: list[dict[str, object]]) -> bool:
        for entry in entries:
            command = str(entry.get("command", "")).lower()
            if "cl.exe" in command:
                return True
            arguments = entry.get("arguments")
            if not isinstance(arguments, list) or not arguments:
                continue
            compiler = str(arguments[0]).lower()
            if compiler.endswith("cl.exe"):
                return True
        return False

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

    def _display_path(self, file_path: Path, context: CheckContext) -> str:
        try:
            return str(file_path.relative_to(context.root))
        except ValueError:
            return str(file_path)

    def _print_progress(self, message: str) -> None:
        try:
            print(message, flush=True)
        except OSError:
            return

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

