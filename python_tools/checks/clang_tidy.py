#!/usr/bin/env python3
from __future__ import annotations

import json
import shutil
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
        self._run_tidy(files, context, result)
        if result.ok:
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
        for file_path in files:
            cmd = [
                context.clang_tidy_binary,
                f"-p={context.build_dir}",
                f"-checks={context.clang_tidy_checks}",
                "--warnings-as-errors=*",
                "--quiet",
                str(file_path),
            ]
            completed = self._runner.run(cmd)
            if completed.returncode != 0:
                output = f"{completed.stdout}{completed.stderr}".strip()
                result.add_error(f"[{file_path}] {output}".strip())


