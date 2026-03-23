from __future__ import annotations

import re

IMPLEMENTATION_SCAN_EXTS = {".cpp", ".cc", ".cxx", ".cu", ".inl"}
FUNCTION_TARGET_LINES = 80
FUNCTION_ALERT_LINES = 140
SUBSTANTIAL_FUNCTION_LINES = 40
MULTI_FUNCTION_TARGET = 3
FILE_FUNCTION_TARGET = 8
FILE_FUNCTION_ALERT = 12
FUNCTION_COMPLEXITY_TARGET = 12
FUNCTION_COMPLEXITY_ALERT = 20

FUNCTION_START_RE = re.compile(
    r"[A-Za-z_~][A-Za-z0-9_:<>~*&\s,]*\([^;]*\)\s*(?:const\s*)?"
    r"(?:noexcept\s*)?(?:->\s*[A-Za-z0-9_:<>*&\s]+)?\s*\{"
)
CONTROL_FLOW_PREFIXES = ("if", "for", "while", "switch", "catch")
NON_FUNCTION_PREFIXES = ("class", "struct", "enum", "union", "namespace", "else", "do", "try")
COMPLEXITY_RE = re.compile(r"\bif\b|\bfor\b|\bwhile\b|\bcase\b|\bcatch\b|&&|\|\||\?")


def collect_function_decomposition_warnings(rel: str, content: str) -> list[str]:
    warnings: list[str] = []
    functions = _collect_function_metrics(content)
    substantial = [metric for metric in functions if metric["lines"] >= SUBSTANTIAL_FUNCTION_LINES]
    if len(functions) > FILE_FUNCTION_TARGET:
        level = "strong function-count alert" if len(functions) > FILE_FUNCTION_ALERT else "function-count warning"
        warnings.append(
            f"{rel}: {level}; {len(functions)} implementation functions exceeds target {FILE_FUNCTION_TARGET} "
            "for one primary responsibility"
        )

    for metric in functions:
        if metric["lines"] > FUNCTION_TARGET_LINES:
            level = "strong function-size alert" if metric["lines"] > FUNCTION_ALERT_LINES else "function-size warning"
            warnings.append(
                f"{rel}:{metric['start_line']}: {level}; function spans {metric['lines']} lines "
                f"(target <= {FUNCTION_TARGET_LINES})"
            )
        if metric["complexity"] > FUNCTION_COMPLEXITY_TARGET:
            level = "strong complexity alert" if metric["complexity"] > FUNCTION_COMPLEXITY_ALERT else "complexity warning"
            warnings.append(
                f"{rel}:{metric['start_line']}: {level}; estimated branching complexity {metric['complexity']} "
                f"exceeds target {FUNCTION_COMPLEXITY_TARGET}"
            )

    if len(substantial) >= MULTI_FUNCTION_TARGET:
        lines = ", ".join(str(metric["start_line"]) for metric in substantial[:MULTI_FUNCTION_TARGET])
        warnings.append(
            f"{rel}: contains {len(substantial)} substantial functions "
            f"(>= {SUBSTANTIAL_FUNCTION_LINES} lines at {lines}); review file responsibility before it grows"
        )

    return warnings


def _collect_function_metrics(content: str) -> list[dict[str, int]]:
    lines = content.splitlines()
    metrics: list[dict[str, int]] = []
    index = 0
    while index < len(lines):
        signature_start = _find_function_signature_start(lines, index)
        if signature_start is None:
            index += 1
            continue
        (start_index, open_index) = signature_start
        end_index = _find_matching_block_end(lines, open_index)
        if end_index is None:
            index = open_index + 1
            continue
        metrics.append({
            "start_line": start_index + 1,
            "lines": end_index - start_index + 1,
            "complexity": _estimate_branching_complexity(lines[start_index:end_index + 1]),
        })
        index = end_index + 1
    return metrics


def _find_function_signature_start(lines: list[str], index: int) -> tuple[int, int] | None:
    line = lines[index].strip()
    if not line or line.startswith("#"):
        return None

    signature_lines: list[str] = []
    start_index = index
    current = index
    while current < len(lines):
        stripped = lines[current].strip()
        if not stripped:
            if signature_lines:
                break
            return None
        if stripped.startswith("#"):
            return None
        signature_lines.append(stripped)
        candidate = " ".join(signature_lines)
        if "{" in stripped:
            if _is_function_signature(candidate):
                return (start_index, current)
            return None
        if ";" in stripped:
            return None
        current += 1
    return None


def _is_function_signature(candidate: str) -> bool:
    normalized = candidate.strip()
    if not FUNCTION_START_RE.search(normalized):
        return False
    prefix = normalized.split("(", maxsplit=1)[0].strip().split()[-1].lower()
    if prefix in CONTROL_FLOW_PREFIXES:
        return False
    if normalized.lower().startswith(NON_FUNCTION_PREFIXES):
        return False
    return True


def _find_matching_block_end(lines: list[str], start_index: int) -> int | None:
    depth = 0
    for index in range(start_index, len(lines)):
        line = lines[index]
        depth += line.count("{")
        depth -= line.count("}")
        if depth == 0 and "}" in line:
            return index
    return None


def _estimate_branching_complexity(function_lines: list[str]) -> int:
    body = "\n".join(function_lines)
    return 1 + len(COMPLEXITY_RE.findall(body))
