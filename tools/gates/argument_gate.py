"""Check explicit parameter counts in project-owned C++ callables."""

from __future__ import annotations

import argparse
import dataclasses
import json
import pathlib
import re
import sys


SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".cu", ".h", ".hip", ".hpp", ".inl"}
CONTROL_NAMES = {"catch", "for", "if", "switch", "while"}
TYPE_MARKERS = {
    "auto",
    "bool",
    "char",
    "class",
    "const",
    "double",
    "explicit",
    "float",
    "inline",
    "int",
    "long",
    "noexcept",
    "short",
    "signed",
    "static",
    "struct",
    "template",
    "unsigned",
    "void",
}
NAME_PATTERN = re.compile(
    r"(?P<name>(?:~?[A-Za-z_]\w*)(?:::(?:~?[A-Za-z_]\w*))*)\s*$"
)


@dataclasses.dataclass(frozen=True)
class Callable:
    path: str
    line: int
    name: str
    parameters: int


def strip_comments_and_literals(source: str) -> str:
    result = list(source)
    index = 0
    state = "code"

    while index < len(source):
        current = source[index]
        following = source[index + 1] if index + 1 < len(source) else ""

        if state == "code":
            if current == "/" and following == "/":
                result[index] = result[index + 1] = " "
                index += 2
                state = "line_comment"
                continue
            if current == "/" and following == "*":
                result[index] = result[index + 1] = " "
                index += 2
                state = "block_comment"
                continue
            if current in {'"', "'"}:
                result[index] = " "
                state = "string" if current == '"' else "character"
            index += 1
            continue

        if state == "line_comment":
            if current == "\n":
                state = "code"
            else:
                result[index] = " "
            index += 1
            continue

        if state == "block_comment":
            if current == "*" and following == "/":
                result[index] = result[index + 1] = " "
                index += 2
                state = "code"
            else:
                if current != "\n":
                    result[index] = " "
                index += 1
            continue

        if current == "\\":
            result[index] = " "
            if index + 1 < len(source) and source[index + 1] != "\n":
                result[index + 1] = " "
                index += 2
            else:
                index += 1
            continue
        if (state == "string" and current == '"') or (
            state == "character" and current == "'"
        ):
            result[index] = " "
            state = "code"
        elif current != "\n":
            result[index] = " "
        index += 1

    return "".join(result)


def matching_parenthesis(source: str, opening: int) -> int | None:
    depth = 0
    for index in range(opening, len(source)):
        if source[index] == "(":
            depth += 1
        elif source[index] == ")":
            depth -= 1
            if depth == 0:
                return index
    return None


def parameter_count(parameters: str) -> int:
    if not parameters.strip() or parameters.strip() == "void":
        return 0

    parentheses = 0
    braces = 0
    brackets = 0
    angles = 0
    commas = 0

    for character in parameters:
        if character == "(":
            parentheses += 1
        elif character == ")":
            parentheses -= 1
        elif character == "{":
            braces += 1
        elif character == "}":
            braces -= 1
        elif character == "[":
            brackets += 1
        elif character == "]":
            brackets -= 1
        elif character == "<":
            angles += 1
        elif character == ">" and angles > 0:
            angles -= 1
        elif character == "," and not any((parentheses, braces, brackets, angles)):
            commas += 1

    return commas + 1


def line_number(source: str, offset: int) -> int:
    return source.count("\n", 0, offset) + 1


def is_callable_candidate(source: str, opening: int, name: str) -> bool:
    if name.startswith("std::") or name.split("::")[-1] in CONTROL_NAMES or name.isupper():
        return False

    line_start = source.rfind("\n", 0, opening) + 1
    prefix = source[line_start:opening]
    if any(token in prefix for token in ("return ", "=", ".", "->")):
        return False

    words = set(re.findall(r"[A-Za-z_]\w*", prefix))
    qualified_type = any(word.startswith(("blitzar", "std")) for word in words)
    return bool(words & TYPE_MARKERS) or qualified_type or "::" in prefix


def scan_source(source: str, path: str, max_parameters: int) -> list[Callable]:
    clean = strip_comments_and_literals(source)
    findings: list[Callable] = []
    opening = clean.find("(")

    while opening >= 0:
        closing = matching_parenthesis(clean, opening)
        if closing is None:
            break

        prefix_match = NAME_PATTERN.search(clean[:opening])
        if prefix_match is not None:
            name = prefix_match.group("name")
            if is_callable_candidate(clean, opening, name):
                count = parameter_count(clean[opening + 1 : closing])
                if count > max_parameters:
                    findings.append(Callable(path, line_number(clean, opening), name, count))

        opening = clean.find("(", closing + 1)

    return findings


def load_exceptions(root: pathlib.Path) -> set[tuple[str, str]]:
    path = root / "plan" / "parameter_exceptions.json"
    if not path.exists():
        return set()
    data = json.loads(path.read_text(encoding="utf-8"))
    return {
        (str(entry["path"]).replace("\\", "/"), str(entry["name"]))
        for entry in data
    }


def discover_files(root: pathlib.Path) -> list[pathlib.Path]:
    return sorted(
        path
        for base in (root / "include", root / "src")
        if base.is_dir()
        for path in base.rglob("*")
        if path.is_file() and path.suffix.lower() in SOURCE_SUFFIXES
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=pathlib.Path, default=pathlib.Path(__file__).resolve().parents[2])
    parser.add_argument("--max-parameters", type=int, default=4)
    parser.add_argument("--report-only", action="store_true")
    arguments = parser.parse_args()
    root = arguments.root.resolve()
    exceptions = load_exceptions(root)
    findings: list[Callable] = []

    for path in discover_files(root):
        relative = path.relative_to(root).as_posix()
        findings.extend(
            finding
            for finding in scan_source(path.read_text(encoding="utf-8"), relative, arguments.max_parameters)
            if (finding.path, finding.name) not in exceptions
        )

    for finding in findings:
        print(f"{finding.path}:{finding.line}: {finding.name} has {finding.parameters} parameters")

    if findings and not arguments.report_only:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
