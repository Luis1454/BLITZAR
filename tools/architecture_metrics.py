"""Language-aware metrics used by the repository architecture gate."""

from __future__ import annotations

import ast
import dataclasses
import pathlib
import re

from argument_gate import (
    NAME_PATTERN,
    is_callable_candidate,
    matching_parenthesis,
    parameter_count,
    strip_comments_and_literals,
)


SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".cu", ".cuh", ".h", ".hip", ".hpp", ".inl"}
BRANCH_PATTERN = re.compile(r"\b(?:if|else|for|while|switch|case|catch)\b|&&|\|\||\?")
ALLOCATION_PATTERN = re.compile(
    r"\b(?:new|delete|malloc|calloc|realloc|free|make_unique|make_shared|"
    r"reserve|resize|emplace_back|push_back)\s*(?:<|\(|;|$)"
)
INCLUDE_PATTERN = re.compile(r"^\s*#\s*include\s*[<\"]([^>\"]+)[>\"]", re.MULTILINE)
CMAKE_FUNCTION_PATTERN = re.compile(
    r"(?im)^\s*(?:function|macro)\s*\(\s*([A-Za-z_]\w*)\s*([^)]*)\)"
)
CMAKE_BRANCH_PATTERN = re.compile(r"\b(?:if|elseif|else|foreach|while)\b", re.IGNORECASE)
PYTHON_BRANCH_NODES = (
    ast.If,
    ast.For,
    ast.AsyncFor,
    ast.While,
    ast.Try,
    ast.IfExp,
    ast.Match,
)


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
    profile: str = "production"
    language: str = "cpp"

    def as_dict(self) -> dict[str, object]:
        max_parameters = max((item.parameters for item in self.functions), default=0)
        max_function_lines = max((item.body_lines for item in self.functions), default=0)
        max_branch_points = max((item.branch_points for item in self.functions), default=0)
        return {
            "path": self.path,
            "profile": self.profile,
            "language": self.language,
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


def python_branch_points(node: ast.AST) -> int:
    points = sum(isinstance(item, PYTHON_BRANCH_NODES) for item in ast.walk(node))
    points += sum(
        max(0, len(item.values) - 1)
        for item in ast.walk(node)
        if isinstance(item, ast.BoolOp)
    )
    points += sum(len(item.handlers) for item in ast.walk(node) if isinstance(item, ast.Try))
    return points


def python_allocation_sites(node: ast.AST) -> int:
    allocation_names = {"bytearray", "dict", "list", "open", "set", "tuple"}
    calls = sum(
        1
        for item in ast.walk(node)
        if isinstance(item, ast.Call)
        and (
            isinstance(item.func, ast.Attribute)
            and item.func.attr in {"append", "extend", "insert"}
            or isinstance(item.func, ast.Name)
            and item.func.id in allocation_names
        )
    )
    comprehensions = sum(
        isinstance(item, (ast.ListComp, ast.SetComp, ast.DictComp, ast.GeneratorExp))
        for item in ast.walk(node)
    )
    return calls + comprehensions


def scan_python_functions(source: str, path: str) -> tuple[FunctionMetric, ...]:
    try:
        tree = ast.parse(source, filename=path)
    except SyntaxError:
        return ()

    functions: list[FunctionMetric] = []
    for node in ast.walk(tree):
        if not isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
            continue
        arguments = node.args
        parameters = (
            len(arguments.posonlyargs)
            + len(arguments.args)
            + len(arguments.kwonlyargs)
            + int(arguments.vararg is not None)
            + int(arguments.kwarg is not None)
        )
        end_line = getattr(node, "end_lineno", node.lineno)
        functions.append(
            FunctionMetric(
                name=node.name,
                line=node.lineno,
                parameters=parameters,
                body_lines=end_line - node.lineno + 1,
                branch_points=python_branch_points(node),
                allocation_sites=python_allocation_sites(node),
            )
        )
    return tuple(sorted(functions, key=lambda item: (item.line, item.name)))


def scan_cmake_functions(source: str, path: str) -> tuple[FunctionMetric, ...]:
    functions: list[FunctionMetric] = []
    for match in CMAKE_FUNCTION_PATTERN.finditer(source):
        end_match = re.search(
            r"(?im)^\s*end(?:function|macro)\s*\(", source[match.end() :]
        )
        if end_match is None:
            continue
        body_start = match.end()
        body_end = body_start + end_match.start()
        body = source[body_start:body_end]
        parameters = len([item for item in match.group(2).split() if item])
        functions.append(
            FunctionMetric(
                name=match.group(1),
                line=line_number(source, match.start()),
                parameters=parameters,
                body_lines=body.count("\n") + 1,
                branch_points=len(CMAKE_BRANCH_PATTERN.findall(body)),
                allocation_sites=0,
            )
        )
    return tuple(functions)


def scan_file_functions(source: str, path: str) -> tuple[FunctionMetric, ...]:
    suffix = pathlib.Path(path).suffix.lower()
    if suffix == ".py":
        return scan_python_functions(source, path)
    if suffix == ".cmake" or pathlib.Path(path).name == "CMakeLists.txt":
        return scan_cmake_functions(source, path)
    if suffix in SOURCE_SUFFIXES:
        return scan_functions(source, path)
    return ()


def dependency_names(source: str, path: pathlib.Path) -> list[str]:
    suffix = path.suffix.lower()
    if suffix in SOURCE_SUFFIXES:
        return INCLUDE_PATTERN.findall(source)
    if suffix == ".py":
        try:
            tree = ast.parse(source, filename=path.as_posix())
        except SyntaxError:
            return []
        dependencies: list[str] = []
        for node in ast.walk(tree):
            if isinstance(node, ast.Import):
                dependencies.extend(alias.name for alias in node.names)
            elif isinstance(node, ast.ImportFrom):
                dependencies.append("." * node.level + (node.module or ""))
        return dependencies
    if suffix == ".cmake" or path.name == "CMakeLists.txt":
        return re.findall(
            r"(?im)^\s*(?:include|find_package|add_subdirectory|target_link_libraries)"
            r"\s*\(([^)]+)\)",
            source,
        )
    return []


def internal_dependency_count(
    root: pathlib.Path,
    path: pathlib.Path,
    dependencies: list[str],
) -> int:
    suffix = path.suffix.lower()
    if suffix in SOURCE_SUFFIXES:
        return sum(1 for include in dependencies if "/" in include or "\\" in include)
    if suffix == ".py":
        local_modules = {item.stem for item in (root / "tools").glob("*.py")}
        return sum(item.lstrip(".").split(".", 1)[0] in local_modules for item in dependencies)
    if suffix == ".cmake" or path.name == "CMakeLists.txt":
        return len(dependencies)
    return 0
