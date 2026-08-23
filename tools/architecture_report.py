"""Report bounded architecture signals without using file line count as a split rule."""

from __future__ import annotations

import argparse
import dataclasses
import json
import pathlib
import re
import sys

from argument_gate import (
    NAME_PATTERN,
    is_callable_candidate,
    load_exceptions,
    matching_parenthesis,
    parameter_count,
    strip_comments_and_literals,
)


SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".cu", ".cuh", ".h", ".hip", ".hpp", ".inl"}
SOURCE_ROOTS = ("include", "src", "apps")
BRANCH_PATTERN = re.compile(r"\b(?:if|else|for|while|switch|case|catch)\b|&&|\|\||\?")
ALLOCATION_PATTERN = re.compile(
    r"\b(?:new|delete|malloc|calloc|realloc|free|make_unique|make_shared|"
    r"reserve|resize|emplace_back|push_back)\s*(?:<|\(|;|$)"
)
INCLUDE_PATTERN = re.compile(r"^\s*#\s*include\s*[<\"]([^>\"]+)[>\"]", re.MULTILINE)
DEFAULT_THRESHOLDS = {
    "max_parameters": 4,
    "max_function_lines": 80,
    "max_functions_per_file": 12,
    "max_branch_points": 12,
    "max_allocation_sites": 8,
    "max_internal_includes": 12,
}


@dataclasses.dataclass(frozen=True)
class FunctionMetric:
    name: str
    line: int
    parameters: int
    body_lines: int
    branch_points: int
    allocation_sites: int

    def as_dict(self) -> dict[str, object]:
        return dataclasses.asdict(self)


@dataclasses.dataclass(frozen=True)
class FileMetric:
    path: str
    line_count: int
    include_count: int
    internal_include_count: int
    allocation_sites: int
    functions: tuple[FunctionMetric, ...]
    signals: tuple[str, ...]

    def as_dict(self) -> dict[str, object]:
        max_parameters = max((item.parameters for item in self.functions), default=0)
        max_function_lines = max((item.body_lines for item in self.functions), default=0)
        max_branch_points = max((item.branch_points for item in self.functions), default=0)
        return {
            "path": self.path,
            "line_count": self.line_count,
            "function_count": len(self.functions),
            "max_parameters": max_parameters,
            "max_function_lines": max_function_lines,
            "max_branch_points": max_branch_points,
            "include_count": self.include_count,
            "internal_include_count": self.internal_include_count,
            "allocation_sites": self.allocation_sites,
            "review_required": bool(self.signals),
            "signals": list(self.signals),
            "functions": [item.as_dict() for item in self.functions],
        }


def matching_brace(source: str, opening: int) -> int | None:
    depth = 0
    for index in range(opening, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return index
    return None


def line_number(source: str, offset: int) -> int:
    return source.count("\n", 0, offset) + 1


def body_opening(source: str, closing: int) -> int | None:
    suffix = source[closing + 1 : closing + 512]
    brace = suffix.find("{")
    if brace < 0:
        return None
    prefix = suffix[:brace]
    if ";" in prefix or "=" in prefix:
        return None
    return closing + 1 + brace


def scan_functions(source: str, path: str) -> tuple[FunctionMetric, ...]:
    clean = strip_comments_and_literals(source)
    functions: list[FunctionMetric] = []
    opening = clean.find("(")

    while opening >= 0:
        closing = matching_parenthesis(clean, opening)
        if closing is None:
            break
        prefix_match = NAME_PATTERN.search(clean[:opening])
        if prefix_match is not None:
            name = prefix_match.group("name")
            if is_callable_candidate(clean, opening, name):
                opening_body = body_opening(clean, closing)
                if opening_body is not None:
                    closing_body = matching_brace(clean, opening_body)
                    if closing_body is not None:
                        body = clean[opening_body + 1 : closing_body]
                        functions.append(
                            FunctionMetric(
                                name=name,
                                line=line_number(clean, opening),
                                parameters=parameter_count(clean[opening + 1 : closing]),
                                body_lines=clean.count("\n", opening_body, closing_body) + 1,
                                branch_points=len(BRANCH_PATTERN.findall(body)),
                                allocation_sites=len(ALLOCATION_PATTERN.findall(body)),
                            )
                        )
        opening = clean.find("(", closing + 1)

    return tuple(functions)


def discover_files(root: pathlib.Path) -> list[pathlib.Path]:
    return sorted(
        path
        for source_root in SOURCE_ROOTS
        for path in (root / source_root).rglob("*")
        if path.is_file() and path.suffix.lower() in SOURCE_SUFFIXES
    )


def load_thresholds(root: pathlib.Path) -> dict[str, int]:
    quality_path = root / "plan" / "quality.json"
    if not quality_path.is_file():
        return dict(DEFAULT_THRESHOLDS)
    quality = json.loads(quality_path.read_text(encoding="utf-8"))
    architecture = quality.get("architecture", {})
    configured = architecture.get("thresholds", {}) if isinstance(architecture, dict) else {}
    thresholds = dict(DEFAULT_THRESHOLDS)
    for name, default in DEFAULT_THRESHOLDS.items():
        value = configured.get(name, default)
        if isinstance(value, int) and value > 0:
            thresholds[name] = value
    return thresholds


def internal_include_count(includes: list[str]) -> int:
    return sum(1 for include in includes if "/" in include or "\\" in include)


def analyze_file(
    root: pathlib.Path,
    path: pathlib.Path,
    thresholds: dict[str, int],
    exceptions: set[tuple[str, str]],
) -> FileMetric:
    source = path.read_text(encoding="utf-8", errors="replace")
    relative = path.relative_to(root).as_posix()
    functions = scan_functions(source, relative)
    includes = INCLUDE_PATTERN.findall(source)
    signals: set[str] = set()

    if len(functions) > thresholds["max_functions_per_file"]:
        signals.add("function_count")
    if sum(item.allocation_sites for item in functions) > thresholds["max_allocation_sites"]:
        signals.add("allocation")
    if internal_include_count(includes) > thresholds["max_internal_includes"]:
        signals.add("include_dependencies")
    if any(
        item.body_lines > thresholds["max_function_lines"] for item in functions
    ):
        signals.add("function_length")
    if any(
        item.branch_points > thresholds["max_branch_points"] for item in functions
    ):
        signals.add("branching")
    if any(
        item.parameters > thresholds["max_parameters"]
        and (relative, item.name) not in exceptions
        for item in functions
    ):
        signals.add("parameter_count")

    return FileMetric(
        path=relative,
        line_count=len(source.splitlines()),
        include_count=len(includes),
        internal_include_count=internal_include_count(includes),
        allocation_sites=sum(item.allocation_sites for item in functions),
        functions=functions,
        signals=tuple(sorted(signals)),
    )


def load_reviews(root: pathlib.Path) -> dict[str, set[str]]:
    path = root / "plan" / "architecture_reviews.json"
    data = json.loads(path.read_text(encoding="utf-8"))
    reviews = data.get("reviews")
    if not isinstance(reviews, list):
        raise ValueError("architecture review registry needs a reviews list")
    result: dict[str, set[str]] = {}
    for review in reviews:
        if not isinstance(review, dict):
            raise ValueError("architecture review entries must be objects")
        review_path = review.get("path")
        signals = review.get("signals")
        if (
            not isinstance(review_path, str)
            or not isinstance(signals, list)
            or not isinstance(review.get("issue"), int)
            or review.get("status") != "accepted"
            or not isinstance(review.get("decision"), str)
            or not review["decision"]
        ):
            raise ValueError(
                "architecture review entries need path, signals, issue, accepted status, and decision"
            )
        if review_path in result:
            raise ValueError(f"duplicate architecture review: {review_path}")
        result[review_path] = {str(signal) for signal in signals}
    return result


def validate_reviews(root: pathlib.Path, files: list[FileMetric]) -> None:
    reviews = load_reviews(root)
    required = {file.path: set(file.signals) for file in files if file.signals}
    if set(reviews) != set(required):
        missing = sorted(set(required) - set(reviews))
        stale = sorted(set(reviews) - set(required))
        raise ValueError(f"architecture review mismatch; missing={missing}, stale={stale}")
    for path, signals in required.items():
        if reviews[path] != signals:
            raise ValueError(
                f"architecture review signals mismatch for {path}: "
                f"report={sorted(signals)}, review={sorted(reviews[path])}"
            )


def build_report(root: pathlib.Path) -> dict[str, object]:
    thresholds = load_thresholds(root)
    exceptions = load_exceptions(root)
    files = [analyze_file(root, path, thresholds, exceptions) for path in discover_files(root)]
    return {
        "schema_version": 1,
        "scope": list(SOURCE_ROOTS),
        "line_count_policy": "informational",
        "thresholds": thresholds,
        "review_required_paths": [file.path for file in files if file.signals],
        "files": [file.as_dict() for file in files],
    }


def write_report(report: dict[str, object], output: pathlib.Path | None) -> None:
    rendered = json.dumps(report, indent=2) + "\n"
    if output is None:
        print(rendered, end="")
        return
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(rendered, encoding="utf-8")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=pathlib.Path, default=pathlib.Path(__file__).resolve().parents[1])
    parser.add_argument("--output", type=pathlib.Path)
    parser.add_argument("--check", action="store_true")
    arguments = parser.parse_args(argv)
    root = arguments.root.resolve()
    try:
        report = build_report(root)
        if arguments.check:
            files = [
                FileMetric(
                    path=str(item["path"]),
                    line_count=int(item["line_count"]),
                    include_count=int(item["include_count"]),
                    internal_include_count=int(item["internal_include_count"]),
                    allocation_sites=int(item["allocation_sites"]),
                    functions=tuple(
                        FunctionMetric(**function) for function in item["functions"]
                    ),
                    signals=tuple(str(signal) for signal in item["signals"]),
                )
                for item in report["files"]
            ]
            validate_reviews(root, files)
    except (OSError, ValueError, json.JSONDecodeError, KeyError, TypeError) as error:
        print(f"architecture-report: {error}", file=sys.stderr)
        return 1
    write_report(report, arguments.output)
    print(
        f"architecture-report: {len(report['files'])} files, "
        f"{len(report['review_required_paths'])} responsibility reviews required",
        file=sys.stderr,
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
