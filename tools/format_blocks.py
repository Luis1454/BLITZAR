#!/usr/bin/env python3
"""Insert blank lines between statement categories in nested C++ statement scopes."""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path


SOURCE_SUFFIXES = {
    ".c",
    ".cc",
    ".cpp",
    ".cxx",
    ".h",
    ".hh",
    ".hpp",
    ".hxx",
    ".hip",
    ".inl",
}

CONTROL_RE = re.compile(r"^(?:if|for|while|switch|catch|else|try|do)\b")
TYPE_SCOPE_RE = re.compile(r"^(?:namespace|class|struct|enum|union)\b")
MACRO_RE = re.compile(r"^[A-Z_][A-Z0-9_]*\s*\(")
EXIT_RE = re.compile(r"^(?:return|throw|break|continue|co_return|goto)\b")
LABEL_RE = re.compile(r"^(?:case\b.*:|default\s*:)")
QUALIFIER_RE = r"(?:(?:const|constexpr|static|thread_local|volatile|mutable|inline|extern|register)\s+)*"
TYPE_RE = (
    r"(?:"
    r"(?:void|bool|char|char8_t|char16_t|char32_t|wchar_t|short|int|long|float|double|auto)"
    r"|decltype\s*\([^;]*\)"
    r"|(?:(?:struct|class|enum)\s+)?[A-Za-z_]\w*(?:::[A-Za-z_]\w*)*"
    r"(?:\s*<[^;{}]*>)?"
    r")"
)
DECLARATION_RE = re.compile(
    rf"^{QUALIFIER_RE}{TYPE_RE}\s+(?:[*&]\s*)?(?:[A-Za-z_]\w*|\[[^]]+\])"
    r"(?:\s*(?:=|\{|\(|\[|,|;|:)|\s*$)"
)


@dataclass
class Scope:
    kind: str
    state: FunctionState | None = None


@dataclass
class FunctionState:
    last_category: str | None = None
    active_category: str | None = None
    last_line_index: int | None = None


def source_files(root: Path) -> list[Path]:
    files: list[Path] = []
    for path in root.rglob("*"):
        if not path.is_file() or path.suffix.lower() not in SOURCE_SUFFIXES:
            continue
        if any(part == ".git" or part == "build" or part.startswith("build-") for part in path.parts):
            continue
        files.append(path)
    return sorted(files)


def mask_code(line: str, in_block_comment: bool) -> tuple[str, bool]:
    """Return a same-length view with comments and literals replaced by spaces."""
    masked = list(line)
    index = 0
    while index < len(line):
        if in_block_comment:
            end = line.find("*/", index)
            stop = len(line) if end < 0 else end + 2
            for position in range(index, stop):
                masked[position] = " "
            index = stop
            in_block_comment = end < 0
            continue

        if line.startswith("//", index):
            for position in range(index, len(line)):
                masked[position] = " "
            break

        if line.startswith("/*", index):
            end = line.find("*/", index + 2)
            stop = len(line) if end < 0 else end + 2
            for position in range(index, stop):
                masked[position] = " "
            index = stop
            in_block_comment = end < 0
            continue

        if line[index] in {"\"", "'"}:
            quote = line[index]
            masked[index] = " "
            index += 1
            while index < len(line):
                escaped = line[index] == "\\"
                masked[index] = " "
                index += 1
                if escaped and index < len(line):
                    masked[index] = " "
                    index += 1
                elif line[index - 1] == quote:
                    break
            continue

        index += 1

    return "".join(masked), in_block_comment


def is_preprocessor(line: str, in_directive: bool) -> tuple[bool, bool]:
    stripped = line.lstrip()
    directive = in_directive or stripped.startswith("#")
    if not directive:
        return False, False
    return True, line.rstrip().endswith("\\")


def looks_like_function_signature(text: str) -> bool:
    stripped = text.strip()
    if not stripped or ";" in stripped or "=" in stripped:
        return False
    if CONTROL_RE.match(stripped) or TYPE_SCOPE_RE.match(stripped):
        return False
    return "(" in stripped and ")" in stripped


def looks_like_declaration(text: str) -> bool:
    return bool(DECLARATION_RE.match(text.strip()))


def classify_statement(text: str) -> str:
    stripped = text.strip()
    if CONTROL_RE.match(stripped):
        return "control"
    if LABEL_RE.match(stripped):
        return "label"
    if MACRO_RE.match(stripped) or stripped.startswith("static_assert"):
        return "assertion"
    if EXIT_RE.match(stripped):
        return "exit"
    if looks_like_declaration(stripped) or stripped.startswith(("using ", "typedef ")):
        return "declaration"
    return "expression"


def opening_kind(
    prefix: str, pending_kind: str | None, pending_parens: int, scopes: list[Scope]
) -> str:
    stripped = prefix.strip()
    if pending_kind == "control" and (pending_parens > 0 or not stripped):
        return "control"
    if re.search(r"\b(?:else|catch)\b", stripped):
        return "control"
    if pending_kind is not None and not stripped:
        return pending_kind
    if CONTROL_RE.match(stripped):
        return "control"
    if TYPE_SCOPE_RE.match(stripped):
        return "type"
    if looks_like_function_signature(stripped):
        return "function"
    if not stripped and any(scope.kind == "function" for scope in scopes):
        return "compound"
    return "initializer"


def current_scope_state(scopes: list[Scope]) -> FunctionState | None:
    if not scopes:
        return None
    return scopes[-1].state


def add_separator(
    state: FunctionState,
    category: str,
    line_index: int,
    lines: list[str],
    insert_before: set[int],
    remove_lines: set[int],
) -> None:
    if state.last_category is not None and state.last_line_index is not None:
        between = range(state.last_line_index + 1, line_index)
        blank_lines = [position for position in between if not lines[position].strip()]
        only_blank_lines = len(blank_lines) == line_index - state.last_line_index - 1
        if only_blank_lines:
            remove_lines.update(blank_lines)
            if state.last_category != category:
                insert_before.add(line_index)
        elif state.last_category != category:
            previous = line_index - 1
            previous_text = lines[previous].strip() if previous >= 0 else ""
            if previous_text and not previous_text.startswith(("//", "/*", "*")):
                insert_before.add(line_index)
    state.last_category = category
    state.active_category = None if category in {"control", "label"} else category
    state.last_line_index = line_index


def remove_continuation_blanks(lines: list[str]) -> list[str]:
    """Keep blank lines out of parenthesized expressions and macro continuations."""
    result: list[str] = []
    paren_depth = 0
    in_block_comment = False
    in_directive = False
    for line in lines:
        preprocessor, in_directive = is_preprocessor(line, in_directive)
        code, in_block_comment = mask_code(line, in_block_comment)
        if not line.strip() and paren_depth > 0 and not preprocessor:
            continue
        result.append(line)
        if not preprocessor:
            paren_depth += code.count("(") - code.count(")")
            paren_depth = max(paren_depth, 0)
    return result


def format_lines(lines: list[str]) -> list[str]:
    lines = remove_continuation_blanks(lines)
    scopes: list[Scope] = []
    insert_before: set[int] = set()
    pending_kind: str | None = None
    in_block_comment = False
    in_directive = False
    pending_parens = 0
    remove_lines: set[int] = set()

    for line_index, line in enumerate(lines):
        preprocessor, in_directive = is_preprocessor(line, in_directive)
        if preprocessor:
            continue

        code, in_block_comment = mask_code(line, in_block_comment)
        if not code.strip():
            continue

        state = current_scope_state(scopes)
        starts_closing = code.lstrip().startswith("}")
        pending_continuation = pending_kind == "control" and pending_parens > 0
        if (
            state is not None
            and state.active_category is None
            and not starts_closing
            and not pending_continuation
        ):
            add_separator(
                state,
                classify_statement(code),
                line_index,
                lines,
                insert_before,
                remove_lines,
            )

        events = [(position, character) for position, character in enumerate(code) if character in "{}"]
        for position, character in events:
            if character == "{":
                kind = opening_kind(code[:position], pending_kind, pending_parens, scopes)
                if kind == "control":
                    state = current_scope_state(scopes)
                    if state is not None:
                        add_separator(
                            state,
                            "control",
                            line_index,
                            lines,
                            insert_before,
                            remove_lines,
                        )
                state = FunctionState() if kind in {"function", "control", "compound"} else None
                scopes.append(Scope(kind, state))
                pending_kind = None
                pending_parens = 0
                continue

            if not scopes:
                continue
            popped = scopes.pop()

        state = current_scope_state(scopes)
        if state is not None and state.active_category is not None:
            if code.rstrip().endswith(";"):
                state.active_category = None

        if events:
            continue

        if pending_kind == "control" and pending_parens > 0:
            pending_parens += code.count("(") - code.count(")")
            if pending_parens > 0:
                continue
            pending_kind = None
            pending_parens = 0
            continue

        stripped = code.strip()
        if CONTROL_RE.match(stripped):
            pending_kind = "control"
            pending_parens = code.count("(") - code.count(")")
        elif TYPE_SCOPE_RE.match(stripped):
            pending_kind = "type"
            pending_parens = 0
        elif looks_like_function_signature(stripped):
            pending_kind = "function"
            pending_parens = code.count("(") - code.count(")")
        elif pending_kind == "function" and stripped.startswith(("const", "noexcept", "requires")):
            continue
        else:
            pending_kind = None
            pending_parens = 0

    result: list[str] = []
    for line_index, line in enumerate(lines):
        if line_index in remove_lines:
            continue
        if line_index in insert_before and result and result[-1].strip():
            result.append("")
        result.append(line)
    return result


def read_lines(path: Path) -> tuple[list[str], str, bool]:
    with path.open("r", encoding="utf-8", newline="") as stream:
        text = stream.read()
    newline = "\r\n" if "\r\n" in text else "\n"
    return text.splitlines(), newline, text.endswith(("\n", "\r"))


def write_lines(path: Path, lines: list[str], newline: str, trailing_newline: bool) -> None:
    text = newline.join(lines)
    if trailing_newline:
        text += newline
    with path.open("w", encoding="utf-8", newline="") as stream:
        stream.write(text)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="fail when files need formatting")
    parser.add_argument("--write", action="store_true", help="write formatted files")
    parser.add_argument("paths", nargs="*", type=Path)
    arguments = parser.parse_args()
    if arguments.check == arguments.write:
        parser.error("choose exactly one of --check or --write")

    root = Path(__file__).resolve().parents[1]
    paths = arguments.paths or source_files(root)
    changed: list[Path] = []
    for path in paths:
        if not path.is_absolute():
            path = root / path
        lines, newline, trailing_newline = read_lines(path)
        formatted = format_lines(lines)
        if formatted == lines:
            continue
        changed.append(path)
        if arguments.write:
            write_lines(path, formatted, newline, trailing_newline)

    if changed and arguments.check:
        for path in changed:
            print(path.relative_to(root))
        return 1
    if changed and arguments.write:
        print(f"formatted={len(changed)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
