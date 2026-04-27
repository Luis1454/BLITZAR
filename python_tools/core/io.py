#!/usr/bin/env python3
# File: python_tools/core/io.py
# Purpose: Python quality and automation support for BLITZAR governance.

from __future__ import annotations

import csv
import json
import re
import subprocess
from collections.abc import Callable
from dataclasses import dataclass
from pathlib import Path
from re import Pattern

from .models import JsonValue

REQ_ID_RE = re.compile(r"^REQ-[A-Z]+-[0-9]{3}$")
TEST_MACRO_RE = re.compile(r"TEST(?:_F)?\(\s*([A-Za-z0-9_]+)\s*,\s*([A-Za-z0-9_]+)\s*\)")
RUST_TEST_RE = re.compile(r"#\s*\[\s*test\s*\][\s\r\n]*fn\s+([A-Za-z0-9_]+)\s*\(")


@dataclass(frozen=True)
# Description: Defines the RegexSpec contract.
class RegexSpec:
    pattern: str
    flags: int = 0

    # Description: Executes the compile operation.
    def compile(self) -> Pattern[str]:
        return re.compile(self.pattern, self.flags)

    # Description: Executes the search operation.
    def search(self, text: str) -> bool:
        return self.compile().search(text) is not None


@dataclass(frozen=True)
# Description: Defines the PathSpec contract.
class PathSpec:
    root: Path

    @staticmethod
    # Description: Executes the is_under operation.
    def is_under(path: Path, parent: Path) -> bool:
        try:
            path.relative_to(parent)
            return True
        except ValueError:
            return False

    # Description: Executes the resolve operation.
    def resolve(self, relative: str) -> Path:
        return (self.root / relative).resolve()


# Description: Defines the JsonLoader contract.
class JsonLoader:
    # Description: Executes the load operation.
    def load(self, path: Path) -> tuple[JsonValue | None, str | None]:
        try:
            return json.loads(path.read_text(encoding="utf-8")), None
        except Exception as exc:
            return None, str(exc)


# Description: Defines the CsvLoader contract.
class CsvLoader:
    # Description: Executes the load_rows operation.
    def load_rows(
        self,
        path: Path,
        expected_columns: tuple[str, ...],
        parse_error_label: str,
        column_error_label: str,
    ) -> tuple[list[dict[str, str]], list[str]]:
        errors: list[str] = []
        try:
            with path.open("r", encoding="utf-8", newline="") as handle:
                reader = csv.DictReader(handle)
                fieldnames = tuple(reader.fieldnames or [])
                rows = list(reader)
        except Exception as exc:
            errors.append(f"failed to parse {parse_error_label}: {exc}")
            return [], errors

        if fieldnames != expected_columns:
            errors.append(column_error_label + ", ".join(expected_columns))
        return rows, errors


# Description: Defines the GitTrackedService contract.
class GitTrackedService:
    # Description: Executes the is_tracked operation.
    def is_tracked(self, root: Path, rel_path: str) -> bool:
        result = subprocess.run(
            ["git", "ls-files", "--error-unmatch", rel_path],
            cwd=root,
            capture_output=True,
            text=True,
            check=False,
        )
        return result.returncode == 0


# Description: Defines the ProcessRunner contract.
class ProcessRunner:
    # Description: Executes the run operation.
    def run(
        self,
        args: list[str],
        timeout: int | None = None,
        cwd: Path | None = None,
    ) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            args,
            capture_output=True,
            text=True,
            timeout=timeout,
            cwd=cwd,
            check=False,
        )

    # Description: Executes the run_with_heartbeat operation.
    def run_with_heartbeat(
        self,
        args: list[str],
        timeout: int | None = None,
        cwd: Path | None = None,
        heartbeat_seconds: int = 30,
        on_heartbeat: Callable[[], None] | None = None,
    ) -> subprocess.CompletedProcess[str]:
        process = subprocess.Popen(
            args,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            cwd=cwd,
        )
        try:
            while True:
                try:
                    stdout, stderr = process.communicate(timeout=heartbeat_seconds)
                    return subprocess.CompletedProcess(args, process.returncode, stdout, stderr)
                except subprocess.TimeoutExpired as exc:
                    if on_heartbeat is not None:
                        on_heartbeat()
                    if timeout is not None:
                        timeout -= heartbeat_seconds
                        if timeout <= 0:
                            process.kill()
                            stdout, stderr = process.communicate()
                            raise subprocess.TimeoutExpired(
                                args,
                                heartbeat_seconds,
                                output=stdout,
                                stderr=stderr,
                            ) from exc
        finally:
            if process.poll() is None:
                process.kill()


# Description: Executes the collect_test_ids operation.
def collect_test_ids(root: Path, extra_test_ids: set[str], test_macro_re: Pattern[str] = TEST_MACRO_RE) -> set[str]:
    tests: set[str] = set(extra_test_ids)
    for path in (root / "tests").rglob("*.cpp"):
        text = path.read_text(encoding="utf-8", errors="ignore")
        for suite, case in test_macro_re.findall(text):
            tests.add(f"{suite}.{case}")
    rust_root = root / "rust"
    if rust_root.exists():
        for path in rust_root.rglob("*.rs"):
            text = path.read_text(encoding="utf-8", errors="ignore")
            for test_name in RUST_TEST_RE.findall(text):
                tests.add(_normalize_rust_test_id(root, path, test_name))
    return tests


# Description: Executes the _normalize_rust_test_id operation.
def _normalize_rust_test_id(root: Path, path: Path, test_name: str) -> str:
    rel_parts = list(path.relative_to(root).with_suffix("").parts)
    if rel_parts and rel_parts[0] == "rust":
        rel_parts = rel_parts[1:]
    normalized_parts = [part.replace("-", "_") for part in rel_parts]
    return "::".join((*normalized_parts, test_name))


# Description: Executes the collect_filtered_test_ids operation.
def collect_filtered_test_ids(
    root: Path,
    id_re: Pattern[str],
    relative_files: tuple[str, ...],
) -> set[str]:
    filtered_ids: set[str] = set()
    for rel_path in relative_files:
        path = root / rel_path
        if not path.exists():
            continue
        text = path.read_text(encoding="utf-8", errors="ignore")
        for match in id_re.finditer(text):
            filtered_ids.add(match.group(0))
    return filtered_ids
