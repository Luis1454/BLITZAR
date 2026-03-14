#!/usr/bin/env python3
from __future__ import annotations

import csv
import json
import re
import subprocess
from dataclasses import dataclass
from pathlib import Path
from re import Pattern

from .models import JsonValue

REQ_ID_RE = re.compile(r"^REQ-[A-Z]+-[0-9]{3}$")
TEST_MACRO_RE = re.compile(r"TEST(?:_F)?\(\s*([A-Za-z0-9_]+)\s*,\s*([A-Za-z0-9_]+)\s*\)")
RUST_TEST_RE = re.compile(r"#\s*\[\s*test\s*\][\s\r\n]*fn\s+([A-Za-z0-9_]+)\s*\(")


@dataclass(frozen=True)
class RegexSpec:
    pattern: str
    flags: int = 0

    def compile(self) -> Pattern[str]:
        return re.compile(self.pattern, self.flags)

    def search(self, text: str) -> bool:
        return self.compile().search(text) is not None


@dataclass(frozen=True)
class PathSpec:
    root: Path

    @staticmethod
    def is_under(path: Path, parent: Path) -> bool:
        try:
            path.relative_to(parent)
            return True
        except ValueError:
            return False

    def resolve(self, relative: str) -> Path:
        return (self.root / relative).resolve()


class JsonLoader:
    def load(self, path: Path) -> tuple[JsonValue | None, str | None]:
        try:
            return json.loads(path.read_text(encoding="utf-8")), None
        except Exception as exc:
            return None, str(exc)


class CsvLoader:
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


class GitTrackedService:
    def is_tracked(self, root: Path, rel_path: str) -> bool:
        result = subprocess.run(
            ["git", "ls-files", "--error-unmatch", rel_path],
            cwd=root,
            capture_output=True,
            text=True,
            check=False,
        )
        return result.returncode == 0


class ProcessRunner:
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


def _normalize_rust_test_id(root: Path, path: Path, test_name: str) -> str:
    rel_parts = list(path.relative_to(root).with_suffix("").parts)
    if rel_parts and rel_parts[0] == "rust":
        rel_parts = rel_parts[1:]
    normalized_parts = [part.replace("-", "_") for part in rel_parts]
    return "::".join((*normalized_parts, test_name))


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
