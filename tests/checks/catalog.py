#!/usr/bin/env python3
# @file tests/checks/catalog.py
# @author Luis1454
# @project BLITZAR
# @brief Automated verification assets for BLITZAR quality gates.

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


# @brief Documents the import symbol operation contract.
# @param qualified_name Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def import_symbol(qualified_name: str) -> Any:
    module_name, symbol_name = qualified_name.rsplit(".", 1)
    module = import_module(module_name)
    return getattr(module, symbol_name)


# @brief Documents the build factory operation contract.
# @param qualified_name Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def _build_factory(qualified_name: str) -> CheckFactory:
    # @brief Documents the factory operation contract.
    # @param None This contract does not take explicit parameters.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _factory() -> BaseCheck:
        check_type = cast(type[BaseCheck], import_symbol(qualified_name))
        return check_type()

    return _factory


@lru_cache(maxsize=1)
# @brief Documents the load catalog operation contract.
# @param None This contract does not take explicit parameters.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def load_catalog() -> dict[str, Any]:
    return cast(dict[str, Any], json.loads(CATALOG_PATH.read_text(encoding="utf-8")))


# @brief Documents the load check registry operation contract.
# @param None This contract does not take explicit parameters.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def load_check_registry() -> dict[str, CheckFactory]:
    payload = load_catalog()["check_registry"]["checks"]
    return {name: _build_factory(qualified_name) for name, qualified_name in payload.items()}


# @brief Documents the load check sequences operation contract.
# @param None This contract does not take explicit parameters.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def load_check_sequences() -> dict[str, list[str]]:
    payload = load_catalog()["check_registry"].get("sequences", {})
    return {name: list(sequence) for name, sequence in payload.items()}


# @brief Documents the load command specs operation contract.
# @param None This contract does not take explicit parameters.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def load_command_specs() -> dict[str, dict[str, Any]]:
    payload = load_catalog()["commands"]
    return {name: dict(spec) for name, spec in payload.items()}


# @brief Documents the load command spec operation contract.
# @param command_name Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def load_command_spec(command_name: str) -> dict[str, Any]:
    commands = load_command_specs()
    return commands[command_name]
