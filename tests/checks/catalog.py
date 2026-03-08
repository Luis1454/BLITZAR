#!/usr/bin/env python3
from __future__ import annotations

import json
from collections.abc import Callable
from functools import lru_cache
from importlib import import_module
from pathlib import Path
from typing import Any, cast

from python_tools.core.base_check import BaseCheck

CATALOG_PATH = Path(__file__).with_name("catalog.json")
CheckFactory = Callable[[], BaseCheck]


def import_symbol(qualified_name: str) -> Any:
    module_name, symbol_name = qualified_name.rsplit(".", 1)
    module = import_module(module_name)
    return getattr(module, symbol_name)


def _build_factory(qualified_name: str) -> CheckFactory:
    def _factory() -> BaseCheck:
        check_type = cast(type[BaseCheck], import_symbol(qualified_name))
        return check_type()

    return _factory


@lru_cache(maxsize=1)
def load_catalog() -> dict[str, Any]:
    return cast(dict[str, Any], json.loads(CATALOG_PATH.read_text(encoding="utf-8")))


def load_check_registry() -> dict[str, CheckFactory]:
    payload = load_catalog()["check_registry"]["checks"]
    return {name: _build_factory(qualified_name) for name, qualified_name in payload.items()}


def load_check_sequences() -> dict[str, list[str]]:
    payload = load_catalog()["check_registry"].get("sequences", {})
    return {name: list(sequence) for name, sequence in payload.items()}


def load_command_specs() -> dict[str, dict[str, Any]]:
    payload = load_catalog()["commands"]
    return {name: dict(spec) for name, spec in payload.items()}


def load_command_spec(command_name: str) -> dict[str, Any]:
    commands = load_command_specs()
    return commands[command_name]
