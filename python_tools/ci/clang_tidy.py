#!/usr/bin/env python3
# @file python_tools/ci/clang_tidy.py
# @author Luis1454
# @project BLITZAR
# @brief Python quality and automation support for BLITZAR governance.

from __future__ import annotations

import json
import os
import platform
import shutil
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

from python_tools.ci.clang_tidy_file_executor import ClangTidyFileExecutor
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


# @brief Defines the clang tidy check type contract.
# @param None This contract does not take explicit parameters.
# @note Keep construction and side effects explicit for deterministic quality gates.
class ClangTidyCheck(BaseCheck):
    name = "clang_tidy"
    failure_title = "clang-tidy check failed:"

    # @brief Documents the init operation contract.
    # @param None This contract does not take explicit parameters.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def __init__(self) -> None:
        self._runner = ProcessRunner()

    # @brief Documents the execute operation contract.
    # @param context Input value used by this contract.
    # @param result Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
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

    # @brief Documents the load compile database operation contract.
    # @param context Input value used by this contract.
    # @param result Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
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

    # @brief Documents the load files operation contract.
    # @param context Input value used by this contract.
    # @param entries Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _load_files(self, context: CheckContext, entries: list[dict[str, object]]) -> list[Path]:
        path_spec = PathSpec(context.root)
        allowed_abs = [path_spec.resolve(rel) for rel in (context.paths if context.paths else DEFAULT_PATHS)]
        files: list[Path] = []
        seen: set[Path] = set()
        for entry in entries:
            file_path = Path(str(entry["file"])).resolve()
            if not path_spec.is_under(file_path, context.root):
                continue
            if not any(path_spec.is_under(file_path, allowed) for allowed in allowed_abs):
                continue
            if file_path in seen:
                continue
            seen.add(file_path)
            files.append(file_path)
        return files

    # @brief Documents the run tidy operation contract.
    # @param files Input value used by this contract.
    # @param context Input value used by this contract.
    # @param binary Input value used by this contract.
    # @param entries Input value used by this contract.
    # @param result Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
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
        file_executor = ClangTidyFileExecutor(self._runner, HEARTBEAT_SECONDS)
        errors: list[str] = []
        warnings: list[str] = []
        self._print_progress(
            f"[clang-tidy] analyzing {len(files)} file(s) with jobs={jobs} logs={log_dir}"
        )

        if jobs <= 1 or len(files) <= 1:
            for index, file_path in enumerate(files, start=1):
                ok, message, warning = file_executor.run(index, len(files), file_path, context, binary, extra_args, log_dir)
                if not ok:
                    errors.append(message)
                if warning:
                    warnings.append(warning)
        else:
            with ThreadPoolExecutor(max_workers=jobs) as executor:
                futures = {
                    executor.submit(
                        file_executor.run,
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

    # @brief Documents the resolve jobs operation contract.
    # @param jobs Input value used by this contract.
    # @param file_count Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _resolve_jobs(self, jobs: int, file_count: int) -> int:
        if file_count <= 0:
            return 1
        if jobs <= 0:
            cpu_count = os.cpu_count() or 4
            jobs = max(1, min(DEFAULT_AUTO_JOB_CAP, cpu_count // 2 if cpu_count > 1 else 1))
        return max(1, min(jobs, file_count))

    # @brief Documents the resolve log dir operation contract.
    # @param context Input value used by this contract.
    # @param build_dir Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _resolve_log_dir(self, context: CheckContext, build_dir: Path) -> Path:
        if context.clang_tidy_log_dir is not None:
            log_dir = context.clang_tidy_log_dir
        else:
            log_dir = build_dir / "clang-tidy-logs"
        log_dir.mkdir(parents=True, exist_ok=True)
        return log_dir

    # @brief Documents the resolve binary operation contract.
    # @param configured_binary Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
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

    # @brief Documents the resolve extra args operation contract.
    # @param entries Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _resolve_extra_args(self, entries: list[dict[str, object]]) -> list[str]:
        if self._uses_msvc_driver(entries):
            return ["--extra-arg-before=--driver-mode=cl"]
        return []

    # @brief Documents the uses msvc driver operation contract.
    # @param entries Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
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

    # @brief Documents the print progress operation contract.
    # @param message Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _print_progress(self, message: str) -> None:
        try:
            print(message, flush=True)
        except OSError:
            return

    # @brief Documents the filter diff files operation contract.
    # @param files Input value used by this contract.
    # @param context Input value used by this contract.
    # @param result Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
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

    # @brief Documents the contains header like diff operation contract.
    # @param rel_paths Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _contains_header_like_diff(self, rel_paths: list[str]) -> bool:
        for rel_path in rel_paths:
            suffix = Path(rel_path).suffix.lower()
            if suffix in HEADER_LIKE_SUFFIXES:
                return True
        return False

