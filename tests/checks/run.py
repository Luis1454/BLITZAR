#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import sys
from pathlib import Path
from typing import Any, cast

ROOT = Path(__file__).resolve().parents[2]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from python_tools.core.base_check import BaseCheck
from python_tools.core.models import CheckContext
from python_tools.core.reporting import ResultReporter
from tests.checks.catalog import import_symbol, load_command_spec, load_command_specs


def _build_root_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Run catalog-driven repository quality commands.")
    subparsers = parser.add_subparsers(dest="command", required=True)
    for command_name, spec in sorted(load_command_specs().items()):
        subparser = subparsers.add_parser(command_name, help=spec["description"], description=spec["description"])
        for argument in spec["arguments"]:
            flags = argument["flags"]
            kwargs = {
                "help": argument["help"],
            }
            if "type" in argument:
                if argument["type"] == "int":
                    kwargs["type"] = int
                else:
                    raise ValueError(f"unsupported argument type: {argument['type']}")
            if "dest" in argument:
                kwargs["dest"] = argument["dest"]
            if "required" in argument:
                kwargs["required"] = argument["required"]
            if "action" in argument:
                kwargs["action"] = argument["action"]
            if "default" in argument or "default_env" in argument:
                default = argument.get("default")
                env_name = argument.get("default_env", "")
                if env_name:
                    default = os.getenv(env_name, default)
                kwargs["default"] = default
            subparser.add_argument(*flags, **kwargs)
    return parser


def _resolve_value(spec: dict[str, Any], args: argparse.Namespace) -> Any:
    source = spec["source"]
    if source == "arg":
        value = getattr(args, spec["name"])
    elif source == "env":
        value = os.getenv(spec["name"], spec.get("default", ""))
    elif source == "repo_root":
        value = ROOT
    elif source == "mapping":
        return {key: _resolve_value(item, args) for key, item in spec["items"].items()}
    elif source == "literal":
        value = spec.get("value")
    else:
        raise ValueError(f"unsupported catalog source: {source}")
    if spec.get("path") == "resolve" and isinstance(value, str):
        value = Path(value).resolve()
    if spec.get("tuple") and isinstance(value, list):
        value = tuple(value)
    return value


def build_context(args: argparse.Namespace) -> CheckContext:
    spec = load_command_spec(args.command)
    payload = {field_name: _resolve_value(field_spec, args) for field_name, field_spec in spec["context"].items()}
    return CheckContext(**payload)


def build_check(args: argparse.Namespace) -> BaseCheck:
    spec = load_command_spec(args.command)
    check_type = cast(type[BaseCheck], import_symbol(spec["check"]))
    return check_type()


def main(argv: list[str] | None = None) -> int:
    parser = _build_root_parser()
    args = parser.parse_args(argv)
    context = build_context(args)
    check = build_check(args)
    return 0 if ResultReporter().emit(check.run(context)) else 1


if __name__ == "__main__":
    raise SystemExit(main())
