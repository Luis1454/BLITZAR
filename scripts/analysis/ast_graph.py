#!/usr/bin/env python3
"""Build a repository-level C++/CUDA entity graph.

This intentionally avoids compiler-specific AST tooling so it can run in the
minimal CI/dev shells used by BLITZAR. It extracts the structural facts needed
for architecture review: entities, inheritance, composition-like fields,
signature dependencies, and local include edges.
"""

from __future__ import annotations

import argparse
import csv
import json
import re
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable


SOURCE_SUFFIXES = {".cpp", ".hpp", ".cu", ".inl"}
SKIP_DIRS = {
    ".git",
    ".pytest_cache",
    "build",
    "build-debug",
    "build-release",
    "third_party",
}
ROOTS = ("apps", "engine", "modules", "runtime", "tests")
TYPE_WORDS = {
    "auto",
    "bool",
    "char",
    "const",
    "double",
    "enum",
    "explicit",
    "float",
    "friend",
    "inline",
    "int",
    "long",
    "mutable",
    "noexcept",
    "private",
    "protected",
    "public",
    "short",
    "signed",
    "static",
    "struct",
    "typename",
    "unsigned",
    "virtual",
    "void",
    "volatile",
}
TEMPLATE_TYPES = {"array", "optional", "pair", "span", "tuple", "unique_ptr", "vector"}


@dataclass
class Entity:
    id: str
    name: str
    kind: str
    file: str
    line: int
    definition: bool = False
    namespace: str = ""
    bases: list[str] = field(default_factory=list)
    fields: list[dict[str, str]] = field(default_factory=list)
    methods: list[dict[str, str]] = field(default_factory=list)


@dataclass
class Function:
    id: str
    name: str
    file: str
    line: int
    owner: str = ""
    return_type: str = ""
    params: list[str] = field(default_factory=list)


def rel(path: Path, root: Path) -> str:
    return path.relative_to(root).as_posix()


def iter_sources(root: Path) -> Iterable[Path]:
    for base_name in ROOTS:
        base = root / base_name
        if not base.exists():
            continue
        for path in base.rglob("*"):
            if any(part in SKIP_DIRS for part in path.parts):
                continue
            if path.is_file() and path.suffix in SOURCE_SUFFIXES:
                yield path


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group(0).count("\n"), text, flags=re.S)
    text = re.sub(r"//.*", "", text)
    return text


def line_for_offset(text: str, offset: int) -> int:
    return text.count("\n", 0, offset) + 1


def find_matching(text: str, start: int, open_char: str = "{", close_char: str = "}") -> int:
    depth = 0
    i = start
    while i < len(text):
        char = text[i]
        if char == open_char:
            depth += 1
        elif char == close_char:
            depth -= 1
            if depth == 0:
                return i
        i += 1
    return -1


def split_top_level(text: str, delimiter: str) -> list[str]:
    chunks: list[str] = []
    depth_angle = 0
    depth_paren = 0
    depth_brace = 0
    depth_bracket = 0
    start = 0
    for index, char in enumerate(text):
        if char == "<":
            depth_angle += 1
        elif char == ">" and depth_angle:
            depth_angle -= 1
        elif char == "(":
            depth_paren += 1
        elif char == ")" and depth_paren:
            depth_paren -= 1
        elif char == "{":
            depth_brace += 1
        elif char == "}" and depth_brace:
            depth_brace -= 1
        elif char == "[":
            depth_bracket += 1
        elif char == "]" and depth_bracket:
            depth_bracket -= 1
        elif (
            char == delimiter
            and depth_angle == 0
            and depth_paren == 0
            and depth_brace == 0
            and depth_bracket == 0
        ):
            chunks.append(text[start:index])
            start = index + 1
    tail = text[start:]
    if tail.strip():
        chunks.append(tail)
    return chunks


def normalize_type_name(raw: str) -> str:
    cleaned = re.sub(r"\b(const|volatile|static|inline|virtual|explicit|friend|mutable)\b", " ", raw)
    cleaned = re.sub(r"\b(class|struct|enum)\b", " ", cleaned)
    cleaned = re.sub(r"[*&\[\](),=]", " ", cleaned)
    cleaned = cleaned.replace("::", " ")
    tokens = re.findall(r"[A-Za-z_][A-Za-z0-9_]*", cleaned)
    if not tokens:
        return ""
    tokens = [token for token in tokens if token not in TYPE_WORDS and token not in TEMPLATE_TYPES]
    return tokens[-1] if tokens else ""


def extract_type_names(raw: str) -> list[str]:
    raw = re.sub(r'"(?:\\.|[^"])*"', " ", raw)
    tokens = re.findall(r"[A-Za-z_][A-Za-z0-9_]*(?:::[A-Za-z_][A-Za-z0-9_]*)*", raw)
    result: list[str] = []
    for token in tokens:
        leaf = token.split("::")[-1]
        if leaf in TYPE_WORDS or leaf in TEMPLATE_TYPES:
            continue
        if leaf and leaf[0].isupper() and leaf not in result:
            result.append(leaf)
    return result


def resolve_include(include: str, including_file: Path, root: Path, known: set[str]) -> str:
    candidates = [
        including_file.parent / include,
        root / include,
        root / "engine" / "include" / include,
        root / "runtime" / "include" / include,
        root / "modules" / "qt" / "src" / include,
        root / "engine" / "src" / "cuda" / include,
    ]
    for candidate in candidates:
        if candidate.exists():
            return rel(candidate.resolve(), root)
    return include if include in known else ""


def namespace_at(text: str, offset: int) -> str:
    before = text[:offset]
    matches = list(re.finditer(r"\bnamespace\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{", before))
    if not matches:
        return ""
    return matches[-1].group(1)


def parse_bases(base_text: str) -> list[str]:
    bases: list[str] = []
    for item in split_top_level(base_text, ","):
        item = re.sub(r"\b(public|protected|private|virtual)\b", " ", item)
        name = normalize_type_name(item)
        if name and name not in bases:
            bases.append(name)
    return bases


def parse_fields_and_methods(body: str) -> tuple[list[dict[str, str]], list[dict[str, str]]]:
    fields: list[dict[str, str]] = []
    methods: list[dict[str, str]] = []
    body = re.sub(r"\b(public|protected|private)\s*:", "", body)
    for statement in split_top_level(body, ";"):
        stmt = " ".join(statement.strip().split())
        if not stmt or stmt.startswith(("using ", "typedef ", "static_assert", "#")):
            continue
        if "{" in stmt or "}" in stmt:
            continue
        if "(" in stmt and ")" in stmt:
            head = stmt.split("(", 1)[0].strip()
            params = stmt.rsplit(")", 1)[0].split("(", 1)[1]
            method_name = head.split()[-1] if head.split() else ""
            methods.append(
                {
                    "name": method_name.replace("~", ""),
                    "return_type": head.rsplit(" ", 1)[0] if " " in head else "",
                    "params": params,
                }
            )
            continue
        if not re.search(r"[A-Za-z_][A-Za-z0-9_]*\s*(?:\[[^\]]*\])?\s*(?:=.*)?$", stmt):
            continue
        field_decl = stmt.split("=", 1)[0].strip()
        field_name = re.sub(r"\[[^\]]*\]", "", field_decl.split(",")[0]).split()[-1]
        field_name = field_name.lstrip("*&")
        field_type = field_decl.rsplit(field_name, 1)[0].strip(" *&")
        if field_name and field_type:
            fields.append({"name": field_name, "type": field_type})
    return fields, methods


def parse_entities(path: Path, root: Path, text: str) -> tuple[list[Entity], list[Function]]:
    entities: list[Entity] = []
    functions: list[Function] = []
    cleaned = strip_comments(text)
    entity_re = re.compile(
        r"\b(?P<kind>class|struct|enum\s+class|enum)\s+"
        r"(?:alignas\s*\([^)]*\)\s*)?"
        r"(?P<name>[A-Za-z_][A-Za-z0-9_]*)"
        r"\s*(?:final\s*)?"
        r"(?::(?P<bases>[^{;]+))?\s*(?P<term>[{;])",
        re.M,
    )
    for match in entity_re.finditer(cleaned):
        kind = match.group("kind").replace(" ", "_")
        name = match.group("name")
        file_path = rel(path, root)
        entity_id = f"{file_path}:{name}:{match.start()}"
        entity = Entity(
            id=entity_id,
            name=name,
            kind=kind,
            file=file_path,
            line=line_for_offset(cleaned, match.start()),
            definition=match.group("term") == "{",
            namespace=namespace_at(cleaned, match.start()),
            bases=parse_bases(match.group("bases") or ""),
        )
        if match.group("term") == "{":
            open_index = match.end() - 1
            close_index = find_matching(cleaned, open_index)
            if close_index > open_index:
                body = cleaned[open_index + 1 : close_index]
                entity.fields, entity.methods = parse_fields_and_methods(body)
        entities.append(entity)

    function_re = re.compile(
        r"(?P<ret>(?:[A-Za-z_][A-Za-z0-9_:<>*&,\s]+?)?)\s+"
        r"(?:(?P<owner>[A-Za-z_][A-Za-z0-9_]*)::)?"
        r"(?P<name>[A-Za-z_~][A-Za-z0-9_]*)\s*"
        r"\((?P<params>[^;{}()]*(?:\([^)]*\)[^;{}()]*)*)\)\s*"
        r"(?:const\s*)?(?:noexcept\s*)?(?:->\s*[A-Za-z_][A-Za-z0-9_:<>*&,\s]+)?\s*\{",
        re.M,
    )
    for match in function_re.finditer(cleaned):
        name = match.group("name")
        if name in {"if", "for", "while", "switch", "catch"}:
            continue
        file_path = rel(path, root)
        owner = match.group("owner") or ""
        func_id = f"{file_path}:{owner}::{name}:{match.start()}"
        params = [part.strip() for part in split_top_level(match.group("params") or "", ",")]
        functions.append(
            Function(
                id=func_id,
                name=name,
                file=file_path,
                line=line_for_offset(cleaned, match.start()),
                owner=owner,
                return_type=(match.group("ret") or "").strip(),
                params=[param for param in params if param],
            )
        )
    return entities, functions


def build_graph(root: Path) -> dict[str, object]:
    sources = sorted(iter_sources(root))
    known = {rel(path, root) for path in sources}
    entities: list[Entity] = []
    functions: list[Function] = []
    includes: list[dict[str, str]] = []
    for path in sources:
        text = path.read_text(encoding="utf-8", errors="ignore")
        file_path = rel(path, root)
        for match in re.finditer(r'^\s*#\s*include\s+"([^"]+)"', text, re.M):
            target = resolve_include(match.group(1), path, root, known)
            includes.append({"from": file_path, "to": target or match.group(1), "raw": match.group(1)})
        parsed_entities, parsed_functions = parse_entities(path, root, text)
        entities.extend(parsed_entities)
        functions.extend(parsed_functions)

    name_to_entities: dict[str, list[Entity]] = {}
    for entity in entities:
        name_to_entities.setdefault(entity.name, []).append(entity)

    def resolve_type(name: str) -> str:
        matches = name_to_entities.get(name, [])
        return matches[0].id if len(matches) == 1 else name

    edges: list[dict[str, str]] = []
    for include in includes:
        if include["to"] in known:
            edges.append({"from": include["from"], "to": include["to"], "kind": "include"})
    for entity in entities:
        edges.append({"from": entity.file, "to": entity.id, "kind": "declares"})
        for base in entity.bases:
            edges.append({"from": entity.id, "to": resolve_type(base), "kind": "inherits"})
        for field_info in entity.fields:
            for field_type in extract_type_names(field_info["type"]):
                edges.append(
                    {
                        "from": entity.id,
                        "to": resolve_type(field_type),
                        "kind": "field",
                        "label": field_info["name"],
                    }
                )
        for method in entity.methods:
            for dep in extract_type_names(method.get("return_type", "")):
                edges.append({"from": entity.id, "to": resolve_type(dep), "kind": "method_return"})
            for dep in extract_type_names(method.get("params", "")):
                edges.append({"from": entity.id, "to": resolve_type(dep), "kind": "method_param"})
    for func in functions:
        edges.append({"from": func.file, "to": func.id, "kind": "defines"})
        if func.owner:
            edges.append({"from": func.id, "to": resolve_type(func.owner), "kind": "method_of"})
        for dep in extract_type_names(func.return_type):
            edges.append({"from": func.id, "to": resolve_type(dep), "kind": "returns"})
        for param in func.params:
            for dep in extract_type_names(param):
                edges.append({"from": func.id, "to": resolve_type(dep), "kind": "param"})

    nodes = [{"id": file_path, "kind": "file", "label": file_path} for file_path in sorted(known)]
    for entity in entities:
        nodes.append(
            {
                "id": entity.id,
                "kind": entity.kind,
                "label": entity.name,
                "file": entity.file,
                "line": entity.line,
                "definition": entity.definition,
                "namespace": entity.namespace,
                "fields": entity.fields,
                "methods": entity.methods,
            }
        )
    for func in functions:
        nodes.append(
            {
                "id": func.id,
                "kind": "function",
                "label": f"{func.owner + '::' if func.owner else ''}{func.name}",
                "file": func.file,
                "line": func.line,
                "return_type": func.return_type,
                "params": func.params,
            }
        )
    return {
        "roots": ROOTS,
        "source_count": len(sources),
        "entity_count": len(entities),
        "function_count": len(functions),
        "nodes": nodes,
        "edges": edges,
    }


def entity_layer_report(graph: dict[str, object]) -> list[dict[str, object]]:
    nodes = graph["nodes"]
    edges = graph["edges"]
    entity_ids = {
        node["id"]
        for node in nodes
        if node["kind"] in {"class", "struct", "enum", "enum_class"} and node.get("definition")
    }
    incoming: dict[str, int] = {entity_id: 0 for entity_id in entity_ids}
    outgoing: dict[str, int] = {entity_id: 0 for entity_id in entity_ids}
    kind_counts: dict[str, dict[str, int]] = {entity_id: {} for entity_id in entity_ids}
    for edge in edges:
        src = edge["from"]
        dst = edge["to"]
        if src in entity_ids and edge["kind"] in {"inherits", "field", "method_param", "method_return"}:
            outgoing[src] += 1
            kind_counts[src][edge["kind"]] = kind_counts[src].get(edge["kind"], 0) + 1
        if dst in entity_ids:
            incoming[dst] += 1
    report = []
    for node in nodes:
        if node["id"] not in entity_ids:
            continue
        method_count = len(node.get("methods", []))
        field_count = len(node.get("fields", []))
        report.append(
            {
                "name": node["label"],
                "kind": node["kind"],
                "file": node.get("file", ""),
                "line": node.get("line", 0),
                "incoming": incoming[node["id"]],
                "outgoing": outgoing[node["id"]],
                "field_count": field_count,
                "method_count": method_count,
                "relation_counts": kind_counts[node["id"]],
                "thin_abstraction_score": (
                    1
                    if node["kind"] in {"class", "struct"} and field_count == 0 and method_count <= 2
                    else 0
                ),
            }
        )
    return sorted(report, key=lambda item: (-int(item["incoming"]) - int(item["outgoing"]), item["file"]))


def write_dot(graph: dict[str, object], path: Path) -> None:
    with path.open("w", encoding="utf-8") as handle:
        handle.write("digraph BLITZAR_AST {\n")
        handle.write("  rankdir=LR;\n")
        handle.write('  node [shape=box, fontsize=10, fontname="monospace"];\n')
        nodes = graph["nodes"]
        entity_ids = {node["id"] for node in nodes if node["kind"] in {"class", "struct", "enum", "enum_class"}}
        function_ids = {node["id"] for node in nodes if node["kind"] == "function"}
        for node in nodes:
            if node["id"] not in entity_ids and node["id"] not in function_ids:
                continue
            label = node["label"].replace('"', '\\"')
            shape = "ellipse" if node["kind"] == "function" else "box"
            handle.write(f'  "{node["id"]}" [label="{label}", shape={shape}];\n')
        allowed = {"inherits", "field", "method_param", "method_return", "returns", "param", "method_of"}
        for edge in graph["edges"]:
            if edge["kind"] not in allowed:
                continue
            handle.write(
                f'  "{edge["from"]}" -> "{edge["to"]}" '
                f'[label="{edge["kind"]}", fontsize=8];\n'
            )
        handle.write("}\n")


def write_summary(graph: dict[str, object], report: list[dict[str, object]], path: Path) -> None:
    thin = [item for item in report if item["thin_abstraction_score"]]
    hot = report[:25]
    with path.open("w", encoding="utf-8") as handle:
        handle.write("# BLITZAR AST Graph Summary\n\n")
        handle.write(f"- Sources scanned: {graph['source_count']}\n")
        handle.write(f"- Entities found: {graph['entity_count']}\n")
        handle.write(f"- Functions found: {graph['function_count']}\n")
        handle.write(f"- Edges found: {len(graph['edges'])}\n")
        handle.write(f"- Thin abstraction candidates: {len(thin)}\n\n")
        handle.write("## Most Connected Entities\n\n")
        handle.write("| Entity | File | In | Out | Fields | Methods |\n")
        handle.write("| --- | --- | ---: | ---: | ---: | ---: |\n")
        for item in hot:
            handle.write(
                f"| {item['name']} | {item['file']}:{item['line']} | {item['incoming']} | "
                f"{item['outgoing']} | {item['field_count']} | {item['method_count']} |\n"
            )
        handle.write("\n## Thin Abstraction Candidates\n\n")
        handle.write("| Entity | File | In | Out | Fields | Methods |\n")
        handle.write("| --- | --- | ---: | ---: | ---: | ---: |\n")
        for item in thin[:80]:
            handle.write(
                f"| {item['name']} | {item['file']}:{item['line']} | {item['incoming']} | "
                f"{item['outgoing']} | {item['field_count']} | {item['method_count']} |\n"
            )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default=".", type=Path)
    parser.add_argument("--out", default="build/analysis", type=Path)
    args = parser.parse_args()
    root = args.root.resolve()
    out_dir = (root / args.out).resolve() if not args.out.is_absolute() else args.out
    out_dir.mkdir(parents=True, exist_ok=True)

    graph = build_graph(root)
    report = entity_layer_report(graph)
    (out_dir / "ast_graph.json").write_text(json.dumps(graph, indent=2), encoding="utf-8")
    write_dot(graph, out_dir / "ast_graph.dot")
    write_summary(graph, report, out_dir / "ast_summary.md")
    with (out_dir / "class_layers.csv").open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(
            handle,
            fieldnames=[
                "name",
                "kind",
                "file",
                "line",
                "incoming",
                "outgoing",
                "field_count",
                "method_count",
                "relation_counts",
                "thin_abstraction_score",
            ],
        )
        writer.writeheader()
        writer.writerows(report)
    print(f"wrote {out_dir / 'ast_graph.json'}")
    print(f"wrote {out_dir / 'ast_graph.dot'}")
    print(f"wrote {out_dir / 'ast_summary.md'}")
    print(f"wrote {out_dir / 'class_layers.csv'}")
    print(
        f"scanned={graph['source_count']} entities={graph['entity_count']} "
        f"functions={graph['function_count']} edges={len(graph['edges'])}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
