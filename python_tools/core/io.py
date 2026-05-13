#!/usr/bin/env python3
# @file python_tools/core/io.py
# @author Luis1454
# @project BLITZAR
# @brief Python quality and automation support for BLITZAR governance.

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
# @brief Defines the regex spec type contract.
# @param None This contract does not take explicit parameters.
# @note Keep construction and side effects explicit for deterministic quality gates.
class RegexSpec:
    pattern: str
    flags: int = 0

    # @brief Documents the compile operation contract.
    # @param None This contract does not take explicit parameters.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def compile(self) -> Pattern[str]:
        return re.compile(self.pattern, self.flags)

    # @brief Documents the search operation contract.
    # @param text Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def search(self, text: str) -> bool:
        return self.compile().search(text) is not None


@dataclass(frozen=True)
# @brief Defines the path spec type contract.
# @param None This contract does not take explicit parameters.
# @note Keep construction and side effects explicit for deterministic quality gates.
class PathSpec:
    root: Path

    @staticmethod
    # @brief Documents the is under operation contract.
    # @param path Input value used by this contract.
    # @param parent Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def is_under(path: Path, parent: Path) -> bool:
        try:
            path.relative_to(parent)
            return True
        except ValueError:
            return False

    # @brief Documents the resolve operation contract.
    # @param relative Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def resolve(self, relative: str) -> Path:
        return (self.root / relative).resolve()


# @brief Defines the json loader type contract.
# @param None This contract does not take explicit parameters.
# @note Keep construction and side effects explicit for deterministic quality gates.
class JsonLoader:
    # @brief Documents the load operation contract.
    # @param path Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def load(self, path: Path) -> tuple[JsonValue | None, str | None]:
        try:
            return json.loads(path.read_text(encoding="utf-8")), None
        except Exception as exc:
            return None, str(exc)


# @brief Defines the csv loader type contract.
# @param None This contract does not take explicit parameters.
# @note Keep construction and side effects explicit for deterministic quality gates.
class CsvLoader:
    # @brief Documents the load rows operation contract.
    # @param path Input value used by this contract.
    # @param expected_columns Input value used by this contract.
    # @param parse_error_label Input value used by this contract.
    # @param column_error_label Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
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


# @brief Defines the git tracked service type contract.
# @param None This contract does not take explicit parameters.
# @note Keep construction and side effects explicit for deterministic quality gates.
class GitTrackedService:
    # @brief Documents the is tracked operation contract.
    # @param root Input value used by this contract.
    # @param rel_path Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def is_tracked(self, root: Path, rel_path: str) -> bool:
        result = subprocess.run(
            ["git", "ls-files", "--error-unmatch", rel_path],
            cwd=root,
            capture_output=True,
            text=True,
            check=False,
        )
        return result.returncode == 0


# @brief Defines the process runner type contract.
# @param None This contract does not take explicit parameters.
# @note Keep construction and side effects explicit for deterministic quality gates.
class ProcessRunner:
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
        return subprocess.run(
            args,
            capture_output=True,
            text=True,
            timeout=timeout,
            cwd=cwd,
            check=False,
        )

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


# @brief Documents the collect test ids operation contract.
# @param root Input value used by this contract.
# @param extra_test_ids Input value used by this contract.
# @param test_macro_re Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
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


# @brief Documents the normalize rust test id operation contract.
# @param root Input value used by this contract.
# @param path Input value used by this contract.
# @param test_name Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def _normalize_rust_test_id(root: Path, path: Path, test_name: str) -> str:
    rel_parts = list(path.relative_to(root).with_suffix("").parts)
    if rel_parts and rel_parts[0] == "rust":
        rel_parts = rel_parts[1:]
    normalized_parts = [part.replace("-", "_") for part in rel_parts]
    return "::".join((*normalized_parts, test_name))


# @brief Documents the collect filtered test ids operation contract.
# @param root Input value used by this contract.
# @param id_re Input value used by this contract.
# @param relative_files Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
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
