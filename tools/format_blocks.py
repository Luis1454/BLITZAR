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
CONTROL_KEYWORD_RE = re.compile(r"^(if|for|while|switch|catch|else|try|do)\b")
TYPE_SCOPE_RE = re.compile(
    r"^(?:(?:template\s*<[^{};]*>\s*)?)(?:namespace|class|struct|enum|union)\b"
)
EXTERN_SCOPE_RE = re.compile(r'^extern\s+"[^\"]+"(?:\s|$)')
RAW_LITERAL_RE = re.compile(r'R"([^\s()\\]{0,16})\(')
MACRO_RE = re.compile(r"^[A-Z_][A-Z0-9_]*\s*\(")
EXIT_RE = re.compile(r"^(?:return|throw|break|continue|co_return|goto)\b")
LABEL_RE = re.compile(r"^(?:case\b.*:|default\s*:)")
NAMED_CAST_RE = re.compile(r"^(?:static|dynamic|reinterpret|const)_cast\s*<")
C_STYLE_CAST_RE = re.compile(
    r"^\(\s*(?:void|bool|char(?:8_t|16_t|32_t)?|wchar_t|short|int|long|float|double|"
    r"unsigned|signed|const\b|volatile\b|[A-Z]\w*(?:::\w+)*|"
    r"[a-z_]\w*::[A-Za-z_]\w*)[^)]*\)"
)
ASSIGNMENT_RE = re.compile(r"(?<![=!<>])(?:\+=|-=|\*=|/=|%=|&=|\|=|\^=|<<=|>>=|=(?!=))")
CALL_RE = re.compile(
    r"^(?:[A-Za-z_]\w*(?:(?:::|\.|->)[A-Za-z_]\w*)*)\s*\("
)
LAMBDA_CAPTURE_RE = re.compile(r"^\[[^\]]*\]")
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
    force_separator: bool = False


def source_files(root: Path) -> list[Path]:
    files: list[Path] = []
    for path in root.rglob("*"):
        if not path.is_file() or path.suffix.lower() not in SOURCE_SUFFIXES:
            continue
        if any(part == ".git" or part == "build" or part.startswith("build-") for part in path.parts):
            continue
        files.append(path)
    return sorted(files)


def mask_code(
    line: str, in_block_comment: bool, raw_delimiter: str | None
) -> tuple[str, bool, str | None]:
    """Return a same-length view with comments and literals replaced by spaces."""
    masked = list(line)
    index = 0
    while index < len(line):
        if raw_delimiter is not None:
            terminator = ")" + raw_delimiter + '"'
            end = line.find(terminator, index)
            stop = len(line) if end < 0 else end + len(terminator)
            for position in range(index, stop):
                masked[position] = " "
            index = stop
            if end < 0:
                return "".join(masked), in_block_comment, raw_delimiter
            raw_delimiter = None
            continue

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

        raw_match = RAW_LITERAL_RE.match(line, index)
        if raw_match is not None:
            raw_delimiter = raw_match.group(1)
            terminator = ")" + raw_delimiter + '"'
            end = line.find(terminator, raw_match.end())
            stop = len(line) if end < 0 else end + len(terminator)
            for position in range(index, stop):
                masked[position] = " "
            index = stop
            if end >= 0:
                raw_delimiter = None
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

    return "".join(masked), in_block_comment, raw_delimiter


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


def looks_like_function_start(text: str) -> bool:
    stripped = text.strip()
    if not stripped or ";" in stripped or "=" in stripped:
        return False
    if CONTROL_RE.match(stripped) or TYPE_SCOPE_RE.match(stripped):
        return False
    prefix = stripped.split("(", 1)[0]
    return "(" in stripped and (re.search(r"\s", prefix) is not None or "::" in prefix)


def looks_like_lambda_signature(text: str) -> bool:
    stripped = text.strip()
    if "=" not in stripped or ";" in stripped:
        return False
    capture = re.search(r"\[[^\]]*\]", stripped)
    if capture is None or not stripped[: capture.start()].rstrip().endswith("="):
        return False
    suffix = stripped[capture.end() :].strip()
    return not suffix or suffix.startswith((
        "(", "mutable", "const", "noexcept", "->", "requires", "{"
    ))


def looks_like_lambda_assignment_start(text: str) -> bool:
    stripped = text.strip()
    if ";" in stripped or not stripped.endswith("="):
        return False
    return looks_like_declaration(stripped[:-1].rstrip())


def looks_like_lambda_capture(text: str) -> bool:
    return LAMBDA_CAPTURE_RE.match(text.strip()) is not None


def looks_like_declaration(text: str) -> bool:
    return bool(DECLARATION_RE.match(text.strip()))


def control_category(text: str) -> str | None:
    stripped = text.strip()
    stripped = re.sub(r"^(?:}\s*)+", "", stripped)
    match = CONTROL_KEYWORD_RE.match(stripped)
    if match is None:
        return None
    keyword = match.group(1)
    if keyword == "else":
        return "if"
    if keyword == "catch":
        return "try"
    return keyword


def classify_statement(text: str, previous_category: str | None = None) -> str:
    stripped = text.strip()
    control = control_category(stripped)
    if control is not None:
        if control == "while" and previous_category == "do":
            return "do"
        return control
    if LABEL_RE.match(stripped):
        return "label"
    if MACRO_RE.match(stripped) or stripped.startswith("static_assert"):
        return "assertion"
    if EXIT_RE.match(stripped):
        return "exit"
    if NAMED_CAST_RE.match(stripped) or C_STYLE_CAST_RE.match(stripped):
        return "cast"
    if looks_like_declaration(stripped) or stripped.startswith(("using ", "typedef ")):
        return "declaration"
    if CALL_RE.match(stripped):
        return "call"
    if ASSIGNMENT_RE.search(stripped):
        return "assignment"
    return "expression"


def opening_kind(
    prefix: str, pending_kind: str | None, pending_parens: int, scopes: list[Scope]
) -> str:
    stripped = prefix.strip()
    if pending_kind in {"control", "function"}:
        return pending_kind
    if pending_kind == "lambda":
        return "function"
    if pending_kind == "lambda_candidate" and looks_like_lambda_capture(stripped):
        return "function"
    if pending_kind == "lambda_candidate" and not stripped:
        return "initializer"
    if re.search(r"\b(?:else|catch)\b", stripped):
        return "control"
    if pending_kind is not None and not stripped:
        return pending_kind
    if CONTROL_RE.match(stripped):
        return "control"
    if stripped == "extern" or EXTERN_SCOPE_RE.match(stripped):
        return "type"
    if TYPE_SCOPE_RE.match(stripped):
        return "type"
    if looks_like_lambda_signature(stripped):
        return "function"
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
    force_separator = state.force_separator
    if state.last_category is not None and state.last_line_index is not None:
        between = range(state.last_line_index + 1, line_index)
        blank_lines = [position for position in between if not lines[position].strip()]
        only_blank_lines = len(blank_lines) == line_index - state.last_line_index - 1
        if only_blank_lines:
            if state.last_category != category or force_separator:
                remove_lines.update(blank_lines)
                insert_before.add(line_index)
            elif len(blank_lines) > 1:
                remove_lines.update(blank_lines[1:])
        elif state.last_category != category or force_separator:
            previous = line_index - 1
            while previous > state.last_line_index and not lines[previous].strip():
                previous -= 1
            if previous >= 0 and lines[previous].strip():
                insert_before.add(line_index)
    state.last_category = category
    state.active_category = (
        None
        if category in {
            "control", "if", "for", "while", "switch", "try", "do", "label", "scope"
        }
        else category
    )
    state.last_line_index = line_index
    state.force_separator = False


def remove_continuation_blanks(lines: list[str]) -> list[str]:
    """Keep blank lines out of parenthesized expressions and macro continuations."""
    result: list[str] = []
    paren_depth = 0
    in_block_comment = False
    raw_delimiter: str | None = None
    in_directive = False
    for line in lines:
        preprocessor, in_directive = is_preprocessor(line, in_directive)
        code, in_block_comment, raw_delimiter = mask_code(
            line, in_block_comment, raw_delimiter
        )
        if not line.strip() and paren_depth > 0 and not preprocessor and raw_delimiter is None:
            continue
        result.append(line)
        if not preprocessor:
            paren_depth += code.count("(") - code.count(")")
            paren_depth = max(paren_depth, 0)
    return result


def collapse_blank_runs(lines: list[str]) -> list[str]:
    """Keep at most one blank line without changing comments or raw literals."""
    result: list[str] = []
    in_block_comment = False
    raw_delimiter: str | None = None
    previous_blank = False
    for line in lines:
        code, in_block_comment, raw_delimiter = mask_code(
            line, in_block_comment, raw_delimiter
        )
        blank = not line.strip() and not in_block_comment and raw_delimiter is None
        if blank and previous_blank:
            continue
        result.append(line)
        previous_blank = blank
    return result


def format_lines(lines: list[str]) -> list[str]:
    lines = remove_continuation_blanks(lines)
    scopes: list[Scope] = []
    insert_before: set[int] = set()
    pending_kind: str | None = None
    pending_control_category: str | None = None
    in_block_comment = False
    raw_delimiter: str | None = None
    in_directive = False
    pending_parens = 0
    parenthesis_depth = 0
    remove_lines: set[int] = set()

    for line_index, line in enumerate(lines):
        preprocessor, in_directive = is_preprocessor(line, in_directive)
        if preprocessor:
            continue

        code, in_block_comment, raw_delimiter = mask_code(
            line, in_block_comment, raw_delimiter
        )
        if not code.strip():
            continue

        state = current_scope_state(scopes)
        starts_closing = code.lstrip().startswith("}")
        standalone_opening = code.strip() == "{" and parenthesis_depth == 0
        pending_continuation = pending_kind in {"control", "function", "lambda"} and pending_parens > 0
        if (
            state is not None
            and state.active_category is None
            and not starts_closing
            and not standalone_opening
            and not pending_continuation
        ):
            add_separator(
                state,
                classify_statement(code, state.last_category),
                line_index,
                lines,
                insert_before,
                remove_lines,
            )

        events = [(position, character) for position, character in enumerate(code) if character in "{}"]
        for position, character in events:
            if character == "{":
                inside_initializer = bool(scopes) and scopes[-1].kind == "initializer"
                kind = (
                    "initializer"
                    if code[:position].strip() == ""
                    and (parenthesis_depth > 0 or inside_initializer)
                    else opening_kind(code[:position], pending_kind, pending_parens, scopes)
                )
                if kind == "control":
                    state = current_scope_state(scopes)
                    if state is not None:
                        category = (
                            control_category(code[:position])
                            or pending_control_category
                            or "control"
                        )
                        add_separator(
                            state,
                            category,
                            line_index,
                            lines,
                            insert_before,
                            remove_lines,
                        )
                elif kind == "compound":
                    state = current_scope_state(scopes)
                    if state is not None:
                        add_separator(
                            state,
                            "scope",
                            line_index,
                            lines,
                            insert_before,
                            remove_lines,
                        )
                state = FunctionState() if kind in {"function", "control", "compound"} else None
                scopes.append(Scope(kind, state))
                pending_kind = None
                pending_control_category = None
                pending_parens = 0
                continue

            if not scopes:
                continue
            popped = scopes.pop()
            if popped.kind == "compound":
                state = current_scope_state(scopes)
                if state is not None:
                    state.last_category = "scope"
                    state.active_category = None
                    state.last_line_index = line_index

        state = current_scope_state(scopes)
        if state is not None and state.active_category is not None:
            if code.rstrip().endswith(";"):
                state.force_separator = (
                    state.last_line_index is not None and line_index > state.last_line_index
                )
                state.active_category = None

        if events:
            parenthesis_depth += code.count("(") - code.count(")")
            parenthesis_depth = max(parenthesis_depth, 0)
            continue

        if pending_kind in {"control", "function", "lambda"} and pending_parens > 0:
            pending_parens += code.count("(") - code.count(")")
            if pending_parens > 0:
                parenthesis_depth += code.count("(") - code.count(")")
                parenthesis_depth = max(parenthesis_depth, 0)
                continue
            pending_parens = 0
            if pending_kind == "control" and code.rstrip().endswith(";"):
                pending_kind = None
                pending_control_category = None
            parenthesis_depth += code.count("(") - code.count(")")
            parenthesis_depth = max(parenthesis_depth, 0)
            continue

        stripped = code.strip()
        if CONTROL_RE.match(stripped):
            pending_kind = "control"
            pending_control_category = control_category(stripped)
            pending_parens = code.count("(") - code.count(")")
        elif TYPE_SCOPE_RE.match(stripped):
            pending_kind = "type"
            pending_parens = 0
        elif pending_kind == "lambda_candidate":
            if looks_like_lambda_capture(stripped):
                pending_kind = "lambda"
                pending_parens = code.count("(") - code.count(")")
            else:
                pending_kind = None
                pending_parens = 0
        elif looks_like_lambda_signature(stripped):
            pending_kind = "lambda"
            pending_parens = code.count("(") - code.count(")")
        elif looks_like_function_signature(stripped):
            pending_kind = "function"
            pending_parens = code.count("(") - code.count(")")
        elif current_scope_state(scopes) is None and looks_like_function_start(stripped):
            pending_kind = "function"
            pending_parens = code.count("(") - code.count(")")
        elif pending_kind in {"function", "lambda"} and stripped.startswith(
            ("const", "mutable", "noexcept", "->", "requires")
        ):
            parenthesis_depth += code.count("(") - code.count(")")
            parenthesis_depth = max(parenthesis_depth, 0)
            continue
        elif looks_like_lambda_assignment_start(stripped):
            pending_kind = "lambda_candidate"
            pending_parens = 0
        else:
            pending_kind = None
            pending_control_category = None
            pending_parens = 0

        parenthesis_depth += code.count("(") - code.count(")")
        parenthesis_depth = max(parenthesis_depth, 0)

    result: list[str] = []
    for line_index, line in enumerate(lines):
        if line_index in remove_lines:
            continue
        if line_index in insert_before and result and result[-1].strip():
            result.append("")
        result.append(line)
    return collapse_blank_runs(result)


def detect_newline(text: str) -> tuple[str, bool]:
    crlf_count = text.count("\r\n")
    bare_lf_count = text.count("\n") - crlf_count
    bare_cr_count = text.count("\r") - crlf_count
    newline_kinds = sum(
        count > 0 for count in (crlf_count, bare_lf_count, bare_cr_count)
    )
    mixed = bare_cr_count > 0 or newline_kinds > 1
    if crlf_count >= bare_lf_count and crlf_count > 0:
        return "\r\n", mixed
    return "\n", mixed


def read_lines(path: Path) -> tuple[list[str], str, bool, bool]:
    with path.open("r", encoding="utf-8", newline="") as stream:
        text = stream.read()
    newline, mixed = detect_newline(text)
    return text.splitlines(), newline, text.endswith(("\n", "\r")), mixed


def write_lines(path: Path, lines: list[str], newline: str, trailing_newline: bool) -> None:
    text = newline.join(lines)
    if trailing_newline:
        text += newline
    with path.open("w", encoding="utf-8", newline="") as stream:
        stream.write(text)


def display_path(path: Path, root: Path) -> str:
    try:
        return str(path.relative_to(root))
    except ValueError:
        return str(path)


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
        lines, newline, trailing_newline, mixed_newlines = read_lines(path)
        formatted = format_lines(lines)
        needs_formatting = formatted != lines
        if not needs_formatting and not mixed_newlines:
            continue
        changed.append(path)
        if arguments.write:
            write_lines(path, formatted, newline, trailing_newline)

    if changed and arguments.check:
        for path in changed:
            _, _, _, mixed_newlines = read_lines(path)
            reason = "mixed line endings" if mixed_newlines else "statement grouping"
            print(f"{display_path(path, root)}: {reason}")
        return 1
    if changed and arguments.write:
        print(f"formatted={len(changed)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
