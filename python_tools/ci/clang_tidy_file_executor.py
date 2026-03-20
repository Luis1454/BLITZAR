#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import time
from pathlib import Path

from python_tools.core.io import ProcessRunner
from python_tools.core.models import CheckContext


class ClangTidyFileExecutor:
    def __init__(self, runner: ProcessRunner, heartbeat_seconds: int) -> None:
        self._runner = runner
        self._heartbeat_seconds = heartbeat_seconds

    def run(
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
                heartbeat_seconds=self._heartbeat_seconds,
                on_heartbeat=lambda: self._print_progress(
                    f"[clang-tidy] [{index}/{total}] still running {display_path} ({int(time.monotonic() - start)}s)"
                ),
            )
        except subprocess.TimeoutExpired as exc:
            return self._handle_timeout(
                index,
                total,
                file_path,
                display_path,
                context,
                binary,
                extra_args,
                log_dir,
                timeout_seconds,
                start,
                exc,
            )

        return self._handle_completion(index, total, file_path, display_path, context, log_dir, start, completed)

    def _handle_timeout(
        self,
        index: int,
        total: int,
        file_path: Path,
        display_path: str,
        context: CheckContext,
        binary: str,
        extra_args: list[str],
        log_dir: Path,
        timeout_seconds: int | None,
        start: float,
        exc: subprocess.TimeoutExpired,
    ) -> tuple[bool, str, str]:
        elapsed = time.monotonic() - start
        log_path = self._log_path_for(file_path, context, log_dir)
        timeout_output = self._stringify_output(exc.output) + self._stringify_output(exc.stderr)
        timeout_message = f"[clang-tidy] [{index}/{total}] timeout {display_path} ({elapsed:.1f}s) limit={timeout_seconds}s"
        self._print_progress(timeout_message)
        fallback_checks = self._resolve_timeout_fallback_checks(
            context.clang_tidy_checks,
            context.clang_tidy_timeout_fallback_checks,
        )
        if not fallback_checks:
            log_error = self._write_log(log_path, f"{timeout_message}\n\n{timeout_output}")
            if log_error:
                return False, f"[{file_path}] clang-tidy timed out and log write failed: {log_error}", ""
            return False, f"[{file_path}] clang-tidy timed out after {timeout_seconds}s (see {log_path})", ""

        fallback_cmd = self._make_command(binary, context, extra_args, file_path, fallback_checks)
        fallback_start = time.monotonic()
        self._print_progress(f"[clang-tidy] [{index}/{total}] retry {display_path} with fallback checks={fallback_checks}")
        fallback_completed = self._runner.run_with_heartbeat(
            fallback_cmd,
            timeout=timeout_seconds,
            cwd=context.root,
            heartbeat_seconds=self._heartbeat_seconds,
            on_heartbeat=lambda: self._print_progress(
                f"[clang-tidy] [{index}/{total}] still running fallback {display_path} ({int(time.monotonic() - fallback_start)}s)"
            ),
        )
        fallback_output = f"{fallback_completed.stdout}{fallback_completed.stderr}"
        log_error = self._write_log(
            log_path,
            f"{timeout_message}\nfallback_checks={fallback_checks}\n\n{fallback_output}",
        )
        if log_error:
            return False, f"[{file_path}] clang-tidy fallback log write failed: {log_error}", ""
        fallback_elapsed = time.monotonic() - fallback_start
        if fallback_completed.returncode != 0:
            self._print_progress(
                f"[clang-tidy] [{index}/{total}] fail fallback {display_path} ({fallback_elapsed:.1f}s) log={log_path}"
            )
            return False, f"[{file_path}] clang-tidy fallback failed after timeout (see {log_path})", ""
        self._print_progress(f"[clang-tidy] [{index}/{total}] done fallback {display_path} ({fallback_elapsed:.1f}s)")
        warning = f"{display_path}: analyzer timed out after {timeout_seconds}s; fallback checks applied ({fallback_checks})"
        return True, "", warning

    def _handle_completion(
        self,
        index: int,
        total: int,
        file_path: Path,
        display_path: str,
        context: CheckContext,
        log_dir: Path,
        start: float,
        completed: subprocess.CompletedProcess[str],
    ) -> tuple[bool, str, str]:
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

    @staticmethod
    def _resolve_timeout_fallback_checks(checks: str, fallback_checks: str) -> str:
        if "clang-analyzer-" not in checks:
            return ""
        trimmed = fallback_checks.strip()
        if not trimmed or trimmed == checks:
            return ""
        return trimmed

    @staticmethod
    def _stringify_output(value: str | bytes | None) -> str:
        if isinstance(value, bytes):
            return value.decode("utf-8", errors="ignore")
        return value or ""

    @staticmethod
    def _write_log(log_path: Path, content: str) -> str:
        try:
            log_path.write_text(content, encoding="utf-8", errors="ignore")
            return ""
        except OSError as exc:
            return f"log write error: {exc}"

    @staticmethod
    def _log_path_for(file_path: Path, context: CheckContext, log_dir: Path) -> Path:
        try:
            rel_str = str(file_path.relative_to(context.root))
        except ValueError:
            rel_str = str(file_path)
        safe_name = rel_str.replace("\\", "__").replace("/", "__").replace(":", "_")
        return log_dir / f"{safe_name}.log"

    @staticmethod
    def _display_path(file_path: Path, context: CheckContext) -> str:
        try:
            return str(file_path.relative_to(context.root))
        except ValueError:
            return str(file_path)

    @staticmethod
    def _print_progress(message: str) -> None:
        try:
            print(message, flush=True)
        except OSError:
            return
