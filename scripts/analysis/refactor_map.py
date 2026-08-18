#!/usr/bin/env python3
"""Generate a responsibility and ownership map for the production tree."""

from __future__ import annotations

import argparse
import json
import re
import sys
from collections import Counter, defaultdict
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from python_tools.policies.repo_policy_function_metrics import _collect_function_metrics

SOURCE_EXTENSIONS = {".cpp", ".hpp", ".cu", ".cuh", ".inl"}
ROOTS = ("apps", "engine", "modules", "runtime")
SKIP_PARTS = {"build", "dist", "exports", "issue527-worktree"}
POINTER_RE = re.compile(
    r"\b(?:const\s+)?[A-Za-z_][A-Za-z0-9_:<>]*(?:\s+const)?\s*\*\s*"
    r"(?P<name>[A-Za-z_][A-Za-z0-9_]*)"
)
MULTI_DECL_RE = re.compile(
    r"^\s*(?:const\s+)?(?:auto|[A-Za-z_][A-Za-z0-9_:<>]*)\s+"
    r"[A-Za-z_][A-Za-z0-9_]*\s*=.*[,].*;\s*$"
)


def iter_sources(root: Path) -> list[Path]:
    paths: list[Path] = []
    for root_name in ROOTS:
        base = root / root_name
        if not base.is_dir():
            continue
        for path in base.rglob("*"):
            if path.is_file() and path.suffix in SOURCE_EXTENSIONS:
                if not any(part in SKIP_PARTS for part in path.parts):
                    paths.append(path)
    return sorted(paths)


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", lambda match: "\n" * match.group(0).count("\n"), text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def module_name(path: Path, root: Path) -> str:
    parts = path.relative_to(root).parts
    if len(parts) >= 2:
        return "/".join(parts[:2])
    return parts[0]


def pointer_kind(relative: str, line: str) -> str:
    if relative.startswith("modules/qt/"):
        return "Qt boundary"
    if "/module/" in relative or relative.startswith("apps/"):
        return "ABI or process boundary"
    if "new " in line or "delete " in line:
        return "ownership candidate"
    if "const char*" in line or "string_view" in line:
        return "borrowed text"
    return "borrowed or ownership candidate"


def pointer_entries(path: Path, root: Path, cleaned: str) -> list[dict[str, object]]:
    relative = path.relative_to(root).as_posix()
    entries: list[dict[str, object]] = []
    for line_number, line in enumerate(cleaned.splitlines(), 1):
        if "*" not in line:
            continue
        for match in POINTER_RE.finditer(line):
            entries.append({
                "file": relative,
                "line": line_number,
                "name": match.group("name"),
                "kind": pointer_kind(relative, line),
            })
    return entries


def file_summary(path: Path, root: Path, content: str) -> dict[str, object]:
    cleaned = strip_comments(content)
    functions = _collect_function_metrics(content) if path.suffix in {".cpp", ".cu", ".inl"} else []
    declaration_lines = [
        index for index, line in enumerate(cleaned.splitlines(), 1) if MULTI_DECL_RE.match(line)
    ]
    return {
        "file": path.relative_to(root).as_posix(),
        "module": module_name(path, root),
        "lines": len(content.splitlines()),
        "functions": len(functions),
        "max_function_lines": max((item["lines"] for item in functions), default=0),
        "max_complexity": max((item["complexity"] for item in functions), default=0),
        "multiple_declaration_lines": declaration_lines,
        "pointers": pointer_entries(path, root, cleaned),
    }


def potential_dead_files(paths: list[Path], corpus: str, root: Path) -> list[str]:
    candidates: list[str] = []
    for path in paths:
        relative = path.relative_to(root).as_posix()
        if path.suffix not in {".cpp", ".cu"} or "/tests/" in f"/{relative}":
            continue
        if corpus.count(path.name) <= 1:
            candidates.append(relative)
    return candidates


def build_report(root: Path) -> dict[str, object]:
    paths = iter_sources(root)
    summaries = [file_summary(path, root, path.read_text(encoding="utf-8", errors="ignore")) for path in paths]
    grouped: dict[str, dict[str, int]] = defaultdict(lambda: {"files": 0, "functions": 0, "pointers": 0})
    for item in summaries:
        group = grouped[str(item["module"])]
        group["files"] += 1
        group["functions"] += int(item["functions"])
        group["pointers"] += len(item["pointers"])
    reference_files = [
        path for path in root.rglob("*")
        if path.is_file()
        and (path.suffix in SOURCE_EXTENSIONS or path.name in {"CMakeLists.txt", "Module.cmake"})
        and not any(part in SKIP_PARTS for part in path.parts)
    ]
    corpus = "\n".join(path.read_text(encoding="utf-8", errors="ignore") for path in reference_files)
    pointers = [entry for item in summaries for entry in item["pointers"]]
    return {
        "source_files": len(paths),
        "functions": sum(int(item["functions"]) for item in summaries),
        "modules": dict(sorted(grouped.items())),
        "files": summaries,
        "pointer_counts": dict(Counter(str(entry["kind"]) for entry in pointers)),
        "potential_dead_files": potential_dead_files(paths, corpus, root),
    }


def render_markdown(report: dict[str, object]) -> str:
    lines = [
        "# Refactor Map",
        "",
        "This report is diagnostic. Potential dead files require reference, registration, export, and test review before deletion.",
        "",
        f"- Production source files: {report['source_files']}",
        f"- Implementation functions: {report['functions']}",
        "",
        "## Modules",
        "",
        "| Module | Files | Functions | Pointer candidates |",
        "|---|---:|---:|---:|",
    ]
    for name, values in report["modules"].items():
        lines.append(f"| `{name}` | {values['files']} | {values['functions']} | {values['pointers']} |")
    lines += ["", "## Pointer Classification", ""]
    for name, count in sorted(report["pointer_counts"].items()):
        lines.append(f"- `{name}`: {count}")
    lines += ["", "## Potential Dead Files", "", "No file in this section is deleted automatically.", ""]
    candidates = report["potential_dead_files"] or ["None detected by filename reference heuristic."]
    lines.extend(f"- `{candidate}`" for candidate in candidates)
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path("."))
    parser.add_argument("--output", type=Path)
    parser.add_argument("--json", action="store_true")
    arguments = parser.parse_args()
    report = build_report(arguments.root.resolve())
    payload = json.dumps(report, indent=2) if arguments.json else render_markdown(report)
    if arguments.output:
        arguments.output.write_text(payload + "\n", encoding="utf-8")
    else:
        print(payload)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
